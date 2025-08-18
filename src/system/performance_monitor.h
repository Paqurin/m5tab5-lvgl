#ifndef PERFORMANCE_MONITOR_H
#define PERFORMANCE_MONITOR_H

#include "os_config.h"
#include <vector>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_timer.h>

/**
 * @file performance_monitor.h
 * @brief Real-time Performance Monitoring System for M5Stack Tab5
 * 
 * Provides comprehensive performance tracking including:
 * - Frame rate monitoring
 * - Memory usage tracking
 * - CPU load analysis
 * - Task execution timing
 * - System bottleneck detection
 */

struct FrameStats {
    float averageFPS = 60.0f;
    float currentFPS = 60.0f;
    float minFPS = 60.0f;
    float maxFPS = 60.0f;
    uint32_t totalFrames = 0;
    uint32_t droppedFrames = 0;
    uint64_t lastFrameTime = 0;
    std::vector<float> fpsHistory;
};

struct MemoryStats {
    size_t totalHeap = 0;
    size_t freeHeap = 0;
    size_t totalPSRAM = 0;
    size_t freePSRAM = 0;
    size_t largestFreeBlock = 0;
    float heapFragmentation = 0.0f;
    float psramFragmentation = 0.0f;
    size_t peakUsage = 0;
    uint32_t allocationCount = 0;
    uint32_t deallocationCount = 0;
    std::vector<float> heapHistory;
};

struct CPUStats {
    float cpuLoad = 0.0f;
    float averageLoad = 0.0f;
    float maxLoad = 0.0f;
    uint32_t currentFrequency = 360;
    uint32_t idleTime = 0;
    uint32_t taskSwitches = 0;
    std::vector<float> loadHistory;
};

struct SystemStats {
    uint32_t uptime = 0;
    uint32_t freeStackSize = 0;
    uint32_t taskCount = 0;
    float temperature = 0.0f;
    float powerConsumption = 0.0f;
    bool isRealTime = true;
    uint32_t interruptCount = 0;
};

struct PerformanceAlert {
    enum Type {
        FRAME_DROP,
        HIGH_CPU_LOAD,
        LOW_MEMORY,
        HIGH_FRAGMENTATION,
        TASK_OVERRUN,
        THERMAL_WARNING
    };
    
    Type type;
    const char* message;
    uint32_t timestamp;
    float severity; // 0.0 - 1.0
};

class PerformanceMonitor {
public:
    PerformanceMonitor() = default;
    ~PerformanceMonitor() = default;

    /**
     * @brief Initialize performance monitor
     * @return OS_OK on success, error code on failure
     */
    os_error_t initialize();

    /**
     * @brief Shutdown performance monitor
     * @return OS_OK on success, error code on failure
     */
    os_error_t shutdown();

    /**
     * @brief Update performance statistics
     * @param deltaTime Time since last update in milliseconds
     * @return OS_OK on success, error code on failure
     */
    os_error_t update(uint32_t deltaTime);

    /**
     * @brief Record frame timing
     * @param frameTime Frame time in microseconds
     */
    void recordFrameTime(uint64_t frameTime);

    /**
     * @brief Record memory allocation
     * @param size Allocation size in bytes
     */
    void recordAllocation(size_t size);

    /**
     * @brief Record memory deallocation
     * @param size Deallocation size in bytes
     */
    void recordDeallocation(size_t size);

    /**
     * @brief Record task execution time
     * @param taskId Task identifier
     * @param executionTime Execution time in microseconds
     */
    void recordTaskExecution(uint32_t taskId, uint64_t executionTime);

    /**
     * @brief Check if system is maintaining 60Hz
     * @return true if maintaining target framerate
     */
    bool isMaintaining60Hz() const { return m_frameStats.averageFPS >= 58.0f; }

    /**
     * @brief Get current frame rate
     * @return Current FPS
     */
    float getCurrentFPS() const { return m_frameStats.currentFPS; }

    /**
     * @brief Get average frame rate
     * @return Average FPS over last measurement window
     */
    float getAverageFPS() const { return m_frameStats.averageFPS; }

    /**
     * @brief Get frame statistics
     * @return Frame statistics structure
     */
    const FrameStats& getFrameStats() const { return m_frameStats; }

    /**
     * @brief Get memory statistics
     * @return Memory statistics structure
     */
    const MemoryStats& getMemoryStats() const { return m_memoryStats; }

    /**
     * @brief Get CPU statistics
     * @return CPU statistics structure
     */
    const CPUStats& getCPUStats() const { return m_cpuStats; }

    /**
     * @brief Get system statistics
     * @return System statistics structure
     */
    const SystemStats& getSystemStats() const { return m_systemStats; }

    /**
     * @brief Check for performance alerts
     * @return Vector of active performance alerts
     */
    const std::vector<PerformanceAlert>& getAlerts() const { return m_alerts; }

    /**
     * @brief Set performance alert thresholds
     * @param minFPS Minimum acceptable FPS
     * @param maxCPULoad Maximum acceptable CPU load
     * @param minFreeMemory Minimum free memory percentage
     */
    void setAlertThresholds(float minFPS, float maxCPULoad, float minFreeMemory);

    /**
     * @brief Generate performance report
     * @return Performance grade (A-F)
     */
    char generatePerformanceGrade() const;

    /**
     * @brief Print comprehensive performance report
     */
    void printPerformanceReport() const;

    /**
     * @brief Print real-time performance dashboard
     */
    void printDashboard() const;

    /**
     * @brief Check if system is performing optimally
     * @return true if all metrics are within acceptable ranges
     */
    bool isPerformingOptimally() const;

    /**
     * @brief Get performance score (0-100)
     * @return Overall performance score
     */
    float getPerformanceScore() const;

    /**
     * @brief Reset all statistics
     */
    void resetStatistics();

    /**
     * @brief Enable/disable real-time monitoring
     * @param enabled true to enable continuous monitoring
     */
    void setRealTimeMonitoring(bool enabled) { m_realTimeMonitoring = enabled; }

    /**
     * @brief Register performance callback
     * @param callback Callback function for performance events
     * @param userData User data for callback
     */
    void registerPerformanceCallback(void(*callback)(const PerformanceAlert&, void*), void* userData);

private:
    /**
     * @brief Update frame rate statistics
     */
    void updateFrameStats();

    /**
     * @brief Update memory statistics
     */
    void updateMemoryStats();

    /**
     * @brief Update CPU statistics
     */
    void updateCPUStats();

    /**
     * @brief Update system statistics
     */
    void updateSystemStats();

    /**
     * @brief Check for performance alerts
     */
    void checkPerformanceAlerts();

    /**
     * @brief Add performance alert
     * @param type Alert type
     * @param message Alert message
     * @param severity Alert severity (0.0-1.0)
     */
    void addAlert(PerformanceAlert::Type type, const char* message, float severity);

    /**
     * @brief Calculate moving average
     * @param history History vector
     * @param newValue New value to add
     * @param maxSize Maximum history size
     * @return Updated average
     */
    float updateMovingAverage(std::vector<float>& history, float newValue, size_t maxSize);

    // Statistics
    FrameStats m_frameStats;
    MemoryStats m_memoryStats;
    CPUStats m_cpuStats;
    SystemStats m_systemStats;

    // Alerts and thresholds
    std::vector<PerformanceAlert> m_alerts;
    float m_minFPSThreshold = 55.0f;
    float m_maxCPULoadThreshold = 80.0f;
    float m_minFreeMemoryThreshold = 20.0f;
    float m_maxFragmentationThreshold = 30.0f;

    // Monitoring state
    bool m_initialized = false;
    bool m_realTimeMonitoring = true;
    uint32_t m_lastUpdateTime = 0;
    uint32_t m_measurementWindow = 1000; // 1 second

    // Callback system
    void(*m_performanceCallback)(const PerformanceAlert&, void*) = nullptr;
    void* m_callbackUserData = nullptr;

    // Performance tracking
    uint64_t m_lastFrameTimestamp = 0;
    uint32_t m_frameCount = 0;
    uint32_t m_lastCPUMeasurement = 0;
    uint32_t m_lastMemoryMeasurement = 0;

    // Task execution tracking
    struct TaskExecutionInfo {
        uint32_t id;
        uint64_t totalTime;
        uint32_t executionCount;
        uint64_t maxTime;
        float averageTime;
    };
    std::vector<TaskExecutionInfo> m_taskExecutions;

    // Configuration constants
    static constexpr size_t MAX_HISTORY_SIZE = 60; // 1 minute at 1Hz
    static constexpr float TARGET_FPS = 60.0f;
    static constexpr uint32_t ALERT_COOLDOWN_MS = 5000; // 5 seconds between same alerts
    static constexpr const char* TAG = "PerformanceMonitor";
};

// Performance macros for easy integration
#define PERF_MONITOR_FRAME_START() \
    uint64_t _perf_frame_start = esp_timer_get_time()

#define PERF_MONITOR_FRAME_END(monitor) \
    do { \
        uint64_t _perf_frame_time = esp_timer_get_time() - _perf_frame_start; \
        (monitor)->recordFrameTime(_perf_frame_time); \
    } while(0)

#define PERF_MONITOR_TASK_START() \
    uint64_t _perf_task_start = esp_timer_get_time()

#define PERF_MONITOR_TASK_END(monitor, taskId) \
    do { \
        uint64_t _perf_task_time = esp_timer_get_time() - _perf_task_start; \
        (monitor)->recordTaskExecution(taskId, _perf_task_time); \
    } while(0)

#endif // PERFORMANCE_MONITOR_H