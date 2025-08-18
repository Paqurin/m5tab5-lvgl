#include "memory_manager.h"
#include <esp_log.h>
#include <esp_heap_caps.h>
#include <esp_cache.h>
#include <esp_pm.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

static const char* TAG = "MemoryManager";

// MemoryPool Implementation with PSRAM optimization
MemoryPool::MemoryPool(size_t blockSize, size_t blockCount) 
    : m_blockSize(blockSize), m_totalBlocks(blockCount), m_freeBlocks(blockCount) {
    
    size_t totalSize = blockSize * blockCount;
    
    // Use PSRAM for large allocations, internal RAM for small/frequent ones
    uint32_t caps = (totalSize > 32768) ? MALLOC_CAP_SPIRAM : MALLOC_CAP_INTERNAL;
    if (caps == MALLOC_CAP_SPIRAM) {
        caps |= MALLOC_CAP_32BIT; // Ensure 32-bit alignment for RISC-V
    }
    
    m_memory = heap_caps_aligned_alloc(32, totalSize, caps); // 32-byte alignment for DMA
    
    if (m_memory) {
        m_blockUsed.resize(blockCount, false);
        m_memoryType = caps;
        
        // Pre-touch memory pages to avoid cache misses
        if (caps & MALLOC_CAP_SPIRAM) {
            volatile uint8_t* ptr = static_cast<uint8_t*>(m_memory);
            for (size_t i = 0; i < totalSize; i += 4096) {
                ptr[i] = 0;
            }
        }
        
        ESP_LOGI(TAG, "Created memory pool: %d blocks of %d bytes (%s)", 
                blockCount, blockSize, (caps & MALLOC_CAP_SPIRAM) ? "PSRAM" : "Internal");
    } else {
        ESP_LOGE(TAG, "Failed to allocate memory pool");
        m_totalBlocks = 0;
        m_freeBlocks = 0;
    }
}

MemoryPool::~MemoryPool() {
    if (m_memory) {
        heap_caps_free(m_memory);
    }
}

void* MemoryPool::allocate() {
    if (m_freeBlocks == 0) {
        return nullptr;
    }

    for (size_t i = 0; i < m_totalBlocks; i++) {
        if (!m_blockUsed[i]) {
            m_blockUsed[i] = true;
            m_freeBlocks--;
            return static_cast<char*>(m_memory) + (i * m_blockSize);
        }
    }

    return nullptr;
}

bool MemoryPool::deallocate(void* ptr) {
    if (!ptr || !m_memory) {
        return false;
    }

    char* charPtr = static_cast<char*>(ptr);
    char* basePtr = static_cast<char*>(m_memory);
    
    if (charPtr < basePtr) {
        return false;
    }

    size_t offset = charPtr - basePtr;
    if (offset % m_blockSize != 0) {
        return false;
    }

    size_t blockIndex = offset / m_blockSize;
    if (blockIndex >= m_totalBlocks) {
        return false;
    }

    if (!m_blockUsed[blockIndex]) {
        return false; // Double free
    }

    m_blockUsed[blockIndex] = false;
    m_freeBlocks++;
    return true;
}

// MemoryManager Implementation
MemoryManager::~MemoryManager() {
    if (m_initialized) {
        checkLeaks();
        if (m_mutex) {
            vSemaphoreDelete(m_mutex);
        }
    }
}

os_error_t MemoryManager::initialize() {
    if (m_initialized) {
        return OS_OK;
    }

    ESP_LOGI(TAG, "Initializing Memory Manager");

    // Create optimized memory pools for ESP32-P4 with PSRAM
    m_pools.push_back(std::make_unique<MemoryPool>(16, 128));   // Small objects - Internal RAM
    m_pools.push_back(std::make_unique<MemoryPool>(64, 64));    // Medium objects - Internal RAM
    m_pools.push_back(std::make_unique<MemoryPool>(256, 32));   // Large objects - Internal RAM
    m_pools.push_back(std::make_unique<MemoryPool>(1024, 16));  // XL objects - PSRAM
    m_pools.push_back(std::make_unique<MemoryPool>(4096, 8));   // XXL objects - PSRAM
    m_pools.push_back(std::make_unique<MemoryPool>(16384, 4));  // Graphics buffers - PSRAM
    
    // Create mutex for thread safety
    m_mutex = xSemaphoreCreateMutex();
    if (!m_mutex) {
        ESP_LOGE(TAG, "Failed to create memory manager mutex");
        return OS_ERROR_NO_MEMORY;
    }

    // Reserve space for tracking allocations
    m_activeBlocks.reserve(256);

    m_initialized = true;
    ESP_LOGI(TAG, "Memory Manager initialized with %d pools", m_pools.size());
    
    // Optimize for real-time performance
    optimizeForRealtime();
    
    return OS_OK;
}

void* MemoryManager::allocate(size_t size, const char* file, int line) {
    if (!m_initialized || size == 0) {
        return nullptr;
    }

    // Thread safety
    if (xSemaphoreTake(m_mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        ESP_LOGW(TAG, "Failed to acquire memory mutex");
        return nullptr;
    }

    void* ptr = nullptr;

    // Try to allocate from pools first for small allocations
    if (size <= 16384) {
        for (auto& pool : m_pools) {
            if (size <= pool->getBlockSize() && pool->getFreeBlocks() > 0) {
                ptr = pool->allocate();
                if (ptr) {
                    break;
                }
            }
        }
    }

    // Fall back to optimized heap allocation
    if (!ptr) {
        uint32_t caps = MALLOC_CAP_DEFAULT;
        
        // Use PSRAM for large allocations
        if (size > 1024) {
            caps = MALLOC_CAP_SPIRAM | MALLOC_CAP_32BIT;
        }
        // Use internal RAM for small, frequent allocations
        else if (size <= 256) {
            caps = MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT;
        }
        
        // Try aligned allocation for better performance
        if (size >= 32) {
            ptr = heap_caps_aligned_alloc(32, size, caps);
        } else {
            ptr = heap_caps_malloc(size, caps);
        }
        
        // Fallback to any available memory
        if (!ptr) {
            ptr = heap_caps_malloc(size, MALLOC_CAP_DEFAULT);
        }
    }

    if (ptr) {
        // Track the allocation
        MemoryBlock block;
        block.ptr = ptr;
        block.size = size;
        block.timestamp = millis();
        block.file = file;
        block.line = line;
        block.inUse = true;

        m_activeBlocks.push_back(block);
        m_totalAllocated += size;
        m_allocationCount++;

        if (m_totalAllocated > m_peakAllocated) {
            m_peakAllocated = m_totalAllocated;
        }
        
        // Update fragmentation tracking
        updateFragmentationStats();

        #if OS_DEBUG_ENABLED >= 3
        ESP_LOGD(TAG, "Allocated %d bytes at %p (%s:%d)", size, ptr, 
                file ? file : "unknown", line);
        #endif
    } else {
        ESP_LOGW(TAG, "Failed to allocate %d bytes", size);
        m_allocationFailures++;
    }

    xSemaphoreGive(m_mutex);
    return ptr;
}

bool MemoryManager::deallocate(void* ptr) {
    if (!ptr || !m_initialized) {
        return false;
    }

    // Thread safety
    if (xSemaphoreTake(m_mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        ESP_LOGW(TAG, "Failed to acquire memory mutex for deallocation");
        return false;
    }

    auto it = findBlock(ptr);
    if (it == m_activeBlocks.end()) {
        ESP_LOGW(TAG, "Attempt to free untracked pointer %p", ptr);
        xSemaphoreGive(m_mutex);
        return false;
    }

    size_t size = it->size;
    bool freedFromPool = false;

    // Try to free from pools first
    for (auto& pool : m_pools) {
        if (pool->deallocate(ptr)) {
            freedFromPool = true;
            break;
        }
    }

    // Fall back to heap deallocation
    if (!freedFromPool) {
        heap_caps_free(ptr);
    }

    // Update tracking
    m_totalAllocated -= size;
    m_deallocationCount++;
    m_activeBlocks.erase(it);
    
    // Update fragmentation tracking
    updateFragmentationStats();

    #if OS_DEBUG_ENABLED >= 3
    ESP_LOGD(TAG, "Deallocated %d bytes at %p", size, ptr);
    #endif

    xSemaphoreGive(m_mutex);
    return true;
}

void* MemoryManager::allocateFromPool(size_t poolIndex) {
    if (!m_initialized || poolIndex >= m_pools.size()) {
        return nullptr;
    }

    return m_pools[poolIndex]->allocate();
}

bool MemoryManager::deallocateFromPool(void* ptr, size_t poolIndex) {
    if (!ptr || !m_initialized || poolIndex >= m_pools.size()) {
        return false;
    }

    return m_pools[poolIndex]->deallocate(ptr);
}

size_t MemoryManager::checkLeaks() {
    size_t leakCount = 0;
    uint32_t currentTime = millis();

    for (const auto& block : m_activeBlocks) {
        if (block.inUse) {
            leakCount++;
            ESP_LOGW(TAG, "Memory leak: %d bytes at %p, allocated %d ms ago (%s:%d)",
                    block.size, block.ptr, currentTime - block.timestamp,
                    block.file ? block.file : "unknown", block.line);
        }
    }

    if (leakCount > 0) {
        ESP_LOGW(TAG, "Found %d memory leaks", leakCount);
    }

    return leakCount;
}

void MemoryManager::printStats() {
    ESP_LOGI(TAG, "=== Memory Statistics ===");
    ESP_LOGI(TAG, "Total allocated: %d bytes", m_totalAllocated);
    ESP_LOGI(TAG, "Peak allocated: %d bytes", m_peakAllocated);
    ESP_LOGI(TAG, "Active allocations: %d", m_activeBlocks.size());
    ESP_LOGI(TAG, "Total allocations: %d", m_allocationCount);
    ESP_LOGI(TAG, "Total deallocations: %d", m_deallocationCount);
    ESP_LOGI(TAG, "Free heap: %d bytes", getFreeHeap());
    ESP_LOGI(TAG, "Largest free block: %d bytes", getLargestFreeBlock());

    ESP_LOGI(TAG, "=== Pool Statistics ===");
    for (size_t i = 0; i < m_pools.size(); i++) {
        auto& pool = m_pools[i];
        ESP_LOGI(TAG, "Pool %d: %d/%d blocks free (%d bytes each)", 
                i, pool->getFreeBlocks(), pool->getTotalBlocks(), pool->getBlockSize());
    }
}

void MemoryManager::garbageCollect() {
    ESP_LOGI(TAG, "Starting garbage collection...");
    
    // Thread safety
    if (xSemaphoreTake(m_mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        ESP_LOGW(TAG, "Failed to acquire memory mutex for GC");
        return;
    }
    
    size_t initialFreeHeap = getFreeHeap();
    size_t initialFreePSRAM = getFreePSRAM();
    
    // Check for leaks and stale allocations
    uint32_t currentTime = millis();
    size_t reclaimedMemory = 0;
    
    auto it = m_activeBlocks.begin();
    while (it != m_activeBlocks.end()) {
        // Check for very old allocations that might be leaks
        if (currentTime - it->timestamp > 300000) { // 5 minutes
            ESP_LOGW(TAG, "Potential leak: %d bytes at %p, age %d ms (%s:%d)",
                    it->size, it->ptr, currentTime - it->timestamp,
                    it->file ? it->file : "unknown", it->line);
        }
        ++it;
    }
    
    // Force heap compaction if available
    #ifdef CONFIG_HEAP_TASK_TRACKING
    heap_caps_check_integrity_all(true);
    #endif
    
    // Update fragmentation statistics
    updateFragmentationStats();
    
    size_t finalFreeHeap = getFreeHeap();
    size_t finalFreePSRAM = getFreePSRAM();
    
    ESP_LOGI(TAG, "GC completed. Heap: %d->%d KB, PSRAM: %d->%d KB",
            initialFreeHeap/1024, finalFreeHeap/1024,
            initialFreePSRAM/1024, finalFreePSRAM/1024);
    
    xSemaphoreGive(m_mutex);
}

size_t MemoryManager::getFreeHeap() const {
    return heap_caps_get_free_size(MALLOC_CAP_DEFAULT);
}

size_t MemoryManager::getLargestFreeBlock() const {
    return heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT);
}

size_t MemoryManager::getFreePSRAM() const {
    return heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
}

size_t MemoryManager::getLargestFreePSRAMBlock() const {
    return heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM);
}

float MemoryManager::getFragmentationRatio() const {
    size_t freeHeap = getFreeHeap();
    size_t largestBlock = getLargestFreeBlock();
    if (freeHeap == 0) return 1.0f;
    return 1.0f - (float)largestBlock / (float)freeHeap;
}

void MemoryManager::updateFragmentationStats() {
    // Update fragmentation tracking for both internal RAM and PSRAM
    m_heapFragmentation = getFragmentationRatio();
    
    size_t freePSRAM = getFreePSRAM();
    size_t largestPSRAMBlock = getLargestFreePSRAMBlock();
    if (freePSRAM > 0) {
        m_psramFragmentation = 1.0f - (float)largestPSRAMBlock / (float)freePSRAM;
    }
}

void* MemoryManager::allocateDMA(size_t size, size_t alignment) {
    if (!m_initialized || size == 0) {
        return nullptr;
    }
    
    // DMA requires specific alignment and internal RAM
    uint32_t caps = MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL;
    
    if (alignment == 0) {
        alignment = 4; // Minimum alignment for ESP32-P4
    }
    
    void* ptr = heap_caps_aligned_alloc(alignment, size, caps);
    
    if (ptr) {
        // Track DMA allocation
        MemoryBlock block;
        block.ptr = ptr;
        block.size = size;
        block.timestamp = millis();
        block.file = "DMA";
        block.line = 0;
        block.inUse = true;
        
        if (xSemaphoreTake(m_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            m_activeBlocks.push_back(block);
            m_totalAllocated += size;
            m_allocationCount++;
            xSemaphoreGive(m_mutex);
        }
        
        ESP_LOGD(TAG, "Allocated %d bytes DMA memory at %p", size, ptr);
    }
    
    return ptr;
}

std::vector<MemoryBlock>::iterator MemoryManager::findBlock(void* ptr) {
    return std::find_if(m_activeBlocks.begin(), m_activeBlocks.end(),
                       [ptr](const MemoryBlock& block) {
                           return block.ptr == ptr;
                       });
}

void MemoryManager::optimizeForRealtime() {
    ESP_LOGI(TAG, "Optimizing memory manager for real-time performance");
    
    // Pre-allocate pools to avoid allocation during runtime
    for (auto& pool : m_pools) {
        // Pre-touch all pool memory
        for (size_t i = 0; i < pool->getTotalBlocks(); i++) {
            void* ptr = pool->allocate();
            if (ptr) {
                // Touch the memory to ensure it's mapped
                memset(ptr, 0, pool->getBlockSize());
                pool->deallocate(ptr);
            }
        }
    }
    
    // Perform initial garbage collection
    garbageCollect();
    
    ESP_LOGI(TAG, "Real-time optimization complete");
}