#include "display_hal.h"
#include "../system/os_manager.h"
#include <esp_log.h>

static const char* TAG = "DisplayHAL";

DisplayHAL::~DisplayHAL() {
    shutdown();
}

os_error_t DisplayHAL::initialize() {
    if (m_initialized) {
        return OS_OK;
    }

    ESP_LOGI(TAG, "Initializing Display HAL (%dx%d)", OS_SCREEN_WIDTH, OS_SCREEN_HEIGHT);

    // Initialize LVGL if not already done
    if (!lv_is_initialized()) {
        lv_init();
    }

    // Initialize LVGL display driver
    os_error_t result = initializeLVGL();
    if (result != OS_OK) {
        ESP_LOGE(TAG, "Failed to initialize LVGL display");
        return result;
    }

    // Set default brightness and enable display
    setBrightness(128);
    setEnabled(true);

    m_lastFPSUpdate = millis();
    m_initialized = true;

    ESP_LOGI(TAG, "Display HAL initialized successfully");
    return OS_OK;
}

os_error_t DisplayHAL::shutdown() {
    if (!m_initialized) {
        return OS_OK;
    }

    ESP_LOGI(TAG, "Shutting down Display HAL");

    // Disable display
    setEnabled(false);

    // Clean up LVGL resources
    if (m_buffer1) {
        free(m_buffer1);
        m_buffer1 = nullptr;
    }
    if (m_buffer2) {
        free(m_buffer2);
        m_buffer2 = nullptr;
    }

    m_lvglDisplay = nullptr;
    m_initialized = false;

    ESP_LOGI(TAG, "Display HAL shutdown complete");
    return OS_OK;
}

os_error_t DisplayHAL::update(uint32_t deltaTime) {
    if (!m_initialized || !m_enabled) {
        return OS_OK;
    }

    // Handle LVGL tasks
    lv_timer_handler();

    // Update FPS statistics
    updateFPS();

    return OS_OK;
}

bool DisplayHAL::selfTest() {
    if (!m_initialized) {
        return false;
    }

    ESP_LOGI(TAG, "Running display self-test");

    // Basic functionality test
    if (!m_lvglDisplay) {
        ESP_LOGE(TAG, "LVGL display not initialized");
        return false;
    }

    // Test draw buffer allocation
    if (!m_buffer1) {
        ESP_LOGE(TAG, "Draw buffer not allocated");
        return false;
    }

    // Test display dimensions
    if (getWidth() != OS_SCREEN_WIDTH || getHeight() != OS_SCREEN_HEIGHT) {
        ESP_LOGE(TAG, "Invalid display dimensions");
        return false;
    }

    ESP_LOGI(TAG, "Display self-test passed");
    return true;
}

os_error_t DisplayHAL::setBrightness(uint8_t brightness) {
    if (!m_initialized) {
        return OS_ERROR_GENERIC;
    }

    m_brightness = brightness;

    // TODO: Implement actual brightness control via PWM or I2C
    // For now, just store the value
    
    ESP_LOGD(TAG, "Set brightness to %d", brightness);
    return OS_OK;
}

os_error_t DisplayHAL::setEnabled(bool enabled) {
    if (!m_initialized) {
        return OS_ERROR_GENERIC;
    }

    if (m_enabled == enabled) {
        return OS_OK;
    }

    m_enabled = enabled;

    if (enabled) {
        ESP_LOGI(TAG, "Display enabled");
        // TODO: Power on display
    } else {
        ESP_LOGI(TAG, "Display disabled");
        // TODO: Power off display
    }

    return OS_OK;
}

os_error_t DisplayHAL::setLowPowerMode(bool enabled) {
    if (!m_initialized) {
        return OS_ERROR_GENERIC;
    }

    m_lowPowerMode = enabled;

    if (enabled) {
        // Reduce refresh rate and brightness for power saving
        setBrightness(m_brightness / 2);
        ESP_LOGI(TAG, "Display low power mode enabled");
    } else {
        // Restore normal operation
        setBrightness(m_brightness);
        ESP_LOGI(TAG, "Display low power mode disabled");
    }

    return OS_OK;
}

os_error_t DisplayHAL::forceRefresh() {
    if (!m_initialized || !m_enabled) {
        return OS_ERROR_GENERIC;
    }

    // Force LVGL to refresh the entire screen
    lv_obj_invalidate(lv_scr_act());
    lv_refr_now(m_lvglDisplay);

    m_lastRefresh = millis();
    return OS_OK;
}

void DisplayHAL::printStats() const {
    ESP_LOGI(TAG, "=== Display HAL Statistics ===");
    ESP_LOGI(TAG, "Resolution: %dx%d", getWidth(), getHeight());
    ESP_LOGI(TAG, "Enabled: %s", m_enabled ? "yes" : "no");
    ESP_LOGI(TAG, "Brightness: %d/255", m_brightness);
    ESP_LOGI(TAG, "Low power mode: %s", m_lowPowerMode ? "yes" : "no");
    ESP_LOGI(TAG, "FPS: %.1f", m_fps);
    ESP_LOGI(TAG, "Total flushes: %d", m_totalFlushes);
    ESP_LOGI(TAG, "Last refresh: %d ms ago", millis() - m_lastRefresh);
}

void DisplayHAL::lvglFlushCallback(lv_disp_drv_t* disp_drv, 
                                  const lv_area_t* area, 
                                  lv_color_t* color_p) {
    DisplayHAL* self = static_cast<DisplayHAL*>(disp_drv->user_data);
    
    if (self) {
        self->m_totalFlushes++;
    }

    // TODO: Implement actual display flushing to hardware
    // For now, just mark as ready
    lv_disp_flush_ready(disp_drv);
}

os_error_t DisplayHAL::initializeLVGL() {
    // Calculate buffer size (10 lines worth of pixels)
    uint32_t bufferSize = OS_SCREEN_WIDTH * 10;
    
    // Allocate draw buffers
    m_buffer1 = static_cast<lv_color_t*>(malloc(bufferSize * sizeof(lv_color_t)));
    if (!m_buffer1) {
        ESP_LOGE(TAG, "Failed to allocate primary draw buffer");
        return OS_ERROR_NO_MEMORY;
    }

    // Optional second buffer for better performance
    m_buffer2 = static_cast<lv_color_t*>(malloc(bufferSize * sizeof(lv_color_t)));
    if (!m_buffer2) {
        ESP_LOGW(TAG, "Failed to allocate secondary draw buffer, using single buffer");
    }

    // Initialize draw buffer
    lv_disp_draw_buf_init(&m_drawBuffer, m_buffer1, m_buffer2, bufferSize);

    // Initialize display driver
    lv_disp_drv_init(&m_displayDriver);
    m_displayDriver.hor_res = OS_SCREEN_WIDTH;
    m_displayDriver.ver_res = OS_SCREEN_HEIGHT;
    m_displayDriver.flush_cb = lvglFlushCallback;
    m_displayDriver.draw_buf = &m_drawBuffer;
    m_displayDriver.user_data = this;

    // Register the driver
    m_lvglDisplay = lv_disp_drv_register(&m_displayDriver);
    if (!m_lvglDisplay) {
        ESP_LOGE(TAG, "Failed to register LVGL display driver");
        return OS_ERROR_GENERIC;
    }

    ESP_LOGI(TAG, "LVGL display driver initialized (%s buffer)",
             m_buffer2 ? "double" : "single");

    return OS_OK;
}

void DisplayHAL::updateFPS() {
    m_frameCount++;
    
    uint32_t now = millis();
    uint32_t elapsed = now - m_lastFPSUpdate;
    
    if (elapsed >= 1000) { // Update every second
        m_fps = (float)m_frameCount * 1000.0f / elapsed;
        m_frameCount = 0;
        m_lastFPSUpdate = now;
    }
}