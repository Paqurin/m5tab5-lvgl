#ifndef OS_CONFIG_H
#define OS_CONFIG_H

#include <Arduino.h>

/**
 * @file os_config.h
 * @brief Core operating system configuration for M5Stack Tab5
 * 
 * This file contains system-wide configuration constants and settings
 * for the M5Tab5 ESP32-P4 operating system.
 */

// Hardware Configuration
#define OS_SCREEN_WIDTH         1280
#define OS_SCREEN_HEIGHT        720
#define OS_SCREEN_BPP           16
#define OS_PSRAM_ENABLED        1

// Memory Configuration
#define OS_MAX_APPS             8
#define OS_APP_STACK_SIZE       8192
#define OS_SYSTEM_HEAP_SIZE     (256 * 1024)  // 256KB for system
#define OS_APP_HEAP_SIZE        (512 * 1024)  // 512KB for apps
#define OS_BUFFER_POOL_SIZE     (128 * 1024)  // 128KB for buffers

// Task Configuration
#define OS_MAX_TASKS            16
#define OS_TASK_PRIORITY_HIGH   3
#define OS_TASK_PRIORITY_NORMAL 2
#define OS_TASK_PRIORITY_LOW    1
#define OS_TASK_PRIORITY_IDLE   0

// Event System Configuration
#define OS_MAX_EVENT_LISTENERS  32
#define OS_EVENT_QUEUE_SIZE     64

// Touch Configuration
#define OS_TOUCH_THRESHOLD      10
#define OS_TOUCH_DEBOUNCE_MS    50
#define OS_GESTURE_TIMEOUT_MS   500

// File System Configuration
#define OS_MAX_FILES_OPEN       8
#define OS_MAX_FILENAME_LEN     64
#define OS_STORAGE_MOUNT_POINT  "/storage"

// UI Configuration
#define OS_UI_REFRESH_RATE      30  // FPS
#define OS_UI_ANIMATION_TIME    200 // ms
#define OS_STATUS_BAR_HEIGHT    40
#define OS_DOCK_HEIGHT          60

// System Timing
#define OS_WATCHDOG_TIMEOUT_MS  30000
#define OS_IDLE_TIMEOUT_MS      300000  // 5 minutes
#define OS_SLEEP_CHECK_MS       1000

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
    OS_ERROR_PERMISSION = -10
} os_error_t;

// Forward declarations
class OSManager;
class AppManager;
class UIManager;
class TouchManager;
class EventSystem;

#endif // OS_CONFIG_H