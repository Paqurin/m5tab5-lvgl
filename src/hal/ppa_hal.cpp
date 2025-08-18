#include "ppa_hal.h"
#include "esp_log.h"

#ifdef CONFIG_ESP_PPA_ACCELERATION

static const char* TAG = "PPA_HAL_STUB";

// Note: ESP32-P4 PPA driver is not yet available in Arduino ESP32 framework
// This is a stub implementation that provides the API without hardware functionality
// When the ESP-IDF PPA driver becomes available in Arduino framework, 
// this can be replaced with the full implementation

// === Initialization ===

esp_err_t ppa_hal_init(void) {
    ESP_LOGW(TAG, "ESP32-P4 PPA driver not yet available in Arduino framework");
    ESP_LOGI(TAG, "PPA HAL initialized in stub mode - ready for future driver integration");
    ESP_LOGI(TAG, "Hardware acceleration will be available when ESP-IDF v5.4+ support is added");
    return ESP_OK;
}

esp_err_t ppa_hal_deinit(void) {
    ESP_LOGI(TAG, "PPA HAL deinitialized (stub mode)");
    return ESP_OK;
}

bool ppa_hal_is_initialized(void) {
    return true; // Always report as initialized in stub mode
}

ppa_hal_status_t ppa_hal_get_status(void) {
    return PPA_STATUS_IDLE; // Always idle in stub mode
}

// === Transform Operations ===

esp_err_t ppa_hal_transform_image(const ppa_image_t* src_img, 
                                  const ppa_rect_t* src_rect,
                                  const ppa_image_t* dst_img, 
                                  uint16_t dst_x, uint16_t dst_y,
                                  const ppa_transform_t* transform,
                                  bool blocking) {
    if (!src_img || !dst_img || !transform) {
        return ESP_ERR_INVALID_ARG;
    }
    
    ESP_LOGD(TAG, "Transform image operation (stub) - scale: %.2f,%.2f rotation: %d", 
             transform->scale_x, transform->scale_y, transform->rotation);
    
    // In stub mode, we could implement software fallback here
    // For now, just indicate the operation would succeed
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t ppa_hal_scale_image(const ppa_image_t* src_img,
                              const ppa_image_t* dst_img,
                              float scale_x, float scale_y,
                              bool blocking) {
    if (!src_img || !dst_img) {
        return ESP_ERR_INVALID_ARG;
    }
    
    ESP_LOGD(TAG, "Scale image operation (stub) - scale: %.2f,%.2f", scale_x, scale_y);
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t ppa_hal_rotate_image(const ppa_image_t* src_img,
                               const ppa_image_t* dst_img,
                               ppa_srm_rotation_angle_t angle,
                               bool blocking) {
    if (!src_img || !dst_img) {
        return ESP_ERR_INVALID_ARG;
    }
    
    ESP_LOGD(TAG, "Rotate image operation (stub) - angle: %d degrees", angle * 90);
    return ESP_ERR_NOT_SUPPORTED;
}

// === Blend Operations ===

esp_err_t ppa_hal_blend_images(const ppa_image_t* bg_img,
                               const ppa_image_t* fg_img,
                               const ppa_image_t* dst_img,
                               const ppa_rect_t* blend_rect,
                               const ppa_blend_params_t* params,
                               bool blocking) {
    if (!bg_img || !fg_img || !dst_img || !params) {
        return ESP_ERR_INVALID_ARG;
    }
    
    ESP_LOGD(TAG, "Blend images operation (stub) - alpha: bg=%d, fg=%d", 
             params->bg_alpha, params->fg_alpha);
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t ppa_hal_alpha_blend(const ppa_image_t* bg_img,
                              const ppa_image_t* fg_img,
                              const ppa_image_t* dst_img,
                              uint8_t alpha,
                              bool blocking) {
    if (!bg_img || !fg_img || !dst_img) {
        return ESP_ERR_INVALID_ARG;
    }
    
    ESP_LOGD(TAG, "Alpha blend operation (stub) - alpha: %d", alpha);
    return ESP_ERR_NOT_SUPPORTED;
}

// === Fill Operations ===

esp_err_t ppa_hal_fill_rect(const ppa_image_t* dst_img,
                            const ppa_rect_t* fill_rect,
                            uint32_t color,
                            bool blocking) {
    if (!dst_img) {
        return ESP_ERR_INVALID_ARG;
    }
    
    ESP_LOGD(TAG, "Fill rectangle operation (stub) - color: 0x%08lX", color);
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t ppa_hal_clear_image(const ppa_image_t* dst_img,
                              uint32_t color,
                              bool blocking) {
    if (!dst_img) {
        return ESP_ERR_INVALID_ARG;
    }
    
    ESP_LOGD(TAG, "Clear image operation (stub) - color: 0x%08lX", color);
    return ESP_ERR_NOT_SUPPORTED;
}

// === LVGL Integration ===

#ifdef PPA_ENABLE_LVGL_INTEGRATION
esp_err_t ppa_hal_lvgl_init(void) {
    ESP_LOGI(TAG, "LVGL PPA integration initialized (stub mode)");
    ESP_LOGW(TAG, "Hardware acceleration not available - using software rendering");
    return ESP_OK;
}

void ppa_hal_lvgl_fill(lv_disp_drv_t* disp_drv, lv_color_t* dest_buf, 
                       lv_coord_t dest_width, const lv_area_t* fill_area, 
                       lv_color_t color) {
    ESP_LOGV(TAG, "LVGL fill operation (stub - no hardware acceleration)");
    // In stub mode, just log the operation
    // Hardware acceleration will be available when ESP-IDF PPA driver is integrated
}

void ppa_hal_lvgl_blend(lv_disp_drv_t* disp_drv, lv_color_t* dest_buf,
                        lv_coord_t dest_width, const lv_area_t* dest_area,
                        const lv_color_t* src_buf, lv_coord_t src_width,
                        const lv_area_t* src_area, lv_opa_t opa) {
    ESP_LOGV(TAG, "LVGL blend operation (stub - no hardware acceleration)");
    // In stub mode, just log the operation
    // Hardware acceleration will be available when ESP-IDF PPA driver is integrated
}

void ppa_hal_lvgl_blit(lv_disp_drv_t* disp_drv, lv_color_t* dest_buf,
                       lv_coord_t dest_width, const lv_area_t* dest_area,
                       const lv_color_t* src_buf, lv_coord_t src_width,
                       const lv_area_t* src_area) {
    ESP_LOGV(TAG, "LVGL blit operation (stub - no hardware acceleration)");
    // In stub mode, just log the operation
    // Hardware acceleration will be available when ESP-IDF PPA driver is integrated
}
#endif

// === Utility Functions ===

bool ppa_hal_format_supported(ppa_image_format_t src_format, 
                              ppa_image_format_t dst_format) {
    // In stub mode, report limited format support
    switch (src_format) {
        case PPA_FORMAT_RGB565:
        case PPA_FORMAT_RGB888:
        case PPA_FORMAT_ARGB8888:
            return (dst_format == PPA_FORMAT_RGB565 || 
                    dst_format == PPA_FORMAT_RGB888 || 
                    dst_format == PPA_FORMAT_ARGB8888);
        default:
            return false;
    }
}

uint8_t ppa_hal_bytes_per_pixel(ppa_image_format_t format) {
    switch (format) {
        case PPA_FORMAT_RGB565: return 2;
        case PPA_FORMAT_RGB888: return 3;
        case PPA_FORMAT_ARGB8888: return 4;
        case PPA_FORMAT_YUV420: return 1;
        case PPA_FORMAT_YUV444: return 3;
        case PPA_FORMAT_A8: return 1;
        case PPA_FORMAT_A4: return 1;
        default: return 2;
    }
}

void* ppa_hal_align_buffer(void* addr) {
    uintptr_t aligned = ((uintptr_t)addr + PPA_CACHE_LINE_SIZE - 1) & ~(PPA_CACHE_LINE_SIZE - 1);
    return (void*)aligned;
}

size_t ppa_hal_align_size(size_t size) {
    return (size + PPA_CACHE_LINE_SIZE - 1) & ~(PPA_CACHE_LINE_SIZE - 1);
}

void* ppa_hal_alloc_buffer(size_t size, bool use_psram) {
    size_t aligned_size = ppa_hal_align_size(size);
    uint32_t caps = MALLOC_CAP_DMA;
    
    if (use_psram) {
        caps |= MALLOC_CAP_SPIRAM;
    } else {
        caps |= MALLOC_CAP_INTERNAL;
    }
    
    void* buffer = heap_caps_aligned_alloc(PPA_CACHE_LINE_SIZE, aligned_size, caps);
    if (!buffer) {
        ESP_LOGE(TAG, "Failed to allocate %zu bytes for PPA buffer", aligned_size);
        return NULL;
    }
    
    ESP_LOGD(TAG, "Allocated %zu bytes PPA buffer at %p", aligned_size, buffer);
    return buffer;
}

void ppa_hal_free_buffer(void* buffer) {
    if (buffer) {
        heap_caps_free(buffer);
    }
}

esp_err_t ppa_hal_wait_completion(uint32_t timeout_ms) {
    // In stub mode, operations complete immediately
    return ESP_OK;
}

esp_err_t ppa_hal_get_performance_stats(uint32_t* ops_per_sec, uint32_t* avg_time_us) {
    if (!ops_per_sec || !avg_time_us) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // Return zero stats in stub mode
    *ops_per_sec = 0;
    *avg_time_us = 0;
    
    return ESP_OK;
}

#endif // CONFIG_ESP_PPA_ACCELERATION