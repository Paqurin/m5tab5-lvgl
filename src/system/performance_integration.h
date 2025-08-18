#ifndef PERFORMANCE_INTEGRATION_H
#define PERFORMANCE_INTEGRATION_H

#include "os_config.h"
#include "memory_manager.h"
#include "task_scheduler.h"
#include "power_manager.h"
#include "performance_monitor.h"

/**
 * @file performance_integration.h
 * @brief System-wide Performance Integration for M5Stack Tab5
 * 
 * Provides centralized performance management that coordinates:
 * - Memory optimization
 * - Task scheduling optimization
 * - Power management with dynamic frequency scaling
 * - Real-time performance monitoring
 * - Automatic system tuning
 */

enum class SystemPerformanceMode {
    POWER_SAVE,     // Maximum battery life
    BALANCED,       // Balance of performance and power
    PERFORMANCE,    // Maximum performance
    REAL_TIME,      // Real-time optimized for 60Hz
    AUTO            // Automatically adjust based on conditions
};

enum class SystemHealthStatus {
    EXCELLENT,      // All systems optimal
    GOOD,          // Good performance
    FAIR,          // Acceptable performance
    POOR,          // Performance issues detected
    CRITICAL       // Critical performance problems
};

struct SystemPerformanceProfile {
    // Memory configuration
    size_t memoryPoolSizes[6] = {128, 64, 32, 16, 8, 4}; // Pool counts
    bool enableMemoryCompaction = true;
    uint32_t gcIntervalMs = 5000;
    
    // Task scheduling configuration
    uint32_t frameTimeBudgetUs = 13000; // 13ms for 60Hz
    uint8_t realtimeTaskPriority = OS_TASK_PRIORITY_CRITICAL;
    uint32_t taskWatchdogMs = 30000;
    
    // Power management configuration
    uint32_t maxCpuFreqMhz = 360;
    uint32_t minCpuFreqMhz = 80;
    PerformanceMode powerMode = PerformanceMode::BALANCED;
    
    // Display configuration
    uint32_t targetFps = 60;
    bool enableVsync = true;
    bool enableDoubleBuffer = true;
    
    // Audio configuration
    uint32_t audioBufferMs = 20;
    bool enableRealtimeAudio = true;
    uint32_t audioSampleRate = 44100;
};

class PerformanceIntegration {
public:
    PerformanceIntegration() = default;
    ~PerformanceIntegration() = default;

    /**
     * @brief Initialize performance integration system
     * @param memoryMgr Memory manager instance
     * @param taskScheduler Task scheduler instance
     * @param powerMgr Power manager instance
     * @param perfMonitor Performance monitor instance
     * @return OS_OK on success, error code on failure
     */
    os_error_t initialize(MemoryManager* memoryMgr, 
                         TaskScheduler* taskScheduler,
                         PowerManager* powerMgr, 
                         PerformanceMonitor* perfMonitor);

    /**
     * @brief Shutdown performance integration
     * @return OS_OK on success, error code on failure
     */
    os_error_t shutdown();

    /**
     * @brief Update performance integration (main loop)
     * @param deltaTime Time since last update in milliseconds
     * @return OS_OK on success, error code on failure
     */
    os_error_t update(uint32_t deltaTime);

    /**
     * @brief Set system performance mode
     * @param mode Performance mode to apply
     * @return OS_OK on success, error code on failure
     */
    os_error_t setPerformanceMode(SystemPerformanceMode mode);

    /**
     * @brief Get current system performance mode
     * @return Current performance mode
     */
    SystemPerformanceMode getPerformanceMode() const { return m_currentMode; }

    /**
     * @brief Apply performance profile
     * @param profile Performance profile to apply
     * @return OS_OK on success, error code on failure
     */
    os_error_t applyPerformanceProfile(const SystemPerformanceProfile& profile);

    /**
     * @brief Get current system health status
     * @return System health status
     */
    SystemHealthStatus getSystemHealth() const;

    /**
     * @brief Optimize system for 60Hz real-time operation
     * @return OS_OK on success, error code on failure
     */
    os_error_t optimizeFor60Hz();

    /**
     * @brief Optimize system for maximum battery life
     * @return OS_OK on success, error code on failure
     */
    os_error_t optimizeForBattery();

    /**
     * @brief Enable automatic performance tuning
     * @param enabled true to enable auto-tuning
     */
    void setAutoTuningEnabled(bool enabled) { m_autoTuningEnabled = enabled; }

    /**
     * @brief Check if system is maintaining 60Hz performance
     * @return true if maintaining 60Hz consistently
     */
    bool isMaintaining60Hz() const;

    /**
     * @brief Get overall system performance score (0-100)
     * @return Performance score
     */
    float getSystemPerformanceScore() const;

    /**
     * @brief Force garbage collection across all systems
     * @return OS_OK on success, error code on failure
     */
    os_error_t forceGarbageCollection();

    /**
     * @brief Print comprehensive system performance report
     */
    void printSystemPerformanceReport() const;

    /**
     * @brief Register performance alert callback
     * @param callback Callback function for performance alerts
     * @param userData User data for callback
     */
    void registerAlertCallback(void(*callback)(const char*, float, void*), void* userData);

    /**
     * @brief Get performance recommendations
     * @return Vector of performance improvement suggestions
     */
    std::vector<std::string> getPerformanceRecommendations() const;

    /**
     * @brief Emergency performance recovery
     * Attempts to recover from critical performance issues
     * @return OS_OK on success, error code on failure
     */
    os_error_t emergencyPerformanceRecovery();

    /**
     * @brief Benchmark system performance
     * Runs comprehensive performance tests
     * @param durationMs Test duration in milliseconds
     * @return Performance benchmark results
     */
    float benchmarkSystemPerformance(uint32_t durationMs = 10000);

private:
    /**
     * @brief Apply power save optimizations
     */
    void applyPowerSaveOptimizations();

    /**
     * @brief Apply balanced optimizations
     */
    void applyBalancedOptimizations();

    /**
     * @brief Apply performance optimizations
     */
    void applyPerformanceOptimizations();

    /**
     * @brief Apply real-time optimizations
     */
    void applyRealTimeOptimizations();

    /**
     * @brief Perform automatic tuning based on current conditions
     */
    void performAutoTuning();

    /**
     * @brief Monitor system health
     */
    void monitorSystemHealth();

    /**
     * @brief Check for performance bottlenecks
     */
    void checkPerformanceBottlenecks();

    /**
     * @brief Optimize memory subsystem
     */
    void optimizeMemorySubsystem();

    /**
     * @brief Optimize task scheduling
     */
    void optimizeTaskScheduling();

    /**
     * @brief Optimize power management
     */
    void optimizePowerManagement();

    /**
     * @brief Handle performance alert
     * @param message Alert message
     * @param severity Alert severity (0.0-1.0)
     */
    void handlePerformanceAlert(const char* message, float severity);

    /**
     * @brief Calculate system health score
     * @return Health score (0.0-1.0)
     */
    float calculateSystemHealthScore() const;

    /**
     * @brief Get performance mode string
     * @param mode Performance mode
     * @return Mode string
     */
    const char* getPerformanceModeString(SystemPerformanceMode mode) const;

    // System component references
    MemoryManager* m_memoryManager = nullptr;
    TaskScheduler* m_taskScheduler = nullptr;
    PowerManager* m_powerManager = nullptr;
    PerformanceMonitor* m_performanceMonitor = nullptr;

    // Current state
    SystemPerformanceMode m_currentMode = SystemPerformanceMode::BALANCED;
    SystemPerformanceProfile m_currentProfile;
    SystemHealthStatus m_systemHealth = SystemHealthStatus::GOOD;

    // Auto-tuning
    bool m_autoTuningEnabled = true;
    uint32_t m_lastTuningTime = 0;
    uint32_t m_tuningIntervalMs = 10000; // 10 seconds

    // Performance tracking
    float m_performanceScore = 85.0f;
    uint32_t m_lastHealthCheck = 0;
    uint32_t m_healthCheckIntervalMs = 5000; // 5 seconds

    // Alert system
    void(*m_alertCallback)(const char*, float, void*) = nullptr;
    void* m_alertUserData = nullptr;

    // Performance history
    std::vector<float> m_performanceHistory;
    uint32_t m_lastPerformanceMeasurement = 0;

    // Emergency recovery state
    bool m_emergencyMode = false;
    uint32_t m_emergencyStartTime = 0;
    uint32_t m_emergencyTimeoutMs = 30000; // 30 seconds

    // Benchmark state
    struct BenchmarkResults {
        float cpuScore = 0.0f;
        float memoryScore = 0.0f;
        float displayScore = 0.0f;
        float audioScore = 0.0f;
        float overallScore = 0.0f;
        uint32_t timestamp = 0;
    };
    BenchmarkResults m_lastBenchmark;

    // Configuration
    bool m_initialized = false;
    uint32_t m_updateIntervalMs = 100; // 100ms update interval

    // Performance thresholds
    static constexpr float MIN_PERFORMANCE_SCORE = 60.0f;
    static constexpr float CRITICAL_PERFORMANCE_SCORE = 40.0f;
    static constexpr float TARGET_FPS = 60.0f;
    static constexpr float MIN_FPS = 55.0f;
    static constexpr float MAX_CPU_LOAD = 80.0f;
    static constexpr float MIN_FREE_MEMORY = 20.0f;
    static constexpr const char* TAG = "PerformanceIntegration";
};

// Global performance integration instance access
extern PerformanceIntegration* g_performanceIntegration;

// Convenience macros for performance optimization
#define PERF_OPTIMIZE_FOR_60HZ() \
    if (g_performanceIntegration) g_performanceIntegration->optimizeFor60Hz()

#define PERF_OPTIMIZE_FOR_BATTERY() \
    if (g_performanceIntegration) g_performanceIntegration->optimizeForBattery()

#define PERF_SET_MODE(mode) \
    if (g_performanceIntegration) g_performanceIntegration->setPerformanceMode(mode)

#define PERF_FORCE_GC() \
    if (g_performanceIntegration) g_performanceIntegration->forceGarbageCollection()

#define PERF_CHECK_60HZ() \
    (g_performanceIntegration ? g_performanceIntegration->isMaintaining60Hz() : false)

#define PERF_GET_SCORE() \
    (g_performanceIntegration ? g_performanceIntegration->getSystemPerformanceScore() : 0.0f)

#endif // PERFORMANCE_INTEGRATION_H