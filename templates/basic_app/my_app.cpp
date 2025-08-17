#include "my_app.h"
#include "../../src/system/os_manager.h"
#include <esp_log.h>

// Logging tag for this app
static const char* TAG = "MyApp";

MyApp::MyApp() : BaseApp("com.example.myapp", "My Application", "1.0.0") {
    // Set app metadata
    setDescription("A basic application template for M5Stack Tab5");
    setAuthor("Your Name");
    setPriority(AppPriority::APP_NORMAL);
    
    // Initialize state
    m_isActive = false;
    m_clickCount = 0;
    m_lastUpdateTime = 0;
}

os_error_t MyApp::initialize() {
    // Check if already initialized
    if (m_initialized) {
        return OS_OK;
    }

    log(ESP_LOG_INFO, "Initializing My Application");

    // Set estimated memory usage (adjust based on your app's needs)
    setMemoryUsage(8192); // 8KB estimated usage

    // Initialize any app-specific resources here
    // Examples:
    // - Load configuration
    // - Initialize services
    // - Allocate buffers
    // - Setup background tasks

    m_initialized = true;
    log(ESP_LOG_INFO, "My Application initialized successfully");

    return OS_OK;
}

os_error_t MyApp::update(uint32_t deltaTime) {
    // This method is called every frame while the app is running
    // deltaTime contains the milliseconds since the last update
    
    // Example: Update counter display every second
    uint32_t currentTime = millis();
    if (currentTime - m_lastUpdateTime >= 1000) {
        m_lastUpdateTime = currentTime;
        
        // Update runtime counter if UI is created
        if (m_counterLabel) {
            uint32_t runtime = getRuntime() / 1000; // Convert to seconds
            char buffer[64];
            snprintf(buffer, sizeof(buffer), "Runtime: %lu seconds", runtime);
            lv_label_set_text(m_counterLabel, buffer);
        }
    }

    // Add your app's update logic here
    // Examples:
    // - Process incoming data
    // - Update animations
    // - Handle state changes
    // - Check for events

    return OS_OK;
}

os_error_t MyApp::shutdown() {
    // Check if already shut down
    if (!m_initialized) {
        return OS_OK;
    }

    log(ESP_LOG_INFO, "Shutting down My Application");

    // Clean up app-specific resources here
    // Examples:
    // - Save app state
    // - Stop background tasks
    // - Free allocated memory
    // - Close file handles
    // - Disconnect from services

    m_initialized = false;
    log(ESP_LOG_INFO, "My Application shutdown complete");

    return OS_OK;
}

os_error_t MyApp::createUI(lv_obj_t* parent) {
    // Validate parent object
    if (!parent) {
        log(ESP_LOG_ERROR, "Invalid parent object for UI creation");
        return OS_ERROR_INVALID_PARAM;
    }

    log(ESP_LOG_INFO, "Creating My Application UI");

    // Create main container
    m_uiContainer = lv_obj_create(parent);
    lv_obj_set_size(m_uiContainer, LV_HOR_RES, 
                    LV_VER_RES - OS_STATUS_BAR_HEIGHT - OS_DOCK_HEIGHT);
    lv_obj_align(m_uiContainer, LV_ALIGN_CENTER, 0, 0);
    
    // Apply theme styling
    lv_obj_set_style_bg_color(m_uiContainer, lv_color_hex(0x1E1E1E), 0);
    lv_obj_set_style_border_opa(m_uiContainer, LV_OPA_TRANSP, 0);

    // Create the main UI components
    createMainUI();

    log(ESP_LOG_INFO, "My Application UI created successfully");
    return OS_OK;
}

os_error_t MyApp::destroyUI() {
    // Check if UI exists
    if (!m_uiContainer) {
        return OS_OK;
    }

    log(ESP_LOG_INFO, "Destroying My Application UI");

    // Delete the main container (automatically deletes all children)
    lv_obj_del(m_uiContainer);
    
    // Reset UI element pointers
    m_uiContainer = nullptr;
    m_mainButton = nullptr;
    m_statusLabel = nullptr;
    m_counterLabel = nullptr;

    log(ESP_LOG_INFO, "My Application UI destroyed");
    return OS_OK;
}

void MyApp::createMainUI() {
    // Create title label
    lv_obj_t* titleLabel = lv_label_create(m_uiContainer);
    lv_label_set_text(titleLabel, "My Application");
    lv_obj_set_style_text_color(titleLabel, lv_color_white(), 0);
    lv_obj_set_style_text_font(titleLabel, &lv_font_montserrat_24, 0);
    lv_obj_align(titleLabel, LV_ALIGN_TOP_MID, 0, 30);

    // Create main interaction button
    m_mainButton = lv_btn_create(m_uiContainer);
    lv_obj_set_size(m_mainButton, 200, 60);
    lv_obj_center(m_mainButton);
    
    // Style the button
    lv_obj_set_style_bg_color(m_mainButton, lv_color_hex(0x3498DB), 0);
    lv_obj_set_style_bg_color(m_mainButton, lv_color_hex(0x2980B9), LV_STATE_PRESSED);
    
    // Add button label
    lv_obj_t* btnLabel = lv_label_create(m_mainButton);
    lv_label_set_text(btnLabel, "Click Me!");
    lv_obj_set_style_text_color(btnLabel, lv_color_white(), 0);
    lv_obj_center(btnLabel);
    
    // Register button callback
    lv_obj_add_event_cb(m_mainButton, buttonCallback, LV_EVENT_CLICKED, this);

    // Create status label
    m_statusLabel = lv_label_create(m_uiContainer);
    lv_label_set_text(m_statusLabel, "Ready");
    lv_obj_set_style_text_color(m_statusLabel, lv_color_hex(0x95A5A6), 0);
    lv_obj_align(m_statusLabel, LV_ALIGN_BOTTOM_MID, 0, -80);

    // Create counter label
    m_counterLabel = lv_label_create(m_uiContainer);
    lv_label_set_text(m_counterLabel, "Runtime: 0 seconds");
    lv_obj_set_style_text_color(m_counterLabel, lv_color_hex(0x95A5A6), 0);
    lv_obj_align(m_counterLabel, LV_ALIGN_BOTTOM_MID, 0, -50);

    // Create click count label
    lv_obj_t* clickLabel = lv_label_create(m_uiContainer);
    char clickText[32];
    snprintf(clickText, sizeof(clickText), "Clicks: %lu", m_clickCount);
    lv_label_set_text(clickLabel, clickText);
    lv_obj_set_style_text_color(clickLabel, lv_color_hex(0x95A5A6), 0);
    lv_obj_align(clickLabel, LV_ALIGN_BOTTOM_MID, 0, -20);
}

void MyApp::handleButtonClick(lv_obj_t* button) {
    // Increment click counter
    m_clickCount++;
    
    // Toggle active state
    m_isActive = !m_isActive;
    
    // Update button appearance and text
    lv_obj_t* btnLabel = lv_obj_get_child(button, 0);
    if (m_isActive) {
        lv_label_set_text(btnLabel, "Active!");
        lv_obj_set_style_bg_color(button, lv_color_hex(0x2ECC71), 0);
        updateStatus("Application is now active");
    } else {
        lv_label_set_text(btnLabel, "Click Me!");
        lv_obj_set_style_bg_color(button, lv_color_hex(0x3498DB), 0);
        updateStatus("Application is now inactive");
    }
    
    // Update click count display
    // Find the click count label (third label created)
    lv_obj_t* clickLabel = nullptr;
    uint32_t childCount = lv_obj_get_child_cnt(m_uiContainer);
    if (childCount >= 4) {
        clickLabel = lv_obj_get_child(m_uiContainer, 3); // Fourth child (0-indexed)
    }
    
    if (clickLabel) {
        char clickText[32];
        snprintf(clickText, sizeof(clickText), "Clicks: %lu", m_clickCount);
        lv_label_set_text(clickLabel, clickText);
    }

    log(ESP_LOG_INFO, "Button clicked! Count: %lu, Active: %s", 
        m_clickCount, m_isActive ? "true" : "false");
}

void MyApp::updateStatus(const char* message) {
    if (m_statusLabel && message) {
        lv_label_set_text(m_statusLabel, message);
        log(ESP_LOG_INFO, "Status updated: %s", message);
    }
}

// Static callback function for button events
void MyApp::buttonCallback(lv_event_t* e) {
    // Get the app instance from user data
    MyApp* app = static_cast<MyApp*>(lv_event_get_user_data(e));
    
    // Get the button that was clicked
    lv_obj_t* button = lv_event_get_target(e);
    
    // Forward to instance method
    if (app) {
        app->handleButtonClick(button);
    }
}

// Factory function for app creation (required for dynamic loading)
extern "C" std::unique_ptr<BaseApp> createMyApp() {
    return std::make_unique<MyApp>();
}