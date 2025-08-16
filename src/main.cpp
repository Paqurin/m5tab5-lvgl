#include <Arduino.h>
#include <lvgl.h>
#include "system/os_manager.h"

#if LV_COLOR_DEPTH != 16
#error "LV_COLOR_DEPTH should be 16 for M5Stack displays"
#endif

/**
 * @file main.cpp
 * @brief Main entry point for M5Stack Tab5 LVGL test os
 * 
 * Initializes and runs the comprehensive operating system framework
 * with modular architecture including HAL, UI, Apps, and Services layers.
 * Designed specifically for M5Stack Tab5 ESP32-P4 hardware.
 */

// Demo application class
class DemoApp : public BaseApp {
private:
    lv_obj_t* m_label = nullptr;
    uint32_t m_counter = 0;

public:
    DemoApp() : BaseApp("demo", "Demo Application", "1.0.0") {
        setDescription("Built-in demo application for M5Stack Tab5");
        setAuthor("M5Stack Tab5 OS");
        setPriority(AppPriority::APP_NORMAL);
    }

    os_error_t initialize() override {
        log(ESP_LOG_INFO, "Initializing demo application");
        setMemoryUsage(1024); // Estimate 1KB memory usage
        return OS_OK;
    }

    os_error_t update(uint32_t deltaTime) override {
        // Update counter every second
        static uint32_t lastUpdate = 0;
        if (millis() - lastUpdate >= 1000) {
            m_counter++;
            
            if (m_label) {
                char text[512];
                snprintf(text, sizeof(text),
                    "M5Stack Tab5 LVGL test os\n"
                    "Version %s\n\n"
                    "5-inch 1280x720 MIPI-DSI Display\n"
                    "GT911 Touch Controller\n"
                    "ESP32-P4 Microcontroller\n\n"
                    "System Status:\n"
                    "Uptime: %d seconds\n"
                    "Free Heap: %d KB\n"
                    "App Runtime: %d seconds\n\n"
                    "Framework Modules:\n"
                    "✓ OS Manager\n"
                    "✓ Memory Manager\n"
                    "✓ Task Scheduler\n"
                    "✓ Event System\n"
                    "✓ HAL Manager\n"
                    "✓ UI Manager\n"
                    "✓ App Manager\n"
                    "✓ Service Manager\n\n"
                    "All systems operational!",
                    OS_VERSION_STRING,
                    OS().getUptime() / 1000,
                    OS().getFreeHeap() / 1024,
                    getRuntime() / 1000);
                
                lv_label_set_text(m_label, text);
            }
            
            lastUpdate = millis();
        }
        
        return OS_OK;
    }

    os_error_t shutdown() override {
        log(ESP_LOG_INFO, "Shutting down demo application");
        return OS_OK;
    }

    os_error_t createUI(lv_obj_t* parent) override {
        log(ESP_LOG_INFO, "Creating demo UI");
        
        m_uiContainer = lv_obj_create(parent);
        lv_obj_set_size(m_uiContainer, LV_HOR_RES, LV_VER_RES - OS_STATUS_BAR_HEIGHT - OS_DOCK_HEIGHT);
        lv_obj_align(m_uiContainer, LV_ALIGN_CENTER, 0, 0);
        lv_obj_set_style_bg_color(m_uiContainer, lv_color_hex(0x1E1E1E), 0);
        lv_obj_set_style_border_opa(m_uiContainer, LV_OPA_TRANSP, 0);

        m_label = lv_label_create(m_uiContainer);
        lv_obj_set_style_text_color(m_label, lv_color_white(), 0);
        lv_obj_set_style_text_align(m_label, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_center(m_label);

        return OS_OK;
    }

    os_error_t destroyUI() override {
        log(ESP_LOG_INFO, "Destroying demo UI");
        
        if (m_uiContainer) {
            lv_obj_del(m_uiContainer);
            m_uiContainer = nullptr;
            m_label = nullptr;
        }
        
        return OS_OK;
    }
};

void setup() {
    Serial.begin(115200);
    delay(1000); // Give time for serial to initialize
    
    Serial.println("\n" "========================================");
    Serial.println("M5Stack Tab5 LVGL test os v" OS_VERSION_STRING);
    Serial.println("ESP32-P4 Modular Operating System");
    Serial.println("========================================\n");

    // Initialize the operating system
    os_error_t result = OS().initialize();
    if (result != OS_OK) {
        Serial.printf("FATAL: OS initialization failed with error %d\n", result);
        Serial.println("System halted.");
        while (true) {
            delay(1000);
        }
    }

    // Register the demo application
    OS().getAppManager().registerApp("demo", []() -> std::unique_ptr<BaseApp> {
        return std::make_unique<DemoApp>();
    });

    // Start the operating system
    result = OS().start();
    if (result != OS_OK) {
        Serial.printf("FATAL: OS start failed with error %d\n", result);
        Serial.println("System halted.");
        while (true) {
            delay(1000);
        }
    }

    // Launch the demo application
    result = OS().getAppManager().launchApp("demo");
    if (result != OS_OK) {
        Serial.printf("WARNING: Failed to launch demo app: %d\n", result);
    }

    // Create demo app UI
    BaseApp* demoApp = OS().getAppManager().getApp("demo");
    if (demoApp) {
        demoApp->createUI(lv_scr_act());
    }

    Serial.println("M5Stack Tab5 LVGL test os startup complete!");
    Serial.println("System is ready for operation.\n");
}

void loop() {
    // Update the operating system
    os_error_t result = OS().update();
    if (result != OS_OK) {
        Serial.printf("OS update error: %d\n", result);
    }

    // Small delay to prevent watchdog issues
    delay(1);
}