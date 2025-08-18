#ifndef OS_CONFIG_H
#define OS_CONFIG_H

#include <Arduino.h>
#include "../hal/hardware_config.h"

/**
 * @file os_config.h
 * @brief Core operating system configuration for M5Stack Tab5
 * 
 * This file contains system-wide configuration constants and settings
 * for the M5Tab5 ESP32-P4 operating system.
 */

// Hardware Configuration (from hardware_config.h)
#define OS_SCREEN_WIDTH         DISPLAY_WIDTH
#define OS_SCREEN_HEIGHT        DISPLAY_HEIGHT
#define OS_SCREEN_BPP           DISPLAY_COLOR_DEPTH
#define OS_PSRAM_ENABLED        HW_HAS_PSRAM

// Memory Configuration - Optimized for 32MB PSRAM
#define OS_MAX_APPS             16
#define OS_APP_STACK_SIZE       32768  // Increased for complex apps
#define OS_SYSTEM_HEAP_SIZE     (3 * 1024 * 1024)   // 3MB for system
#define OS_APP_HEAP_SIZE        (6 * 1024 * 1024)   // 6MB for apps
#define OS_BUFFER_POOL_SIZE     (2 * 1024 * 1024)   // 2MB for buffers
#define OS_AUDIO_BUFFER_SIZE    (512 * 1024)        // 512KB for audio
#define OS_GRAPHICS_BUFFER_SIZE (4 * 1024 * 1024)   // 4MB for graphics

// PSRAM Configuration
#define OS_PSRAM_HEAP_SIZE      (16 * 1024 * 1024)  // 16MB PSRAM heap
#define OS_DISPLAY_BUFFER_SIZE  (4 * 1024 * 1024)   // 4MB for display buffers
#define OS_GRAPHICS_CACHE_SIZE  (2 * 1024 * 1024)   // 2MB graphics cache

// Task Configuration - Optimized for real-time performance
#define OS_MAX_TASKS            64        // Increased for complex apps
#define OS_TASK_PRIORITY_CRITICAL 4       // Critical real-time tasks
#define OS_TASK_PRIORITY_HIGH   3         // High priority tasks
#define OS_TASK_PRIORITY_NORMAL 2         // Normal priority tasks
#define OS_TASK_PRIORITY_LOW    1         // Low priority tasks
#define OS_TASK_PRIORITY_IDLE   0         // Idle tasks

// Real-time task configuration
#define OS_REALTIME_TASK_MAX_RUNTIME 5    // 5ms max for RT tasks
#define OS_FRAME_TIME_BUDGET_US  13000    // 13ms frame budget (60Hz)
#define OS_TASK_WATCHDOG_TIMEOUT 30000    // 30s watchdog timeout

// Event System Configuration - Increased for HD display
#define OS_MAX_EVENT_LISTENERS  64
#define OS_EVENT_QUEUE_SIZE     128

// Touch Configuration
#define OS_TOUCH_THRESHOLD      10
#define OS_TOUCH_DEBOUNCE_MS    50
#define OS_GESTURE_TIMEOUT_MS   500

// File System Configuration - Enhanced for media files
#define OS_MAX_FILES_OPEN       16
#define OS_MAX_FILENAME_LEN     256
#define OS_STORAGE_MOUNT_POINT  "/storage"
#define OS_LARGE_FILE_BUFFER    (512 * 1024)  // 512KB for large file operations

// UI Configuration - Optimized for 60Hz performance
#define OS_UI_REFRESH_RATE      60  // FPS - targeting 60Hz
#define OS_UI_ANIMATION_TIME    150 // ms - optimized for smooth 60Hz
#define OS_STATUS_BAR_HEIGHT    40
#define OS_DOCK_HEIGHT          60
#define OS_UI_DOUBLE_BUFFER     1   // Enable double buffering
#define OS_UI_VSYNC_ENABLED     1   // Enable VSync for smooth rendering
#define OS_UI_FRAME_BUDGET_MS   16  // 16ms frame budget for 60Hz

// System Timing - Performance optimized
#define OS_WATCHDOG_TIMEOUT_MS  30000
#define OS_IDLE_TIMEOUT_MS      300000  // 5 minutes
#define OS_SLEEP_CHECK_MS       1000
#define OS_PERFORMANCE_UPDATE_MS 100    // Performance monitoring interval
#define OS_MEMORY_GC_INTERVAL_MS 5000   // Garbage collection interval
#define OS_POWER_MONITOR_MS     1000    // Power monitoring interval

// Performance thresholds
#define OS_MIN_FPS_THRESHOLD    55.0f   // Minimum acceptable FPS
#define OS_MAX_CPU_LOAD         80.0f   // Maximum CPU load %
#define OS_MIN_FREE_MEMORY      20.0f   // Minimum free memory %
#define OS_MAX_FRAGMENTATION    30.0f   // Maximum fragmentation %

// Debug Configuration
#ifdef DEBUG
#define OS_DEBUG_ENABLED        1
#define OS_LOG_LEVEL            3  // 0=None, 1=Error, 2=Warn, 3=Info, 4=Debug
#else
#define OS_DEBUG_ENABLED        0
#define OS_LOG_LEVEL            1
#endif

// Version Information
#define OS_VERSION_MAJOR        1
#define OS_VERSION_MINOR        0
#define OS_VERSION_PATCH        0
#define OS_VERSION_STRING       "1.0.0"

// Error Codes
typedef enum {
    OS_OK = 0,
    OS_ERROR_GENERIC = -1,
    OS_ERROR_NO_MEMORY = -2,
    OS_ERROR_INVALID_PARAM = -3,
    OS_ERROR_NOT_FOUND = -4,
    OS_ERROR_TIMEOUT = -5,
    OS_ERROR_BUSY = -6,
    OS_ERROR_NOT_SUPPORTED = -7,
    OS_ERROR_HARDWARE = -8,
    OS_ERROR_FILESYSTEM = -9,
    OS_ERROR_PERMISSION = -10,
    OS_ERROR_NOT_AVAILABLE = -11,
    OS_ERROR_NOT_IMPLEMENTED = -12,
    OS_ERROR_NOT_INITIALIZED = -13,
    OS_ERROR_NOT_HANDLED = -14,
    OS_ERROR_OPERATION_NOT_PERMITTED = -15,
    OS_ERROR_NETWORK_UNREACHABLE = -16,
    OS_ERROR_ALREADY_EXISTS = -17,
    OS_ERROR_INVALID_STATE = -18,
    OS_ERROR_INVALID_DATA = -19,
    OS_ERROR_STORAGE = -20
} os_error_t;

// Input Event Types
typedef enum {
    INPUT_EVENT_TOUCH_DOWN,
    INPUT_EVENT_TOUCH_UP,
    INPUT_EVENT_TOUCH_MOVE,
    INPUT_EVENT_KEY_DOWN,
    INPUT_EVENT_KEY_UP,
    INPUT_EVENT_GESTURE,
    INPUT_EVENT_ACCESSIBILITY_SHORTCUT,
    // Multi-touch UI navigation events
    EVENT_UI_HOME_BUTTON,
    EVENT_UI_TASK_SWITCHER,
    EVENT_UI_CONTROL_CENTER,
    EVENT_UI_ZOOM_IN,
    EVENT_UI_ZOOM_OUT,
    EVENT_UI_NAVIGATE_BACK,
    EVENT_UI_NAVIGATE_FORWARD,
    EVENT_UI_NEXT_APP,
    EVENT_UI_PREV_APP,
    EVENT_UI_MINIMIZE_ALL,
    // Touch events for backwards compatibility
    EVENT_UI_TOUCH_PRESS = INPUT_EVENT_TOUCH_DOWN,
    EVENT_UI_TOUCH_RELEASE = INPUT_EVENT_TOUCH_UP,
    EVENT_UI_TOUCH_MOVE = INPUT_EVENT_TOUCH_MOVE
} input_event_type_t;

// Input Event Structure
typedef struct {
    input_event_type_t type;
    uint16_t x, y;              // Touch coordinates
    uint16_t key;               // Key code for keyboard events
    uint32_t timestamp;         // Event timestamp
    void* data;                 // Additional event data
} input_event_t;

// Forward declarations
class OSManager;
class AppManager;
class UIManager;
class TouchManager;
class EventSystem;
class PerformanceMonitor;
class MemoryManager;
class TaskScheduler;
class PowerManager;

// Performance monitoring integration
#define OS_ENABLE_PERFORMANCE_MONITORING 1
#define OS_ENABLE_FRAME_TIMING          1
#define OS_ENABLE_MEMORY_TRACKING       1
#define OS_ENABLE_TASK_PROFILING        1
#define OS_ENABLE_POWER_MONITORING      1

// Audio performance configuration
#define OS_AUDIO_SAMPLE_RATE            44100
#define OS_AUDIO_BUFFER_SAMPLES         1024
#define OS_AUDIO_CHANNELS               2
#define OS_AUDIO_BIT_DEPTH              16
#define OS_AUDIO_REALTIME_PRIORITY      1

// Display performance configuration
#define OS_DISPLAY_BUFFER_COUNT         2    // Double buffering
#define OS_DISPLAY_DMA_ENABLED          1    // DMA acceleration
#define OS_DISPLAY_CACHE_ENABLED        1    // Display caching
#define OS_DISPLAY_VSYNC_TIMEOUT        20   // VSync timeout ms

// ESP32-P4 PPA (Pixel Processing Accelerator) Configuration
#define CONFIG_ESP_PPA_ACCELERATION         1       // Enable PPA hardware acceleration
#define CONFIG_ESP_PPA_LVGL_INTEGRATION     1       // Enable LVGL integration
#define CONFIG_ESP_PPA_PERFORMANCE_MODE     1       // Optimize for performance vs memory
#define CONFIG_ESP_PPA_PSRAM_BUFFERS        1       // Use PSRAM for large buffers
#define HW_HAS_PPA                          1       // Pixel Processing Accelerator available

#endif // OS_CONFIG_H