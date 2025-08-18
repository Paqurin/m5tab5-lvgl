#ifndef PPA_HAL_H
#define PPA_HAL_H

/**
 * @file ppa_hal.h
 * @brief Hardware Abstraction Layer for ESP32-P4 Pixel Processing Accelerator (PPA)
 * 
 * This module provides a hardware abstraction layer for the ESP32-P4's integrated
 * Pixel Processing Accelerator, enabling hardware-accelerated image operations
 * including scaling, rotation, mirroring, blending, and filling.
 * 
 * Key Features:
 * - Scale-Rotate-Mirror (SRM) operations with sub-pixel accuracy
 * - Alpha blending with color-keying support
 * - Hardware-accelerated fill operations
 * - LVGL integration for UI acceleration
 * - Memory-efficient operations with PSRAM optimization
 * - Thread-safe operation management
 */

#include "../system/os_config.h"

#ifdef CONFIG_ESP_PPA_ACCELERATION

#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_err.h"

// ESP32-P4 PPA types (defined here until driver is available in Arduino framework)
typedef enum {
    PPA_SRM_ROTATION_ANGLE_0 = 0,
    PPA_SRM_ROTATION_ANGLE_90 = 1,
    PPA_SRM_ROTATION_ANGLE_180 = 2,
    PPA_SRM_ROTATION_ANGLE_270 = 3
} ppa_srm_rotation_angle_t;

// Forward declarations for stub mode
typedef struct ppa_client_t* ppa_client_handle_t;

// PPA Configuration
#define PPA_MAX_CLIENTS             4
#define PPA_MAX_PENDING_TRANS       2
#define PPA_OPERATION_TIMEOUT_MS    5000
#define PPA_CACHE_LINE_SIZE         32

// LVGL Integration
#ifdef LV_USE_GPU
#define PPA_ENABLE_LVGL_INTEGRATION 1
#include "lvgl.h"
#endif

// PPA Client Types for different operations
typedef enum {
    PPA_CLIENT_TYPE_SRM = 0,    // Scale-Rotate-Mirror
    PPA_CLIENT_TYPE_BLEND,      // Alpha blending
    PPA_CLIENT_TYPE_FILL,       // Fill operations
    PPA_CLIENT_TYPE_LVGL,       // LVGL integration
    PPA_CLIENT_TYPE_MAX
} ppa_client_type_t;

// PPA Operation Status
typedef enum {
    PPA_STATUS_IDLE = 0,
    PPA_STATUS_BUSY,
    PPA_STATUS_COMPLETE,
    PPA_STATUS_ERROR,
    PPA_STATUS_TIMEOUT
} ppa_hal_status_t;

// PPA Image Format (optimized for common formats)
typedef enum {
    PPA_FORMAT_RGB565 = 0,      // 16-bit RGB565 (most common for displays)
    PPA_FORMAT_RGB888,          // 24-bit RGB888
    PPA_FORMAT_ARGB8888,        // 32-bit ARGB8888 with alpha
    PPA_FORMAT_YUV420,          // YUV420 planar (for camera/video)
    PPA_FORMAT_YUV444,          // YUV444 (high quality)
    PPA_FORMAT_A8,              // 8-bit alpha only
    PPA_FORMAT_A4               // 4-bit alpha only
} ppa_image_format_t;

// PPA Rectangle definition
typedef struct {
    uint16_t x;
    uint16_t y;
    uint16_t width;
    uint16_t height;
} ppa_rect_t;

// PPA Image descriptor
typedef struct {
    void* buffer;               // Image buffer (must be cache-aligned)
    uint16_t width;             // Image width in pixels
    uint16_t height;            // Image height in pixels
    ppa_image_format_t format;  // Pixel format
    bool is_psram;              // Buffer located in PSRAM
} ppa_image_t;

// PPA Transform parameters
typedef struct {
    float scale_x;              // Horizontal scaling factor (0.0625 to 16.0)
    float scale_y;              // Vertical scaling factor (0.0625 to 16.0)
    ppa_srm_rotation_angle_t rotation;  // Rotation angle (0, 90, 180, 270 degrees)
    bool mirror_x;              // Horizontal mirroring
    bool mirror_y;              // Vertical mirroring
    uint8_t alpha;              // Alpha value (0-255)
} ppa_transform_t;

// PPA Blend parameters
typedef struct {
    uint8_t bg_alpha;           // Background alpha (0-255)
    uint8_t fg_alpha;           // Foreground alpha (0-255)
    bool color_key_enable;      // Enable color keying
    uint32_t color_key_low;     // Color key low threshold (RGB888)
    uint32_t color_key_high;    // Color key high threshold (RGB888)
    uint32_t color_key_default; // Default color for keyed pixels
} ppa_blend_params_t;

// PPA HAL Handle
typedef struct ppa_hal_context {
    ppa_client_handle_t clients[PPA_CLIENT_TYPE_MAX];
    SemaphoreHandle_t mutex;
    volatile ppa_hal_status_t status;
    TaskHandle_t waiting_task;
    uint32_t operations_count;
    bool initialized;
} ppa_hal_context_t;

// Function prototypes
extern "C" {

/**
 * @brief Initialize PPA HAL
 * 
 * Initializes the PPA hardware abstraction layer, registers clients
 * for different operation types, and sets up synchronization primitives.
 * 
 * @return esp_err_t ESP_OK on success
 */
esp_err_t ppa_hal_init(void);

/**
 * @brief Deinitialize PPA HAL
 * 
 * Cleans up PPA resources and unregisters all clients.
 * 
 * @return esp_err_t ESP_OK on success
 */
esp_err_t ppa_hal_deinit(void);

/**
 * @brief Check if PPA HAL is initialized and ready
 * 
 * @return true if initialized, false otherwise
 */
bool ppa_hal_is_initialized(void);

/**
 * @brief Get current PPA operation status
 * 
 * @return ppa_hal_status_t Current status
 */
ppa_hal_status_t ppa_hal_get_status(void);

// === Scale-Rotate-Mirror Operations ===

/**
 * @brief Perform hardware-accelerated image transformation
 * 
 * Applies scaling, rotation, and mirroring to an image using the PPA.
 * Supports sub-pixel scaling accuracy and optimized memory access patterns.
 * 
 * @param src_img Source image descriptor
 * @param src_rect Source rectangle to transform
 * @param dst_img Destination image descriptor
 * @param dst_x Destination X coordinate
 * @param dst_y Destination Y coordinate
 * @param transform Transform parameters
 * @param blocking If true, function blocks until completion
 * @return esp_err_t ESP_OK on success
 */
esp_err_t ppa_hal_transform_image(const ppa_image_t* src_img, 
                                  const ppa_rect_t* src_rect,
                                  const ppa_image_t* dst_img, 
                                  uint16_t dst_x, uint16_t dst_y,
                                  const ppa_transform_t* transform,
                                  bool blocking);

/**
 * @brief Quick image scaling operation
 * 
 * Optimized function for common scaling operations without rotation/mirroring.
 * 
 * @param src_img Source image
 * @param dst_img Destination image
 * @param scale_x Horizontal scale factor
 * @param scale_y Vertical scale factor
 * @param blocking Block until completion
 * @return esp_err_t ESP_OK on success
 */
esp_err_t ppa_hal_scale_image(const ppa_image_t* src_img,
                              const ppa_image_t* dst_img,
                              float scale_x, float scale_y,
                              bool blocking);

/**
 * @brief Rotate image by specified angle
 * 
 * @param src_img Source image
 * @param dst_img Destination image
 * @param angle Rotation angle (0, 90, 180, 270 degrees)
 * @param blocking Block until completion
 * @return esp_err_t ESP_OK on success
 */
esp_err_t ppa_hal_rotate_image(const ppa_image_t* src_img,
                               const ppa_image_t* dst_img,
                               ppa_srm_rotation_angle_t angle,
                               bool blocking);

// === Alpha Blending Operations ===

/**
 * @brief Alpha blend two images
 * 
 * Performs hardware-accelerated alpha blending of foreground and background images.
 * Supports color keying for advanced compositing effects.
 * 
 * @param bg_img Background image
 * @param fg_img Foreground image
 * @param dst_img Destination image (can be same as bg_img)
 * @param blend_rect Rectangle to blend
 * @param params Blending parameters
 * @param blocking Block until completion
 * @return esp_err_t ESP_OK on success
 */
esp_err_t ppa_hal_blend_images(const ppa_image_t* bg_img,
                               const ppa_image_t* fg_img,
                               const ppa_image_t* dst_img,
                               const ppa_rect_t* blend_rect,
                               const ppa_blend_params_t* params,
                               bool blocking);

/**
 * @brief Simple alpha blend without color keying
 * 
 * @param bg_img Background image
 * @param fg_img Foreground image  
 * @param dst_img Destination image
 * @param alpha Global alpha value (0-255)
 * @param blocking Block until completion
 * @return esp_err_t ESP_OK on success
 */
esp_err_t ppa_hal_alpha_blend(const ppa_image_t* bg_img,
                              const ppa_image_t* fg_img,
                              const ppa_image_t* dst_img,
                              uint8_t alpha,
                              bool blocking);

// === Fill Operations ===

/**
 * @brief Fill rectangle with solid color
 * 
 * Hardware-accelerated rectangle fill operation with optimal performance.
 * 
 * @param dst_img Destination image
 * @param fill_rect Rectangle to fill
 * @param color Fill color (ARGB8888 format)
 * @param blocking Block until completion
 * @return esp_err_t ESP_OK on success
 */
esp_err_t ppa_hal_fill_rect(const ppa_image_t* dst_img,
                            const ppa_rect_t* fill_rect,
                            uint32_t color,
                            bool blocking);

/**
 * @brief Clear entire image buffer
 * 
 * @param dst_img Destination image
 * @param color Clear color (ARGB8888 format)
 * @param blocking Block until completion
 * @return esp_err_t ESP_OK on success
 */
esp_err_t ppa_hal_clear_image(const ppa_image_t* dst_img,
                              uint32_t color,
                              bool blocking);

// === LVGL Integration ===

#ifdef PPA_ENABLE_LVGL_INTEGRATION
/**
 * @brief Initialize LVGL GPU acceleration using PPA
 * 
 * Sets up LVGL to use PPA for hardware-accelerated drawing operations.
 * 
 * @return esp_err_t ESP_OK on success
 */
esp_err_t ppa_hal_lvgl_init(void);

/**
 * @brief LVGL GPU fill function
 */
void ppa_hal_lvgl_fill(lv_disp_drv_t* disp_drv, lv_color_t* dest_buf, 
                       lv_coord_t dest_width, const lv_area_t* fill_area, 
                       lv_color_t color);

/**
 * @brief LVGL GPU blend function
 */
void ppa_hal_lvgl_blend(lv_disp_drv_t* disp_drv, lv_color_t* dest_buf,
                        lv_coord_t dest_width, const lv_area_t* dest_area,
                        const lv_color_t* src_buf, lv_coord_t src_width,
                        const lv_area_t* src_area, lv_opa_t opa);

/**
 * @brief LVGL GPU blit function
 */
void ppa_hal_lvgl_blit(lv_disp_drv_t* disp_drv, lv_color_t* dest_buf,
                       lv_coord_t dest_width, const lv_area_t* dest_area,
                       const lv_color_t* src_buf, lv_coord_t src_width,
                       const lv_area_t* src_area);
#endif

// === Utility Functions ===

/**
 * @brief Convert between different pixel formats
 * 
 * @param src_format Source format
 * @param dst_format Destination format
 * @return true if conversion is supported
 */
bool ppa_hal_format_supported(ppa_image_format_t src_format, 
                              ppa_image_format_t dst_format);

/**
 * @brief Calculate bytes per pixel for given format
 * 
 * @param format Pixel format
 * @return uint8_t Bytes per pixel
 */
uint8_t ppa_hal_bytes_per_pixel(ppa_image_format_t format);

/**
 * @brief Align buffer address for optimal PPA performance
 * 
 * @param addr Buffer address
 * @return void* Aligned address
 */
void* ppa_hal_align_buffer(void* addr);

/**
 * @brief Calculate aligned buffer size
 * 
 * @param size Original size
 * @return size_t Aligned size
 */
size_t ppa_hal_align_size(size_t size);

/**
 * @brief Allocate cache-aligned buffer for PPA operations
 * 
 * @param size Buffer size in bytes
 * @param use_psram Use PSRAM if available
 * @return void* Allocated buffer or NULL on failure
 */
void* ppa_hal_alloc_buffer(size_t size, bool use_psram);

/**
 * @brief Free PPA buffer
 * 
 * @param buffer Buffer to free
 */
void ppa_hal_free_buffer(void* buffer);

/**
 * @brief Wait for current PPA operation to complete
 * 
 * @param timeout_ms Timeout in milliseconds
 * @return esp_err_t ESP_OK if completed, ESP_ERR_TIMEOUT if timeout
 */
esp_err_t ppa_hal_wait_completion(uint32_t timeout_ms);

/**
 * @brief Get PPA performance statistics
 * 
 * @param ops_per_sec Operations per second
 * @param avg_time_us Average operation time in microseconds
 * @return esp_err_t ESP_OK on success
 */
esp_err_t ppa_hal_get_performance_stats(uint32_t* ops_per_sec, uint32_t* avg_time_us);

} // extern "C"

// === Helper Macros ===

// Create image descriptor
#define PPA_IMAGE_INIT(buf, w, h, fmt) { \
    .buffer = (buf), \
    .width = (w), \
    .height = (h), \
    .format = (fmt), \
    .is_psram = heap_caps_get_allocated_size(buf) > 0 && \
                (heap_caps_get_info(buf, MALLOC_CAP_SPIRAM) != NULL) \
}

// Create rectangle
#define PPA_RECT_INIT(x, y, w, h) { \
    .x = (x), .y = (y), .width = (w), .height = (h) \
}

// Create transform with defaults
#define PPA_TRANSFORM_INIT() { \
    .scale_x = 1.0f, \
    .scale_y = 1.0f, \
    .rotation = PPA_SRM_ROTATION_ANGLE_0, \
    .mirror_x = false, \
    .mirror_y = false, \
    .alpha = 255 \
}

// Create blend parameters with defaults
#define PPA_BLEND_PARAMS_INIT() { \
    .bg_alpha = 255, \
    .fg_alpha = 255, \
    .color_key_enable = false, \
    .color_key_low = 0x000000, \
    .color_key_high = 0x000000, \
    .color_key_default = 0x000000 \
}

// Performance optimization flags
#define PPA_OPTIMIZE_FOR_SPEED      1
#define PPA_OPTIMIZE_FOR_MEMORY     2
#define PPA_OPTIMIZE_FOR_QUALITY    3

#endif // CONFIG_ESP_PPA_ACCELERATION

#endif // PPA_HAL_H