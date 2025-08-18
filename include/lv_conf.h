#ifndef LV_CONF_H
#define LV_CONF_H

#define LV_COLOR_DEPTH 16
#define LV_COLOR_16_SWAP 0
#define LV_MEM_SIZE (512 * 1024U)  // Increased to 512KB for better performance
#define LV_USE_PERF_MONITOR 1
#define LV_USE_MEM_MONITOR 1
#define LV_USE_LOG 1
#define LV_LOG_LEVEL LV_LOG_LEVEL_INFO

// Font configuration
#define LV_FONT_MONTSERRAT_12 1
#define LV_FONT_MONTSERRAT_14 1
#define LV_FONT_MONTSERRAT_16 1
#define LV_FONT_MONTSERRAT_18 1
#define LV_FONT_MONTSERRAT_20 1
#define LV_FONT_MONTSERRAT_22 1
#define LV_FONT_MONTSERRAT_24 1
#define LV_FONT_MONTSERRAT_26 1
#define LV_FONT_MONTSERRAT_28 1
#define LV_FONT_MONTSERRAT_32 1
#define LV_FONT_MONTSERRAT_36 1
#define LV_FONT_MONTSERRAT_40 1
#define LV_FONT_MONTSERRAT_48 1

// Theme configuration
#define LV_USE_THEME_DEFAULT 1
#define LV_USE_THEME_MATERIAL 1

// Widget configuration - Enable all widgets used by apps
#define LV_USE_TEXTAREA 1
#define LV_USE_KEYBOARD 1
#define LV_USE_BTNMATRIX 1
#define LV_USE_BTN 1
#define LV_USE_LABEL 1
#define LV_USE_LIST 1
#define LV_USE_DROPDOWN 1
#define LV_USE_SLIDER 1
#define LV_USE_SWITCH 1
#define LV_USE_SPINBOX 1
#define LV_USE_CHECKBOX 1
#define LV_USE_TABVIEW 1
#define LV_USE_CALENDAR 1
#define LV_USE_MSGBOX 1
#define LV_USE_SPINNER 1
#define LV_USE_BAR 1
#define LV_USE_IMG 1
#define LV_USE_CANVAS 1
#define LV_USE_METER 1
#define LV_USE_WIN 1
#define LV_USE_MENU 1

// Layout configuration
#define LV_USE_FLEX 1
#define LV_USE_GRID 1

// Animation configuration
#define LV_USE_ANIM 1

// File system support
#define LV_USE_FS_STDIO 1
#define LV_USE_FS_POSIX 1
#define LV_FS_POSIX_LETTER 'P'
#define LV_FS_POSIX_PATH "/mnt"

// Image decoder support
#define LV_USE_PNG 1
#define LV_USE_BMP 1
#define LV_USE_SJPG 1

// Enable features for terminal and file management
#define LV_TEXTAREA_DEF_SCROLLBAR_MODE LV_SCROLLBAR_MODE_AUTO
#define LV_USE_SCROLLBAR 1

// Voice navigation support
#define LV_KEY_DEF LV_KEY_NEXT, LV_KEY_PREV, LV_KEY_ENTER, LV_KEY_ESC
#define LV_GROUP_DEF_SIZE 16

// Enhanced focus management for screen readers
#define LV_USE_GROUP 1
#define LV_USE_INDEV 1

// Color configuration for RGB565
#define LV_COLOR_MIX_ROUND_OFS 128
#define LV_COLOR_CHROMA_KEY lv_color_hex(0x00ff00)

// Memory configuration optimized for ESP32-P4 with PSRAM
#define LV_MEM_CUSTOM 1
#define LV_MEM_CUSTOM_INCLUDE <stdlib.h>
#define LV_MEM_CUSTOM_ALLOC malloc
#define LV_MEM_CUSTOM_FREE free
#define LV_MEM_CUSTOM_REALLOC realloc

// GPU configuration (disable STM32 DMA2D for ESP32-P4)
#define LV_USE_GPU_STM32_DMA2D 0

// Tick configuration
#define LV_TICK_CUSTOM 1
#define LV_TICK_CUSTOM_INCLUDE "freertos/FreeRTOS.h"
#define LV_TICK_CUSTOM_SYS_TIME_EXPR (xTaskGetTickCount() * portTICK_PERIOD_MS)

// Symbol configuration - Enable symbols used in apps
#define LV_USE_SYMBOL_AUDIO "\xEF\x80\x81"
#define LV_USE_SYMBOL_VIDEO "\xEF\x80\x88"
#define LV_USE_SYMBOL_LIST "\xEF\x80\x8B"
#define LV_USE_SYMBOL_OK "\xEF\x80\x8C"
#define LV_USE_SYMBOL_CLOSE "\xEF\x80\x8D"

// Accessibility configuration for Talkback Voice System
// LV_USE_ASSERT_STYLE defined later for performance optimization
#define LV_USE_USER_DATA 1
#define LV_USE_FLEX 1
#define LV_USE_GRID 1

// Enhanced event handling for voice feedback
#define LV_EVENT_BUBBLE 1
#define LV_EVENT_DEFOCUS 1

// Screen reader support
#define LV_FONT_CUSTOM_DECLARE LV_FONT_DECLARE(lv_font_montserrat_12) \
                              LV_FONT_DECLARE(lv_font_montserrat_16) \
                              LV_FONT_DECLARE(lv_font_montserrat_20)

// Talkback Voice System integration
#define LV_TALKBACK_VOICE_ENABLED 1
#define LV_ACCESSIBILITY_ENABLED 1
#define LV_USE_SYMBOL_POWER "\xEF\x80\x91"
#define LV_USE_SYMBOL_SETTINGS "\xEF\x80\x93"
#define LV_USE_SYMBOL_HOME "\xEF\x80\x95"
#define LV_USE_SYMBOL_DOWNLOAD "\xEF\x80\x99"
#define LV_USE_SYMBOL_DRIVE "\xEF\x80\x9C"
#define LV_USE_SYMBOL_REFRESH "\xEF\x80\xA1"
#define LV_USE_SYMBOL_MUTE "\xEF\x80\xA6"
#define LV_USE_SYMBOL_VOLUME_MID "\xEF\x80\xA7"
#define LV_USE_SYMBOL_VOLUME_MAX "\xEF\x80\xA8"
#define LV_USE_SYMBOL_IMAGE "\xEF\x80\xBE"
#define LV_USE_SYMBOL_EDIT "\xEF\x8C\x84"
#define LV_USE_SYMBOL_PREV "\xEF\x81\x88"
#define LV_USE_SYMBOL_PLAY "\xEF\x81\x8B"
#define LV_USE_SYMBOL_PAUSE "\xEF\x81\x8C"
#define LV_USE_SYMBOL_STOP "\xEF\x81\x8D"
#define LV_USE_SYMBOL_NEXT "\xEF\x81\x91"
#define LV_USE_SYMBOL_EJECT "\xEF\x81\x92"
#define LV_USE_SYMBOL_LEFT "\xEF\x81\x93"
#define LV_USE_SYMBOL_RIGHT "\xEF\x81\x94"
#define LV_USE_SYMBOL_PLUS "\xEF\x81\xA7"
#define LV_USE_SYMBOL_MINUS "\xEF\x81\xA8"
#define LV_USE_SYMBOL_EYE_OPEN "\xEF\x81\xAE"
#define LV_USE_SYMBOL_EYE_CLOSE "\xEF\x81\xB0"
#define LV_USE_SYMBOL_WARNING "\xEF\x81\xB1"
#define LV_USE_SYMBOL_SHUFFLE "\xEF\x81\xB4"
#define LV_USE_SYMBOL_UP "\xEF\x81\xB7"
#define LV_USE_SYMBOL_DOWN "\xEF\x81\xB8"
#define LV_USE_SYMBOL_LOOP "\xEF\x81\xB9"
#define LV_USE_SYMBOL_DIRECTORY "\xEF\x81\xBB"
#define LV_USE_SYMBOL_UPLOAD "\xEF\x82\x93"
#define LV_USE_SYMBOL_CALL "\xEF\x82\x95"
#define LV_USE_SYMBOL_CUT "\xEF\x83\x84"
#define LV_USE_SYMBOL_COPY "\xEF\x83\x85"
#define LV_USE_SYMBOL_SAVE "\xEF\x83\x87"
#define LV_USE_SYMBOL_CHARGE "\xEF\x83\xA7"
#define LV_USE_SYMBOL_PASTE "\xEF\x83\xAA"
#define LV_USE_SYMBOL_BELL "\xEF\x83\xB3"
#define LV_USE_SYMBOL_KEYBOARD "\xEF\x84\x9C"
#define LV_USE_SYMBOL_GPS "\xEF\x84\xA4"
#define LV_USE_SYMBOL_FILE "\xEF\x85\x9B"
#define LV_USE_SYMBOL_WIFI "\xEF\x87\xAB"
#define LV_USE_SYMBOL_BATTERY_FULL "\xEF\x89\x80"
#define LV_USE_SYMBOL_BATTERY_3 "\xEF\x89\x81"
#define LV_USE_SYMBOL_BATTERY_2 "\xEF\x89\x82"
#define LV_USE_SYMBOL_BATTERY_1 "\xEF\x89\x83"
#define LV_USE_SYMBOL_BATTERY_EMPTY "\xEF\x89\x84"
#define LV_USE_SYMBOL_USB "\xEF\x8A\x87"
#define LV_USE_SYMBOL_BLUETOOTH "\xEF\x8A\x94"
#define LV_USE_SYMBOL_TRASH "\xEF\x8B\xAD"
#define LV_USE_SYMBOL_BACKSPACE "\xEF\x95\x9A"
#define LV_USE_SYMBOL_SD_CARD "\xEF\x9F\x82"
#define LV_USE_SYMBOL_NEW_LINE "\xEA\x9C\xB2"

// Performance optimizations for ESP32-P4 - 60Hz Display
#define LV_DISP_DEF_REFR_PERIOD 16  // 60 FPS refresh (16.67ms)
#ifndef LV_INDEV_DEF_READ_PERIOD
#define LV_INDEV_DEF_READ_PERIOD 10  // Input reading period (defined in platformio.ini as 10)
#endif

// Advanced performance optimizations
#define LV_MEM_BUF_MAX_NUM 16
#define LV_MEMCPY_MEMSET_STD 1
#define LV_USE_GPU 1
#define LV_GPU_DMA2D 0
#define LV_ATTRIBUTE_FAST_MEM IRAM_ATTR
#define LV_ATTRIBUTE_DMA DRAM_ATTR

// Rendering optimizations
#define LV_USE_DRAW_SW 1
#define LV_DRAW_SW_ASM LV_DRAW_SW_ASM_NEON
#define LV_USE_DRAW_MASKS 1
#define LV_DRAW_COMPLEX 1
#define LV_SHADOW_CACHE_SIZE 0  // Disable shadow cache for performance
#define LV_IMG_CACHE_DEF_SIZE 4  // Reduce image cache size
#define LV_GRADIENT_MAX_STOPS 4  // Limit gradient complexity

// Task and timing optimizations
#define LV_USE_OS LV_OS_FREERTOS
#define LV_OS_CUSTOM_TICK 1
#define LV_TICK_CUSTOM_INCLUDE "freertos/FreeRTOS.h"
#define LV_TICK_CUSTOM_SYS_TIME_EXPR (xTaskGetTickCount() * portTICK_PERIOD_MS)

// Double buffering and performance optimizations
#define LV_USE_DRAW_MASKS 1
#define LV_DRAW_COMPLEX 1
#define LV_USE_LARGE_COORD 1

// Animation optimizations for smooth 60Hz
#define LV_ANIM_DEFAULT_TIME 150
#define LV_ANIM_PATH_LINEAR 1
#define LV_ANIM_PATH_EASE_IN 1
#define LV_ANIM_PATH_EASE_OUT 1
#define LV_ANIM_PATH_EASE_IN_OUT 1
#define LV_ANIM_PATH_OVERSHOOT 1
#define LV_ANIM_PATH_BOUNCE 1

// Buffer size optimization for 60Hz refresh
#define LV_DISP_DRAW_BUF_SIZE (OS_SCREEN_WIDTH * 40)  // 40 lines for smooth rendering

// Memory allocation optimization
#define LV_MEM_POOL_INCLUDE <stdlib.h>
#define LV_MEM_POOL_ALLOC heap_caps_malloc(size, MALLOC_CAP_SPIRAM)
#define LV_MEM_POOL_FREE heap_caps_free
#define LV_MEM_POOL_EXPAND_SIZE_LIMIT (32 * 1024)

// Advanced caching and optimization
#define LV_USE_SNAPSHOT 1
#define LV_SNAPSHOT_DEF_SIZE (OS_SCREEN_WIDTH * OS_SCREEN_HEIGHT / 8)
#define LV_USE_MONKEY 0  // Disable monkey test for performance

// Performance monitoring for 60Hz validation
#define LV_USE_REFR_DEBUG 0  // Disable in production for performance
#define LV_USE_ASSERT_STYLE 0  // Disable style assertions for performance

// Font optimization
#define LV_FONT_MONTSERRAT_8 0   // Disable unused small fonts
#define LV_FONT_MONTSERRAT_10 0
#define LV_FONT_DEJAVU_16_PERSIAN_HEBREW 0
#define LV_FONT_SIMSUN_16_CJK 0
#define LV_FONT_UNSCII_8 0
#define LV_FONT_UNSCII_16 0

// Widget optimization - disable unused widgets
#define LV_USE_ARC 1
#define LV_USE_ANIMIMG 0
#define LV_USE_CHART 1
#define LV_USE_COLORWHEEL 0
#define LV_USE_IMGBTN 0
#define LV_USE_LED 0
#define LV_USE_LINE 0
#define LV_USE_ROLLER 0
#define LV_USE_TABLE 1
#define LV_USE_TILEVIEW 0

// Animation optimizations
#define LV_ANIM_BUILTIN_TIME_FUNC 1
#define LV_ANIM_CUSTOM_EXEC 0

// Logging configuration
#define LV_LOG_PRINTF 1

#endif