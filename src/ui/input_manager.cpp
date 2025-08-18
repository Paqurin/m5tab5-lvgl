#include "input_manager.h"
#include "../system/os_manager.h"
#include <esp_log.h>

static const char* TAG = "InputManager";

// Static members for LVGL callback
bool InputManager::s_touched = false;
uint16_t InputManager::s_touchX = 0;
uint16_t InputManager::s_touchY = 0;

InputManager::~InputManager() {
    shutdown();
}

os_error_t InputManager::initialize() {
    if (m_initialized) {
        return OS_OK;
    }

    ESP_LOGI(TAG, "Initializing Input Manager");

    // Initialize LVGL input device
    os_error_t result = initializeLVGLInput();
    if (result != OS_OK) {
        ESP_LOGE(TAG, "Failed to initialize LVGL input device");
        return result;
    }

    // Subscribe to touch events from HAL
    SUBSCRIBE_EVENT(EVENT_UI_TOUCH_PRESS, 
                   [this](const EventData& event) {
                       if (event.data && event.dataSize >= sizeof(TouchEventData)) {
                           handleTouchEvent(*(TouchEventData*)event.data);
                       }
                   });

    m_initialized = true;
    ESP_LOGI(TAG, "Input Manager initialized");

    return OS_OK;
}

os_error_t InputManager::shutdown() {
    if (!m_initialized) {
        return OS_OK;
    }

    ESP_LOGI(TAG, "Shutting down Input Manager");

    // Reset touch state
    s_touched = false;
    s_touchX = 0;
    s_touchY = 0;

    m_initialized = false;
    ESP_LOGI(TAG, "Input Manager shutdown complete");

    return OS_OK;
}

os_error_t InputManager::update(uint32_t deltaTime) {
    if (!m_initialized || !m_enabled) {
        return OS_OK;
    }

    // Input processing is handled by LVGL and event callbacks
    return OS_OK;
}

void InputManager::handleTouchEvent(const TouchEventData& eventData) {
    if (!m_initialized || !m_enabled) {
        return;
    }

    switch (eventData.event) {
        case TouchEvent::PRESS:
            s_touched = true;
            s_touchX = eventData.point.x;
            s_touchY = eventData.point.y;
            ESP_LOGD(TAG, "Touch press at (%d, %d)", s_touchX, s_touchY);
            break;

        case TouchEvent::MOVE:
            if (s_touched) {
                s_touchX = eventData.point.x;
                s_touchY = eventData.point.y;
                ESP_LOGD(TAG, "Touch move to (%d, %d)", s_touchX, s_touchY);
            }
            break;

        case TouchEvent::RELEASE:
            s_touched = false;
            ESP_LOGD(TAG, "Touch release");
            break;

        case TouchEvent::GESTURE:
            ESP_LOGD(TAG, "Gesture detected: %d (touches: %d)", (int)eventData.gesture, eventData.touchCount);
            if (m_multiTouchEnabled && eventData.touchCount > 1) {
                handleMultiTouchGesture(eventData);
            }
            // Handle single-touch gesture events
            break;
    }
}

os_error_t InputManager::initializeLVGLInput() {
    // Initialize LVGL input device driver
    lv_indev_drv_init(&m_inputDriver);
    m_inputDriver.type = LV_INDEV_TYPE_POINTER;
    m_inputDriver.read_cb = lvglInputRead;

    // Register input device with LVGL
    m_inputDevice = lv_indev_drv_register(&m_inputDriver);
    if (!m_inputDevice) {
        ESP_LOGE(TAG, "Failed to register LVGL input device");
        return OS_ERROR_GENERIC;
    }

    ESP_LOGD(TAG, "LVGL input device initialized");
    return OS_OK;
}

void InputManager::handleMultiTouchGesture(const TouchEventData& eventData) {
    if (!m_multiTouchEnabled) {
        return;
    }

    ESP_LOGI(TAG, "Processing multi-touch gesture: %d with %d touches", 
             (int)eventData.gesture, eventData.touchCount);

    switch (eventData.gesture) {
        case GestureType::TWO_FINGER_TAP:
            ESP_LOGI(TAG, "Two-finger tap - Context menu");
            // Trigger context menu or app switcher
            break;

        case GestureType::THREE_FINGER_TAP:
            ESP_LOGI(TAG, "Three-finger tap - Home screen");
            // Navigate to home screen
            PUBLISH_EVENT(EVENT_UI_HOME_BUTTON, nullptr, 0);
            break;

        case GestureType::FOUR_FINGER_TAP:
            ESP_LOGI(TAG, "Four-finger tap - App switcher");
            // Show app switcher/task manager
            PUBLISH_EVENT(EVENT_UI_TASK_SWITCHER, nullptr, 0);
            break;

        case GestureType::FIVE_FINGER_TAP:
            ESP_LOGI(TAG, "Five-finger tap - Settings/control center");
            // Show settings or control center
            PUBLISH_EVENT(EVENT_UI_CONTROL_CENTER, nullptr, 0);
            break;

        case GestureType::PINCH_IN:
            ESP_LOGI(TAG, "Pinch in - Zoom out (distance: %.1f)", eventData.gestureDistance);
            // Send zoom out event to active app
            PUBLISH_EVENT(EVENT_UI_ZOOM_OUT, (void*)&eventData.gestureDistance, sizeof(float));
            break;

        case GestureType::PINCH_OUT:
            ESP_LOGI(TAG, "Pinch out - Zoom in (distance: %.1f)", eventData.gestureDistance);
            // Send zoom in event to active app
            PUBLISH_EVENT(EVENT_UI_ZOOM_IN, (void*)&eventData.gestureDistance, sizeof(float));
            break;

        case GestureType::TWO_FINGER_SWIPE_UP:
            ESP_LOGI(TAG, "Two-finger swipe up - Scroll up/back");
            // Navigate back or scroll up
            PUBLISH_EVENT(EVENT_UI_NAVIGATE_BACK, nullptr, 0);
            break;

        case GestureType::TWO_FINGER_SWIPE_DOWN:
            ESP_LOGI(TAG, "Two-finger swipe down - Scroll down/forward");
            // Navigate forward or scroll down
            PUBLISH_EVENT(EVENT_UI_NAVIGATE_FORWARD, nullptr, 0);
            break;

        case GestureType::THREE_FINGER_SWIPE_LEFT:
            ESP_LOGI(TAG, "Three-finger swipe left - Next app");
            // Switch to next app
            PUBLISH_EVENT(EVENT_UI_NEXT_APP, nullptr, 0);
            break;

        case GestureType::THREE_FINGER_SWIPE_RIGHT:
            ESP_LOGI(TAG, "Three-finger swipe right - Previous app");
            // Switch to previous app
            PUBLISH_EVENT(EVENT_UI_PREV_APP, nullptr, 0);
            break;

        case GestureType::FIVE_FINGER_PINCH:
            ESP_LOGI(TAG, "Five-finger pinch - Minimize all");
            // Minimize all windows/apps to home screen
            PUBLISH_EVENT(EVENT_UI_MINIMIZE_ALL, nullptr, 0);
            break;

        case GestureType::PALM_REJECTION:
            ESP_LOGD(TAG, "Palm touch detected and rejected");
            // Palm rejection - ignore this touch
            break;

        default:
            ESP_LOGD(TAG, "Unhandled multi-touch gesture: %d", (int)eventData.gesture);
            break;
    }
}

void InputManager::lvglInputRead(lv_indev_drv_t* indev_drv, lv_indev_data_t* data) {
    // Fill LVGL input data structure
    data->point.x = s_touchX;
    data->point.y = s_touchY;
    data->state = s_touched ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
}