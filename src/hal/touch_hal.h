#ifndef TOUCH_HAL_H
#define TOUCH_HAL_H

#include "../system/os_config.h"
#include <vector>

/**
 * @file touch_hal.h
 * @brief Touch Hardware Abstraction Layer for M5Stack Tab5
 * 
 * Manages the GT911 touch controller with multi-touch support
 * and gesture recognition capabilities.
 */

struct TouchPoint {
    uint16_t x;
    uint16_t y;
    uint8_t pressure;
    uint8_t id;
    bool valid;
    uint32_t timestamp;
    
    TouchPoint() : x(0), y(0), pressure(0), id(0), valid(false), timestamp(0) {}
};

enum class TouchEvent {
    PRESS,
    RELEASE,
    MOVE,
    GESTURE
};

enum class GestureType {
    NONE,
    TAP,
    DOUBLE_TAP,
    LONG_PRESS,
    SWIPE_UP,
    SWIPE_DOWN,
    SWIPE_LEFT,
    SWIPE_RIGHT,
    PINCH_IN,
    PINCH_OUT,
    ROTATE,
    // Multi-touch gestures
    TWO_FINGER_TAP,
    THREE_FINGER_TAP,
    FOUR_FINGER_TAP,
    FIVE_FINGER_TAP,
    TWO_FINGER_SWIPE_UP,
    TWO_FINGER_SWIPE_DOWN,
    THREE_FINGER_SWIPE_LEFT,
    THREE_FINGER_SWIPE_RIGHT,
    FIVE_FINGER_PINCH,
    PALM_REJECTION
};

struct TouchEventData {
    TouchEvent event;
    TouchPoint point;
    GestureType gesture;
    uint32_t timestamp;
    
    // Multi-touch support
    std::vector<TouchPoint> allTouchPoints;
    uint8_t touchCount;
    
    // Gesture parameters
    float gestureDistance;  // For pinch/zoom gestures
    float gestureAngle;     // For rotation gestures
    float gestureVelocity;  // For swipe gestures
    
    TouchEventData() : event(TouchEvent::PRESS), gesture(GestureType::NONE), 
                       timestamp(0), touchCount(0), gestureDistance(0), 
                       gestureAngle(0), gestureVelocity(0) {}
};

typedef std::function<void(const TouchEventData&)> TouchCallback;

class TouchHAL {
public:
    TouchHAL() = default;
    ~TouchHAL();

    /**
     * @brief Initialize the touch controller
     * @return OS_OK on success, error code on failure
     */
    os_error_t initialize();

    /**
     * @brief Shutdown the touch controller
     * @return OS_OK on success, error code on failure
     */
    os_error_t shutdown();

    /**
     * @brief Update touch input (read from hardware and process)
     * @param deltaTime Time elapsed since last update in milliseconds
     * @return OS_OK on success, error code on failure
     */
    os_error_t update(uint32_t deltaTime);

    /**
     * @brief Perform touch controller self-test
     * @return true if test passes, false otherwise
     */
    bool selfTest();

    /**
     * @brief Set touch callback function
     * @param callback Function to call on touch events
     */
    void setTouchCallback(TouchCallback callback) { m_touchCallback = callback; }

    /**
     * @brief Enable/disable touch input
     * @param enabled True to enable, false to disable
     * @return OS_OK on success, error code on failure
     */
    os_error_t setEnabled(bool enabled);

    /**
     * @brief Check if touch is enabled
     * @return true if enabled, false if disabled
     */
    bool isEnabled() const { return m_enabled; }

    /**
     * @brief Set low power mode
     * @param enabled True to enable low power mode
     * @return OS_OK on success, error code on failure
     */
    os_error_t setLowPowerMode(bool enabled);

    /**
     * @brief Calibrate touch controller
     * @return OS_OK on success, error code on failure
     */
    os_error_t calibrate();

    /**
     * @brief Get current touch points
     * @return Vector of active touch points
     */
    const std::vector<TouchPoint>& getCurrentTouches() const { return m_currentTouches; }

    /**
     * @brief Get number of active touches
     * @return Number of active touch points
     */
    size_t getActiveTouchCount() const;

    /**
     * @brief Set touch sensitivity
     * @param sensitivity Sensitivity level (0-255)
     * @return OS_OK on success, error code on failure
     */
    os_error_t setSensitivity(uint8_t sensitivity);

    /**
     * @brief Get touch sensitivity
     * @return Current sensitivity level
     */
    uint8_t getSensitivity() const { return m_sensitivity; }

    /**
     * @brief Enable/disable gesture recognition
     * @param enabled True to enable gestures, false to disable
     */
    void setGestureEnabled(bool enabled) { m_gestureEnabled = enabled; }

    /**
     * @brief Check if gesture recognition is enabled
     * @return true if enabled, false if disabled
     */
    bool isGestureEnabled() const { return m_gestureEnabled; }

    /**
     * @brief Enable/disable multi-touch support
     * @param enabled True to enable multi-touch, false to disable
     */
    void setMultiTouchEnabled(bool enabled) { m_multiTouchEnabled = enabled; }

    /**
     * @brief Check if multi-touch is enabled
     * @return true if enabled, false if disabled
     */
    bool isMultiTouchEnabled() const { return m_multiTouchEnabled; }

    /**
     * @brief Set maximum number of simultaneous touches
     * @param maxTouches Maximum touches (1-10)
     */
    void setMaxTouches(uint8_t maxTouches);

    /**
     * @brief Get maximum number of simultaneous touches
     * @return Maximum touch count
     */
    uint8_t getMaxTouches() const { return m_maxTouches; }

    /**
     * @brief Set palm rejection threshold
     * @param threshold Palm rejection threshold (0-255)
     */
    void setPalmRejectionThreshold(uint8_t threshold) { m_palmRejectionThreshold = threshold; }

    /**
     * @brief Get touch statistics
     */
    void printStats() const;

private:
    /**
     * @brief Initialize GT911 touch controller
     * @return OS_OK on success, error code on failure
     */
    os_error_t initializeController();

    /**
     * @brief Read touch data from controller
     * @return OS_OK on success, error code on failure
     */
    os_error_t readTouchData();

    /**
     * @brief Process touch events and detect gestures
     */
    void processTouchEvents();

    /**
     * @brief Detect gestures from touch data
     * @return Detected gesture type
     */
    GestureType detectGesture();

    /**
     * @brief Detect multi-touch gestures
     * @return Detected multi-touch gesture type
     */
    GestureType detectMultiTouchGesture();

    /**
     * @brief Calculate distance between two touch points
     * @param p1 First touch point
     * @param p2 Second touch point
     * @return Distance in pixels
     */
    float calculateDistance(const TouchPoint& p1, const TouchPoint& p2);

    /**
     * @brief Calculate centroid of multiple touch points
     * @param points Touch points
     * @return Centroid point
     */
    TouchPoint calculateCentroid(const std::vector<TouchPoint>& points);

    /**
     * @brief Check if touch area indicates palm contact
     * @param points Touch points
     * @return true if palm detected
     */
    bool isPalmTouch(const std::vector<TouchPoint>& points);

    /**
     * @brief Filter touch points (debouncing, smoothing)
     */
    void filterTouchPoints();

    /**
     * @brief Send touch event via callback
     * @param eventData Touch event data to send
     */
    void sendTouchEvent(const TouchEventData& eventData);

    /**
     * @brief Read data from GT911 register
     * @param regAddr Register address
     * @param data Buffer to store read data
     * @param length Number of bytes to read
     * @return true on success, false on failure
     */
    bool readGT911Register(uint16_t regAddr, uint8_t* data, uint8_t length);

    /**
     * @brief Write data to GT911 register
     * @param regAddr Register address
     * @param data Data to write
     * @param length Number of bytes to write
     * @return true on success, false on failure
     */
    bool writeGT911Register(uint16_t regAddr, const uint8_t* data, uint8_t length);

    // Hardware configuration
    bool m_initialized = false;
    bool m_enabled = false;
    bool m_lowPowerMode = false;
    uint8_t m_sensitivity = 128;
    bool m_gestureEnabled = true;
    bool m_multiTouchEnabled = true;
    uint8_t m_maxTouches = 5;
    uint8_t m_palmRejectionThreshold = 200;

    // Touch data
    std::vector<TouchPoint> m_currentTouches;
    std::vector<TouchPoint> m_previousTouches;
    TouchCallback m_touchCallback;

    // Gesture detection
    uint32_t m_lastTouchTime = 0;
    uint32_t m_gestureStartTime = 0;
    TouchPoint m_gestureStartPoint;
    bool m_inGesture = false;
    
    // Multi-touch gesture tracking
    std::vector<TouchPoint> m_gestureStartPoints;
    float m_initialDistance = 0.0f;
    float m_initialAngle = 0.0f;
    TouchPoint m_initialCentroid;
    uint32_t m_multiTouchStartTime = 0;
    uint8_t m_maxSimultaneousTouches = 0;

    // Statistics
    uint32_t m_totalTouches = 0;
    uint32_t m_totalGestures = 0;
    uint32_t m_lastCalibration = 0;

    // GT911 specific
    static const uint8_t GT911_I2C_ADDR = 0x5D;
    static const uint16_t GT911_REG_STATUS = 0x814E;
    static const uint16_t GT911_REG_TOUCH = 0x814F;
};

#endif // TOUCH_HAL_H