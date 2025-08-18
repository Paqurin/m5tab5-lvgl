#include "touch_hal.h"
#include "../system/os_manager.h"
#include <esp_log.h>
#include <Wire.h>
#include <cmath>

static const char* TAG = "TouchHAL";

TouchHAL::~TouchHAL() {
    shutdown();
}

os_error_t TouchHAL::initialize() {
    if (m_initialized) {
        return OS_OK;
    }

    ESP_LOGI(TAG, "Initializing Touch HAL (GT911)");

    // Initialize I2C if not already done
    // Note: This would typically be done in a board-specific initialization
    Wire.begin();

    // Initialize GT911 controller
    os_error_t result = initializeController();
    if (result != OS_OK) {
        ESP_LOGE(TAG, "Failed to initialize GT911 controller");
        return result;
    }

    // Reserve space for touch points
    m_currentTouches.reserve(10); // GT911 supports up to 10 points
    m_previousTouches.reserve(10);
    m_gestureStartPoints.reserve(10);

    // Enable touch and set default sensitivity
    setEnabled(true);
    setSensitivity(128);
    setMultiTouchEnabled(true);
    setMaxTouches(5);

    m_initialized = true;
    ESP_LOGI(TAG, "Touch HAL initialized successfully");

    return OS_OK;
}

os_error_t TouchHAL::shutdown() {
    if (!m_initialized) {
        return OS_OK;
    }

    ESP_LOGI(TAG, "Shutting down Touch HAL");

    // Disable touch
    setEnabled(false);

    // Clear touch data
    m_currentTouches.clear();
    m_previousTouches.clear();
    m_touchCallback = nullptr;

    m_initialized = false;
    ESP_LOGI(TAG, "Touch HAL shutdown complete");

    return OS_OK;
}

os_error_t TouchHAL::update(uint32_t deltaTime) {
    if (!m_initialized || !m_enabled) {
        return OS_OK;
    }

    // Read touch data from controller
    os_error_t result = readTouchData();
    if (result != OS_OK) {
        return result;
    }

    // Filter and process touch points
    filterTouchPoints();
    processTouchEvents();

    return OS_OK;
}

bool TouchHAL::selfTest() {
    if (!m_initialized) {
        return false;
    }

    ESP_LOGI(TAG, "Running touch controller self-test");

    // TODO: Implement GT911 specific self-test
    // For now, just check basic communication

    // Test I2C communication
    Wire.beginTransmission(GT911_I2C_ADDR);
    if (Wire.endTransmission() != 0) {
        ESP_LOGE(TAG, "GT911 I2C communication failed");
        return false;
    }

    ESP_LOGI(TAG, "Touch controller self-test passed");
    return true;
}

os_error_t TouchHAL::setEnabled(bool enabled) {
    if (!m_initialized) {
        return OS_ERROR_GENERIC;
    }

    if (m_enabled == enabled) {
        return OS_OK;
    }

    m_enabled = enabled;

    if (enabled) {
        ESP_LOGI(TAG, "Touch input enabled");
        // TODO: Enable GT911 touch detection
    } else {
        ESP_LOGI(TAG, "Touch input disabled");
        // TODO: Disable GT911 touch detection
        // Clear current touches
        m_currentTouches.clear();
    }

    return OS_OK;
}

os_error_t TouchHAL::setLowPowerMode(bool enabled) {
    if (!m_initialized) {
        return OS_ERROR_GENERIC;
    }

    m_lowPowerMode = enabled;

    if (enabled) {
        ESP_LOGI(TAG, "Touch low power mode enabled");
        // TODO: Configure GT911 for low power mode
    } else {
        ESP_LOGI(TAG, "Touch low power mode disabled");
        // TODO: Configure GT911 for normal power mode
    }

    return OS_OK;
}

os_error_t TouchHAL::calibrate() {
    if (!m_initialized) {
        return OS_ERROR_GENERIC;
    }

    ESP_LOGI(TAG, "Calibrating touch controller");

    // TODO: Implement GT911 calibration procedure
    // For now, just record the calibration time
    m_lastCalibration = millis();

    ESP_LOGI(TAG, "Touch calibration complete");
    return OS_OK;
}

size_t TouchHAL::getActiveTouchCount() const {
    size_t count = 0;
    for (const auto& touch : m_currentTouches) {
        if (touch.valid) {
            count++;
        }
    }
    return count;
}

os_error_t TouchHAL::setSensitivity(uint8_t sensitivity) {
    if (!m_initialized) {
        return OS_ERROR_GENERIC;
    }

    m_sensitivity = sensitivity;

    // TODO: Configure GT911 sensitivity
    ESP_LOGD(TAG, "Set touch sensitivity to %d", sensitivity);

    return OS_OK;
}

void TouchHAL::setMaxTouches(uint8_t maxTouches) {
    if (maxTouches > 10) maxTouches = 10;
    if (maxTouches < 1) maxTouches = 1;
    
    m_maxTouches = maxTouches;
    ESP_LOGI(TAG, "Set maximum touches to %d", m_maxTouches);
}

void TouchHAL::printStats() const {
    ESP_LOGI(TAG, "=== Touch HAL Statistics ===");
    ESP_LOGI(TAG, "Enabled: %s", m_enabled ? "yes" : "no");
    ESP_LOGI(TAG, "Low power mode: %s", m_lowPowerMode ? "yes" : "no");
    ESP_LOGI(TAG, "Sensitivity: %d/255", m_sensitivity);
    ESP_LOGI(TAG, "Gesture recognition: %s", m_gestureEnabled ? "yes" : "no");
    ESP_LOGI(TAG, "Multi-touch: %s", m_multiTouchEnabled ? "yes" : "no");
    ESP_LOGI(TAG, "Max touches: %d", m_maxTouches);
    ESP_LOGI(TAG, "Palm rejection: %d", m_palmRejectionThreshold);
    ESP_LOGI(TAG, "Active touches: %d", getActiveTouchCount());
    ESP_LOGI(TAG, "Max simultaneous touches: %d", m_maxSimultaneousTouches);
    ESP_LOGI(TAG, "Total touches: %d", m_totalTouches);
    ESP_LOGI(TAG, "Total gestures: %d", m_totalGestures);
    ESP_LOGI(TAG, "Last calibration: %d ms ago", 
             m_lastCalibration > 0 ? millis() - m_lastCalibration : 0);
}

os_error_t TouchHAL::initializeController() {
    ESP_LOGI(TAG, "Initializing GT911 touch controller");
    
    // GT911 initialization sequence
    Wire.begin();
    delay(10);
    
    // Reset sequence
    // Note: In actual hardware implementation, this would control
    // RST and INT pins according to GT911 specification
    
    // Read product ID to verify controller
    uint8_t productId[4];
    if (!readGT911Register(0x8140, productId, 4)) {
        ESP_LOGE(TAG, "Failed to read GT911 product ID");
        return OS_ERROR_HARDWARE;
    }
    
    // Configure GT911 for 5-point multi-touch
    uint8_t config[1];
    config[0] = 0x05; // Enable 5 touch points
    if (!writeGT911Register(0x804E, config, 1)) {
        ESP_LOGE(TAG, "Failed to configure GT911 touch points");
        return OS_ERROR_HARDWARE;
    }
    
    // Set touch threshold
    config[0] = m_sensitivity;
    if (!writeGT911Register(0x8056, config, 1)) {
        ESP_LOGE(TAG, "Failed to set GT911 touch threshold");
        return OS_ERROR_HARDWARE;
    }
    
    // Enable coordinate output
    config[0] = 0x01;
    if (!writeGT911Register(0x804D, config, 1)) {
        ESP_LOGE(TAG, "Failed to enable GT911 coordinate output");
        return OS_ERROR_HARDWARE;
    }
    
    ESP_LOGI(TAG, "GT911 controller initialized successfully");
    return OS_OK;
}

os_error_t TouchHAL::readTouchData() {
    if (!m_enabled) {
        return OS_OK;
    }
    
    // Store previous touches
    m_previousTouches = m_currentTouches;
    m_currentTouches.clear();

    // Read GT911 status register
    uint8_t touchStatus;
    if (!readGT911Register(GT911_REG_STATUS, &touchStatus, 1)) {
        return OS_ERROR_HARDWARE;
    }
    
    // Check if touch data is ready
    if ((touchStatus & 0x80) == 0) {
        return OS_OK; // No new touch data
    }
    
    // Get number of touch points (lower 4 bits)
    uint8_t touchCount = touchStatus & 0x0F;
    
    // Limit to maximum supported touches
    if (touchCount > m_maxTouches) {
        touchCount = m_maxTouches;
    }
    
    // Read touch point data
    for (uint8_t i = 0; i < touchCount; i++) {
        uint8_t touchData[8];
        uint16_t regAddr = GT911_REG_TOUCH + (i * 8);
        
        if (!readGT911Register(regAddr, touchData, 8)) {
            ESP_LOGW(TAG, "Failed to read touch point %d data", i);
            continue;
        }
        
        TouchPoint point;
        point.id = touchData[0];
        point.x = (touchData[2] << 8) | touchData[1];
        point.y = (touchData[4] << 8) | touchData[3];
        point.pressure = (touchData[6] << 8) | touchData[5];
        point.valid = true;
        point.timestamp = millis();
        
        // Validate coordinates are within screen bounds
        if (point.x < OS_SCREEN_WIDTH && point.y < OS_SCREEN_HEIGHT) {
            m_currentTouches.push_back(point);
            m_totalTouches++;
        }
    }
    
    // Clear status register to acknowledge read
    uint8_t clearStatus = 0x00;
    writeGT911Register(GT911_REG_STATUS, &clearStatus, 1);
    
    return OS_OK;
}

void TouchHAL::processTouchEvents() {
    uint32_t currentTime = millis();
    uint8_t activeTouchCount = getActiveTouchCount();
    
    // Update max simultaneous touches
    if (activeTouchCount > m_maxSimultaneousTouches) {
        m_maxSimultaneousTouches = activeTouchCount;
    }

    // Check for palm rejection if enabled
    if (m_multiTouchEnabled && isPalmTouch(m_currentTouches)) {
        TouchEventData eventData;
        eventData.event = TouchEvent::GESTURE;
        eventData.gesture = GestureType::PALM_REJECTION;
        eventData.timestamp = currentTime;
        eventData.touchCount = activeTouchCount;
        eventData.allTouchPoints = m_currentTouches;
        sendTouchEvent(eventData);
        return; // Skip normal processing for palm touches
    }

    // Process each current touch point
    for (const auto& currentTouch : m_currentTouches) {
        if (!currentTouch.valid) continue;

        // Find corresponding previous touch
        bool foundPrevious = false;
        for (const auto& prevTouch : m_previousTouches) {
            if (prevTouch.id == currentTouch.id && prevTouch.valid) {
                // Touch moved
                if (abs(currentTouch.x - prevTouch.x) > OS_TOUCH_THRESHOLD ||
                    abs(currentTouch.y - prevTouch.y) > OS_TOUCH_THRESHOLD) {
                    
                    TouchEventData eventData;
                    eventData.event = TouchEvent::MOVE;
                    eventData.point = currentTouch;
                    eventData.timestamp = currentTime;
                    eventData.touchCount = activeTouchCount;
                    eventData.allTouchPoints = m_currentTouches;
                    sendTouchEvent(eventData);
                }
                foundPrevious = true;
                break;
            }
        }

        if (!foundPrevious) {
            // New touch (press)
            TouchEventData eventData;
            eventData.event = TouchEvent::PRESS;
            eventData.point = currentTouch;
            eventData.timestamp = currentTime;
            eventData.touchCount = activeTouchCount;
            eventData.allTouchPoints = m_currentTouches;
            sendTouchEvent(eventData);

            m_totalTouches++;
            m_lastTouchTime = currentTime;
        }
    }

    // Check for released touches
    for (const auto& prevTouch : m_previousTouches) {
        if (!prevTouch.valid) continue;

        bool stillActive = false;
        for (const auto& currentTouch : m_currentTouches) {
            if (currentTouch.id == prevTouch.id && currentTouch.valid) {
                stillActive = true;
                break;
            }
        }

        if (!stillActive) {
            // Touch released
            TouchEventData eventData;
            eventData.event = TouchEvent::RELEASE;
            eventData.point = prevTouch;
            eventData.timestamp = currentTime;
            eventData.touchCount = activeTouchCount;
            eventData.allTouchPoints = m_currentTouches;
            sendTouchEvent(eventData);
        }
    }

    // Detect gestures if enabled
    if (m_gestureEnabled) {
        GestureType gesture = GestureType::NONE;
        
        // Try multi-touch gestures first if enabled
        if (m_multiTouchEnabled && activeTouchCount > 1) {
            gesture = detectMultiTouchGesture();
        }
        
        // Fall back to single-touch gestures
        if (gesture == GestureType::NONE) {
            gesture = detectGesture();
        }
        
        if (gesture != GestureType::NONE) {
            TouchEventData eventData;
            eventData.event = TouchEvent::GESTURE;
            eventData.gesture = gesture;
            eventData.timestamp = currentTime;
            eventData.touchCount = activeTouchCount;
            eventData.allTouchPoints = m_currentTouches;
            
            // Calculate gesture parameters
            if (activeTouchCount >= 2) {
                eventData.gestureDistance = calculateDistance(m_currentTouches[0], m_currentTouches[1]);
            }
            
            sendTouchEvent(eventData);
            m_totalGestures++;
        }
    }
}

GestureType TouchHAL::detectGesture() {
    // Simple gesture detection implementation
    // In a real implementation, this would be much more sophisticated

    uint32_t currentTime = millis();
    size_t activeTouches = getActiveTouchCount();

    if (activeTouches == 0) {
        // No active touches, check if we just finished a gesture
        if (m_inGesture && currentTime - m_lastTouchTime > OS_TOUCH_DEBOUNCE_MS) {
            m_inGesture = false;
            
            uint32_t gestureDuration = currentTime - m_gestureStartTime;
            
            // Simple tap detection
            if (gestureDuration < 200) {
                return GestureType::TAP;
            }
            // Long press detection
            else if (gestureDuration > 1000) {
                return GestureType::LONG_PRESS;
            }
        }
    } else if (activeTouches == 1 && !m_inGesture) {
        // Start of potential gesture
        m_inGesture = true;
        m_gestureStartTime = currentTime;
        if (!m_currentTouches.empty() && m_currentTouches[0].valid) {
            m_gestureStartPoint = m_currentTouches[0];
        }
    }

    return GestureType::NONE;
}

void TouchHAL::filterTouchPoints() {
    // Simple filtering to remove noise and apply smoothing
    // In a real implementation, this would be more sophisticated

    for (auto& touch : m_currentTouches) {
        if (!touch.valid) continue;

        // Basic bounds checking
        if (touch.x >= OS_SCREEN_WIDTH) touch.x = OS_SCREEN_WIDTH - 1;
        if (touch.y >= OS_SCREEN_HEIGHT) touch.y = OS_SCREEN_HEIGHT - 1;

        // Simple smoothing with previous touch if available
        for (const auto& prevTouch : m_previousTouches) {
            if (prevTouch.id == touch.id && prevTouch.valid) {
                // Apply simple low-pass filter
                touch.x = (touch.x + prevTouch.x) / 2;
                touch.y = (touch.y + prevTouch.y) / 2;
                break;
            }
        }
    }
}

void TouchHAL::sendTouchEvent(const TouchEventData& eventData) {
    if (m_touchCallback) {
        m_touchCallback(eventData);
    }

    // Also publish as system event
    PUBLISH_EVENT(EVENT_UI_TOUCH_PRESS + (int)eventData.event, 
                  (void*)&eventData, sizeof(eventData));
}

GestureType TouchHAL::detectMultiTouchGesture() {
    uint32_t currentTime = millis();
    uint8_t activeTouchCount = getActiveTouchCount();
    
    if (activeTouchCount < 2) {
        return GestureType::NONE;
    }
    
    // Initialize multi-touch gesture tracking on first detection
    if (m_multiTouchStartTime == 0 || 
        (currentTime - m_multiTouchStartTime) > 500) { // Reset after 500ms gap
        m_multiTouchStartTime = currentTime;
        m_gestureStartPoints.clear();
        
        for (const auto& touch : m_currentTouches) {
            if (touch.valid) {
                m_gestureStartPoints.push_back(touch);
            }
        }
        
        if (activeTouchCount >= 2) {
            m_initialDistance = calculateDistance(m_currentTouches[0], m_currentTouches[1]);
            m_initialCentroid = calculateCentroid(m_currentTouches);
        }
        
        return GestureType::NONE; // Don't trigger gesture immediately
    }
    
    uint32_t gestureDuration = currentTime - m_multiTouchStartTime;
    
    // Short tap gestures (< 300ms)
    if (gestureDuration < 300 && activeTouchCount == 0) {
        switch (m_gestureStartPoints.size()) {
            case 2: return GestureType::TWO_FINGER_TAP;
            case 3: return GestureType::THREE_FINGER_TAP;
            case 4: return GestureType::FOUR_FINGER_TAP;
            case 5: return GestureType::FIVE_FINGER_TAP;
        }
    }
    
    // Active multi-touch gestures
    if (activeTouchCount >= 2 && gestureDuration > 100) {
        TouchPoint currentCentroid = calculateCentroid(m_currentTouches);
        float currentDistance = calculateDistance(m_currentTouches[0], m_currentTouches[1]);
        
        // Pinch/zoom detection
        if (activeTouchCount == 2 && m_initialDistance > 0) {
            float distanceRatio = currentDistance / m_initialDistance;
            
            if (distanceRatio < 0.8) {
                return GestureType::PINCH_IN;
            } else if (distanceRatio > 1.2) {
                return GestureType::PINCH_OUT;
            }
        }
        
        // Five-finger pinch (special case)
        if (activeTouchCount == 5) {
            float distanceFromCenter = calculateDistance(currentCentroid, m_initialCentroid);
            if (distanceFromCenter > 50) {
                return GestureType::FIVE_FINGER_PINCH;
            }
        }
        
        // Multi-finger swipes
        float deltaX = currentCentroid.x - m_initialCentroid.x;
        float deltaY = currentCentroid.y - m_initialCentroid.y;
        
        if (abs(deltaX) > 50 || abs(deltaY) > 50) {
            if (activeTouchCount == 2) {
                if (abs(deltaY) > abs(deltaX)) {
                    return deltaY > 0 ? GestureType::TWO_FINGER_SWIPE_DOWN : GestureType::TWO_FINGER_SWIPE_UP;
                }
            } else if (activeTouchCount == 3) {
                if (abs(deltaX) > abs(deltaY)) {
                    return deltaX > 0 ? GestureType::THREE_FINGER_SWIPE_RIGHT : GestureType::THREE_FINGER_SWIPE_LEFT;
                }
            }
        }
    }
    
    return GestureType::NONE;
}

float TouchHAL::calculateDistance(const TouchPoint& p1, const TouchPoint& p2) {
    float dx = p2.x - p1.x;
    float dy = p2.y - p1.y;
    return sqrt(dx * dx + dy * dy);
}

TouchPoint TouchHAL::calculateCentroid(const std::vector<TouchPoint>& points) {
    TouchPoint centroid;
    centroid.valid = false;
    
    if (points.empty()) {
        return centroid;
    }
    
    float sumX = 0, sumY = 0;
    int validCount = 0;
    
    for (const auto& point : points) {
        if (point.valid) {
            sumX += point.x;
            sumY += point.y;
            validCount++;
        }
    }
    
    if (validCount > 0) {
        centroid.x = static_cast<uint16_t>(sumX / validCount);
        centroid.y = static_cast<uint16_t>(sumY / validCount);
        centroid.valid = true;
        centroid.timestamp = millis();
    }
    
    return centroid;
}

bool TouchHAL::isPalmTouch(const std::vector<TouchPoint>& points) {
    if (points.size() < 3) {
        return false; // Need at least 3 points for palm detection
    }
    
    // Calculate the area covered by touch points
    int minX = INT_MAX, maxX = INT_MIN;
    int minY = INT_MAX, maxY = INT_MIN;
    int validPoints = 0;
    
    for (const auto& point : points) {
        if (point.valid) {
            minX = std::min(minX, (int)point.x);
            maxX = std::max(maxX, (int)point.x);
            minY = std::min(minY, (int)point.y);
            maxY = std::max(maxY, (int)point.y);
            validPoints++;
        }
    }
    
    if (validPoints < 3) {
        return false;
    }
    
    // Palm rejection criteria:
    // 1. Large contact area
    int contactWidth = maxX - minX;
    int contactHeight = maxY - minY;
    int contactArea = contactWidth * contactHeight;
    
    // 2. Many simultaneous touch points
    bool largePressureArea = (contactArea > m_palmRejectionThreshold * 50);
    bool manyTouchPoints = (validPoints >= 4);
    
    // 3. Low pressure variation (palms have more uniform pressure)
    bool lowPressureVariation = true;
    if (validPoints > 1) {
        float avgPressure = 0;
        for (const auto& point : points) {
            if (point.valid) {
                avgPressure += point.pressure;
            }
        }
        avgPressure /= validPoints;
        
        float pressureVariance = 0;
        for (const auto& point : points) {
            if (point.valid) {
                float diff = point.pressure - avgPressure;
                pressureVariance += diff * diff;
            }
        }
        pressureVariance /= validPoints;
        
        lowPressureVariation = (pressureVariance < 100); // Low variance threshold
    }
    
    return (largePressureArea && manyTouchPoints) || 
           (manyTouchPoints && lowPressureVariation);
}

bool TouchHAL::readGT911Register(uint16_t regAddr, uint8_t* data, uint8_t length) {
    Wire.beginTransmission(GT911_I2C_ADDR);
    Wire.write((regAddr >> 8) & 0xFF);  // High byte
    Wire.write(regAddr & 0xFF);         // Low byte
    
    if (Wire.endTransmission() != 0) {
        ESP_LOGW(TAG, "Failed to write GT911 register address 0x%04X", regAddr);
        return false;
    }
    
    Wire.requestFrom(GT911_I2C_ADDR, length);
    
    uint8_t bytesRead = 0;
    while (Wire.available() && bytesRead < length) {
        data[bytesRead++] = Wire.read();
    }
    
    if (bytesRead != length) {
        ESP_LOGW(TAG, "GT911 read incomplete: got %d bytes, expected %d", bytesRead, length);
        return false;
    }
    
    return true;
}

bool TouchHAL::writeGT911Register(uint16_t regAddr, const uint8_t* data, uint8_t length) {
    Wire.beginTransmission(GT911_I2C_ADDR);
    Wire.write((regAddr >> 8) & 0xFF);  // High byte
    Wire.write(regAddr & 0xFF);         // Low byte
    
    for (uint8_t i = 0; i < length; i++) {
        Wire.write(data[i]);
    }
    
    if (Wire.endTransmission() != 0) {
        ESP_LOGW(TAG, "Failed to write GT911 register 0x%04X", regAddr);
        return false;
    }
    
    return true;
}