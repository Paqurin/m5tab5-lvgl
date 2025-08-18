#include "performance_monitor.h"
#include "memory_manager.h"
#include <esp_log.h>
#include <esp_heap_caps.h>
#include <esp_system.h>
#include <esp_timer.h>
#include <soc/rtc.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <algorithm>

static const char* TAG = "PerformanceMonitor";

os_error_t PerformanceMonitor::initialize() {
    if (m_initialized) {
        return OS_OK;
    }

    ESP_LOGI(TAG, "Initializing Performance Monitor");

    // Initialize statistics
    m_frameStats = {};
    m_memoryStats = {};
    m_cpuStats = {};
    m_systemStats = {};

    // Reserve space for history vectors
    m_frameStats.fpsHistory.reserve(MAX_HISTORY_SIZE);
    m_memoryStats.heapHistory.reserve(MAX_HISTORY_SIZE);
    m_cpuStats.loadHistory.reserve(MAX_HISTORY_SIZE);

    // Initialize timestamps
    m_lastUpdateTime = millis();
    m_lastFrameTimestamp = esp_timer_get_time();
    m_frameStats.lastFrameTime = m_lastFrameTimestamp;

    // Set initial values
    m_frameStats.minFPS = 60.0f;
    m_frameStats.maxFPS = 60.0f;
    m_frameStats.averageFPS = 60.0f;
    m_frameStats.currentFPS = 60.0f;

    m_initialized = true;
    ESP_LOGI(TAG, "Performance Monitor initialized successfully");

    return OS_OK;
}

os_error_t PerformanceMonitor::shutdown() {
    if (!m_initialized) {
        return OS_OK;
    }

    ESP_LOGI(TAG, "Shutting down Performance Monitor");

    // Clear all data
    m_frameStats.fpsHistory.clear();
    m_memoryStats.heapHistory.clear();
    m_cpuStats.loadHistory.clear();
    m_alerts.clear();
    m_taskExecutions.clear();

    m_initialized = false;
    ESP_LOGI(TAG, "Performance Monitor shutdown complete");

    return OS_OK;
}

os_error_t PerformanceMonitor::update(uint32_t deltaTime) {
    if (!m_initialized) {
        return OS_ERROR_NOT_INITIALIZED;
    }

    uint32_t currentTime = millis();
    
    // Update statistics every measurement window
    if (currentTime - m_lastUpdateTime >= m_measurementWindow) {
        updateFrameStats();
        updateMemoryStats();
        updateCPUStats();
        updateSystemStats();
        
        if (m_realTimeMonitoring) {
            checkPerformanceAlerts();
        }
        
        m_lastUpdateTime = currentTime;
    }

    return OS_OK;
}

void PerformanceMonitor::recordFrameTime(uint64_t frameTime) {
    if (!m_initialized) return;

    uint64_t currentTime = esp_timer_get_time();
    uint64_t frameDuration = currentTime - m_frameStats.lastFrameTime;
    
    if (frameDuration > 0) {
        float currentFPS = 1000000.0f / frameDuration; // Convert to FPS
        m_frameStats.currentFPS = currentFPS;
        
        // Update min/max
        if (currentFPS < m_frameStats.minFPS) {
            m_frameStats.minFPS = currentFPS;
        }
        if (currentFPS > m_frameStats.maxFPS) {
            m_frameStats.maxFPS = currentFPS;
        }
        
        // Count dropped frames
        if (currentFPS < 55.0f) {
            m_frameStats.droppedFrames++;
        }
        
        m_frameStats.totalFrames++;
    }
    
    m_frameStats.lastFrameTime = currentTime;
    m_frameCount++;
}

void PerformanceMonitor::recordAllocation(size_t size) {
    if (!m_initialized) return;
    
    m_memoryStats.allocationCount++;
}

void PerformanceMonitor::recordDeallocation(size_t size) {
    if (!m_initialized) return;
    
    m_memoryStats.deallocationCount++;
}

void PerformanceMonitor::recordTaskExecution(uint32_t taskId, uint64_t executionTime) {
    if (!m_initialized) return;

    // Find or create task execution info
    auto it = std::find_if(m_taskExecutions.begin(), m_taskExecutions.end(),
                          [taskId](const TaskExecutionInfo& info) {
                              return info.id == taskId;
                          });

    if (it == m_taskExecutions.end()) {
        // Create new task info
        TaskExecutionInfo info = {};
        info.id = taskId;
        info.totalTime = executionTime;
        info.executionCount = 1;
        info.maxTime = executionTime;
        info.averageTime = executionTime / 1000.0f; // Convert to ms
        m_taskExecutions.push_back(info);
    } else {
        // Update existing task info
        it->totalTime += executionTime;
        it->executionCount++;
        if (executionTime > it->maxTime) {
            it->maxTime = executionTime;
        }
        it->averageTime = (it->totalTime / it->executionCount) / 1000.0f; // Convert to ms
    }
}

void PerformanceMonitor::updateFrameStats() {
    if (m_frameCount > 0) {
        // Calculate average FPS over the measurement window
        uint32_t elapsed = millis() - (m_lastUpdateTime - m_measurementWindow);
        if (elapsed > 0) {
            float avgFPS = (m_frameCount * 1000.0f) / elapsed;
            m_frameStats.averageFPS = updateMovingAverage(m_frameStats.fpsHistory, avgFPS, MAX_HISTORY_SIZE);
        }
        m_frameCount = 0;
    }
}

void PerformanceMonitor::updateMemoryStats() {
    // Update heap statistics
    m_memoryStats.totalHeap = ESP.getHeapSize();
    m_memoryStats.freeHeap = ESP.getFreeHeap();
    m_memoryStats.largestFreeBlock = heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT);
    
    // Update PSRAM statistics
    m_memoryStats.totalPSRAM = ESP.getPsramSize();
    m_memoryStats.freePSRAM = ESP.getFreePsram();
    
    // Calculate fragmentation
    if (m_memoryStats.freeHeap > 0) {
        m_memoryStats.heapFragmentation = 
            (1.0f - (float)m_memoryStats.largestFreeBlock / (float)m_memoryStats.freeHeap) * 100.0f;
    }
    
    if (m_memoryStats.freePSRAM > 0) {
        size_t largestPSRAMBlock = heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM);
        m_memoryStats.psramFragmentation = 
            (1.0f - (float)largestPSRAMBlock / (float)m_memoryStats.freePSRAM) * 100.0f;
    }
    
    // Track peak usage
    size_t currentUsage = m_memoryStats.totalHeap - m_memoryStats.freeHeap;
    if (currentUsage > m_memoryStats.peakUsage) {
        m_memoryStats.peakUsage = currentUsage;
    }
    
    // Update history
    updateMovingAverage(m_memoryStats.heapHistory, m_memoryStats.freeHeap, MAX_HISTORY_SIZE);
}

void PerformanceMonitor::updateCPUStats() {
    // Get current CPU frequency (fallback for ESP32-P4)
    m_cpuStats.currentFrequency = 360; // ESP32-P4 default frequency in MHz
    
    // Estimate CPU load based on idle task runtime
    // This is a simplified estimation - in production would use more sophisticated methods
    TaskStatus_t* pxTaskStatusArray;
    volatile UBaseType_t uxArraySize;
    uint32_t ulTotalRunTime;
    
    uxArraySize = uxTaskGetNumberOfTasks();
    pxTaskStatusArray = (TaskStatus_t*)pvPortMalloc(uxArraySize * sizeof(TaskStatus_t));
    
    if (pxTaskStatusArray != NULL) {
        uxArraySize = uxTaskGetSystemState(pxTaskStatusArray, uxArraySize, &ulTotalRunTime);
        
        // Find idle task and calculate load
        uint32_t idleRunTime = 0;
        m_systemStats.taskCount = uxArraySize;
        
        for (UBaseType_t i = 0; i < uxArraySize; i++) {
            if (strstr(pxTaskStatusArray[i].pcTaskName, "IDLE") != NULL) {
                idleRunTime = pxTaskStatusArray[i].ulRunTimeCounter;
                break;
            }
        }
        
        if (ulTotalRunTime > 0) {
            float cpuLoad = 100.0f - ((float)idleRunTime / (float)ulTotalRunTime * 100.0f);
            m_cpuStats.cpuLoad = cpuLoad;
            m_cpuStats.averageLoad = updateMovingAverage(m_cpuStats.loadHistory, cpuLoad, MAX_HISTORY_SIZE);
            
            if (cpuLoad > m_cpuStats.maxLoad) {
                m_cpuStats.maxLoad = cpuLoad;
            }
        }
        
        vPortFree(pxTaskStatusArray);
    }
}

void PerformanceMonitor::updateSystemStats() {
    // Update uptime
    m_systemStats.uptime = millis();
    
    // Update free stack size
    m_systemStats.freeStackSize = uxTaskGetStackHighWaterMark(NULL);
    
    // Update temperature (placeholder - would read from temperature sensor)
    m_systemStats.temperature = 45.0f; // Mock temperature
    
    // Estimate power consumption (placeholder)
    m_systemStats.powerConsumption = 500.0f + (m_cpuStats.cpuLoad * 5.0f);
    
    // Check if system is maintaining real-time performance
    m_systemStats.isRealTime = (m_frameStats.averageFPS >= 58.0f) && 
                               (m_cpuStats.cpuLoad <= 80.0f);
}

void PerformanceMonitor::checkPerformanceAlerts() {
    uint32_t currentTime = millis();
    
    // Clear old alerts
    m_alerts.erase(std::remove_if(m_alerts.begin(), m_alerts.end(),
                                 [currentTime](const PerformanceAlert& alert) {
                                     return (currentTime - alert.timestamp) > ALERT_COOLDOWN_MS;
                                 }), m_alerts.end());
    
    // Check frame rate
    if (m_frameStats.averageFPS < m_minFPSThreshold) {
        addAlert(PerformanceAlert::FRAME_DROP, 
                "Frame rate below threshold", 
                (m_minFPSThreshold - m_frameStats.averageFPS) / m_minFPSThreshold);
    }
    
    // Check CPU load
    if (m_cpuStats.cpuLoad > m_maxCPULoadThreshold) {
        addAlert(PerformanceAlert::HIGH_CPU_LOAD, 
                "CPU load exceeds threshold", 
                (m_cpuStats.cpuLoad - m_maxCPULoadThreshold) / 20.0f);
    }
    
    // Check memory
    float freeMemoryPercent = ((float)m_memoryStats.freeHeap / (float)m_memoryStats.totalHeap) * 100.0f;
    if (freeMemoryPercent < m_minFreeMemoryThreshold) {
        addAlert(PerformanceAlert::LOW_MEMORY, 
                "Low memory warning", 
                (m_minFreeMemoryThreshold - freeMemoryPercent) / m_minFreeMemoryThreshold);
    }
    
    // Check fragmentation
    if (m_memoryStats.heapFragmentation > m_maxFragmentationThreshold) {
        addAlert(PerformanceAlert::HIGH_FRAGMENTATION, 
                "High memory fragmentation", 
                m_memoryStats.heapFragmentation / 100.0f);
    }
}

void PerformanceMonitor::addAlert(PerformanceAlert::Type type, const char* message, float severity) {
    // Check if similar alert exists recently
    uint32_t currentTime = millis();
    for (const auto& alert : m_alerts) {
        if (alert.type == type && (currentTime - alert.timestamp) < ALERT_COOLDOWN_MS) {
            return; // Don't add duplicate alert
        }
    }
    
    PerformanceAlert alert;
    alert.type = type;
    alert.message = message;
    alert.timestamp = currentTime;
    alert.severity = std::min(1.0f, std::max(0.0f, severity));
    
    m_alerts.push_back(alert);
    
    // Call callback if registered
    if (m_performanceCallback) {
        m_performanceCallback(alert, m_callbackUserData);
    }
    
    ESP_LOGW(TAG, "Performance Alert: %s (severity: %.2f)", message, severity);
}

float PerformanceMonitor::updateMovingAverage(std::vector<float>& history, float newValue, size_t maxSize) {
    history.push_back(newValue);
    if (history.size() > maxSize) {
        history.erase(history.begin());
    }
    
    float sum = 0.0f;
    for (float value : history) {
        sum += value;
    }
    
    return sum / history.size();
}

char PerformanceMonitor::generatePerformanceGrade() const {
    float score = getPerformanceScore();
    
    if (score >= 90.0f) return 'A';
    if (score >= 80.0f) return 'B';
    if (score >= 70.0f) return 'C';
    if (score >= 60.0f) return 'D';
    return 'F';
}

float PerformanceMonitor::getPerformanceScore() const {
    float fpsScore = std::min(100.0f, (m_frameStats.averageFPS / TARGET_FPS) * 100.0f);
    float cpuScore = std::max(0.0f, 100.0f - m_cpuStats.cpuLoad);
    float memoryScore = ((float)m_memoryStats.freeHeap / (float)m_memoryStats.totalHeap) * 100.0f;
    float fragmentationScore = std::max(0.0f, 100.0f - m_memoryStats.heapFragmentation);
    
    // Weighted average
    return (fpsScore * 0.4f + cpuScore * 0.3f + memoryScore * 0.2f + fragmentationScore * 0.1f);
}

bool PerformanceMonitor::isPerformingOptimally() const {
    return (m_frameStats.averageFPS >= m_minFPSThreshold) &&
           (m_cpuStats.cpuLoad <= m_maxCPULoadThreshold) &&
           (((float)m_memoryStats.freeHeap / (float)m_memoryStats.totalHeap * 100.0f) >= m_minFreeMemoryThreshold) &&
           (m_memoryStats.heapFragmentation <= m_maxFragmentationThreshold);
}

void PerformanceMonitor::setAlertThresholds(float minFPS, float maxCPULoad, float minFreeMemory) {
    m_minFPSThreshold = minFPS;
    m_maxCPULoadThreshold = maxCPULoad;
    m_minFreeMemoryThreshold = minFreeMemory;
    
    ESP_LOGI(TAG, "Alert thresholds updated: FPS>%.1f, CPU<%.1f%%, Memory>%.1f%%", 
             minFPS, maxCPULoad, minFreeMemory);
}

void PerformanceMonitor::printPerformanceReport() const {
    ESP_LOGI(TAG, "=== PERFORMANCE REPORT ===");
    ESP_LOGI(TAG, "Overall Grade: %c (Score: %.1f/100)", generatePerformanceGrade(), getPerformanceScore());
    ESP_LOGI(TAG, "System Status: %s", isPerformingOptimally() ? "OPTIMAL" : "NEEDS ATTENTION");
    ESP_LOGI(TAG, "");
    
    ESP_LOGI(TAG, "=== FRAME RATE ===");
    ESP_LOGI(TAG, "Current FPS: %.1f", m_frameStats.currentFPS);
    ESP_LOGI(TAG, "Average FPS: %.1f", m_frameStats.averageFPS);
    ESP_LOGI(TAG, "Min/Max FPS: %.1f / %.1f", m_frameStats.minFPS, m_frameStats.maxFPS);
    ESP_LOGI(TAG, "Total Frames: %d", m_frameStats.totalFrames);
    ESP_LOGI(TAG, "Dropped Frames: %d (%.2f%%)", m_frameStats.droppedFrames,
             m_frameStats.totalFrames > 0 ? (float)m_frameStats.droppedFrames / m_frameStats.totalFrames * 100.0f : 0.0f);
    ESP_LOGI(TAG, "");
    
    ESP_LOGI(TAG, "=== CPU PERFORMANCE ===");
    ESP_LOGI(TAG, "Current Load: %.1f%%", m_cpuStats.cpuLoad);
    ESP_LOGI(TAG, "Average Load: %.1f%%", m_cpuStats.averageLoad);
    ESP_LOGI(TAG, "Max Load: %.1f%%", m_cpuStats.maxLoad);
    ESP_LOGI(TAG, "CPU Frequency: %d MHz", m_cpuStats.currentFrequency);
    ESP_LOGI(TAG, "");
    
    ESP_LOGI(TAG, "=== MEMORY USAGE ===");
    ESP_LOGI(TAG, "Free Heap: %d KB / %d KB (%.1f%%)", 
             m_memoryStats.freeHeap / 1024, m_memoryStats.totalHeap / 1024,
             (float)m_memoryStats.freeHeap / m_memoryStats.totalHeap * 100.0f);
    ESP_LOGI(TAG, "Free PSRAM: %d KB / %d KB (%.1f%%)", 
             m_memoryStats.freePSRAM / 1024, m_memoryStats.totalPSRAM / 1024,
             m_memoryStats.totalPSRAM > 0 ? (float)m_memoryStats.freePSRAM / m_memoryStats.totalPSRAM * 100.0f : 0.0f);
    ESP_LOGI(TAG, "Largest Free Block: %d KB", m_memoryStats.largestFreeBlock / 1024);
    ESP_LOGI(TAG, "Heap Fragmentation: %.1f%%", m_memoryStats.heapFragmentation);
    ESP_LOGI(TAG, "Peak Usage: %d KB", m_memoryStats.peakUsage / 1024);
    ESP_LOGI(TAG, "Allocations: %d", m_memoryStats.allocationCount);
    ESP_LOGI(TAG, "Deallocations: %d", m_memoryStats.deallocationCount);
    ESP_LOGI(TAG, "");
    
    ESP_LOGI(TAG, "=== SYSTEM STATUS ===");
    ESP_LOGI(TAG, "Uptime: %.2f seconds", m_systemStats.uptime / 1000.0f);
    ESP_LOGI(TAG, "Task Count: %d", m_systemStats.taskCount);
    ESP_LOGI(TAG, "Free Stack: %d bytes", m_systemStats.freeStackSize);
    ESP_LOGI(TAG, "Real-time: %s", m_systemStats.isRealTime ? "YES" : "NO");
    ESP_LOGI(TAG, "");
    
    if (!m_alerts.empty()) {
        ESP_LOGI(TAG, "=== ACTIVE ALERTS ===");
        for (const auto& alert : m_alerts) {
            ESP_LOGW(TAG, "%s (severity: %.2f)", alert.message, alert.severity);
        }
    }
}

void PerformanceMonitor::printDashboard() const {
    ESP_LOGI(TAG, "╔══════════════════════════════════════════════════════════════╗");
    ESP_LOGI(TAG, "║                    PERFORMANCE DASHBOARD                     ║");
    ESP_LOGI(TAG, "╠══════════════════════════════════════════════════════════════╣");
    ESP_LOGI(TAG, "║ FPS: %5.1f | CPU: %5.1f%% | Memory: %5.1f%% | Grade: %c      ║", 
             m_frameStats.averageFPS, m_cpuStats.cpuLoad, 
             (float)m_memoryStats.freeHeap / m_memoryStats.totalHeap * 100.0f,
             generatePerformanceGrade());
    ESP_LOGI(TAG, "║ Status: %-12s | Freq: %3d MHz | Alerts: %2d       ║",
             isPerformingOptimally() ? "OPTIMAL" : "DEGRADED",
             m_cpuStats.currentFrequency, m_alerts.size());
    ESP_LOGI(TAG, "╚══════════════════════════════════════════════════════════════╝");
}

void PerformanceMonitor::resetStatistics() {
    ESP_LOGI(TAG, "Resetting performance statistics");
    
    m_frameStats.totalFrames = 0;
    m_frameStats.droppedFrames = 0;
    m_frameStats.minFPS = 60.0f;
    m_frameStats.maxFPS = 60.0f;
    m_frameStats.fpsHistory.clear();
    
    m_memoryStats.peakUsage = 0;
    m_memoryStats.allocationCount = 0;
    m_memoryStats.deallocationCount = 0;
    m_memoryStats.heapHistory.clear();
    
    m_cpuStats.maxLoad = 0.0f;
    m_cpuStats.loadHistory.clear();
    
    m_alerts.clear();
    m_taskExecutions.clear();
}

void PerformanceMonitor::registerPerformanceCallback(void(*callback)(const PerformanceAlert&, void*), void* userData) {
    m_performanceCallback = callback;
    m_callbackUserData = userData;
    ESP_LOGI(TAG, "Performance callback registered");
}