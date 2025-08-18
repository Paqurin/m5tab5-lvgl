#ifndef BLUETOOTH_SERVICE_H
#define BLUETOOTH_SERVICE_H

#include "../system/os_config.h"
#include "../hal/bluetooth_hal.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <freertos/event_groups.h>
#include <vector>
#include <string>
#include <functional>

/**
 * @file bluetooth_service.h
 * @brief High-level Bluetooth Service for M5Stack Tab5
 * 
 * Provides unified Bluetooth device management with automatic pairing workflows,
 * device profiles, and integration with the OS input/audio systems.
 * Handles HID input routing, A2DP audio streaming, and device state persistence.
 */

// Service event types
enum class BluetoothServiceEvent {
    DISCOVERY_STARTED,
    DISCOVERY_COMPLETED,
    DEVICE_FOUND,
    DEVICE_CONNECTED,
    DEVICE_DISCONNECTED,
    PAIRING_STARTED,
    PAIRING_SUCCESS,
    PAIRING_FAILED,
    KEYBOARD_INPUT,
    MOUSE_INPUT,
    AUDIO_STARTED,
    AUDIO_STOPPED,
    ERROR_OCCURRED
};

// Device profile configurations
struct DeviceProfile {
    std::string address;
    std::string name;
    BluetoothDeviceType type;
    bool autoConnect = false;
    bool trustedDevice = false;
    uint32_t lastConnected = 0;
    uint32_t connectionCount = 0;
    std::string customName;     // User-assigned name
    uint8_t preferredVolume = 64; // For audio devices
};

// Pairing workflow states
enum class PairingState {
    IDLE,
    SCANNING,
    PIN_REQUIRED,
    AUTHENTICATING,
    SUCCESS,
    FAILED,
    TIMEOUT
};

// Service configuration
struct BluetoothServiceConfig {
    bool autoDiscovery = false;         // Auto-discover on startup
    bool autoConnect = true;            // Auto-connect to known devices
    uint32_t discoveryInterval = 30000; // Auto-discovery interval (ms)
    uint32_t connectionTimeout = 15000; // Connection timeout (ms)
    uint32_t pairingTimeout = 30000;    // Pairing timeout (ms)
    uint8_t maxPairedDevices = 16;      // Maximum paired devices
    bool persistDevices = true;         // Save device profiles to storage
    std::string deviceName = "M5Stack Tab5"; // Local device name
};

// Service event notification
struct BluetoothServiceEventData {
    BluetoothServiceEvent event;
    std::string deviceAddress;
    std::string deviceName;
    BluetoothDeviceType deviceType;
    int32_t rssi = 0;
    std::string errorMessage;
    uint32_t timestamp = 0;
};

/**
 * @brief High-level Bluetooth Service
 * Manages device profiles, pairing workflows, and system integration
 */
class BluetoothService {
public:
    BluetoothService();
    ~BluetoothService();

    /**
     * @brief Initialize Bluetooth service
     * @param config Service configuration
     * @return OS_OK on success, error code on failure
     */
    os_error_t initialize(const BluetoothServiceConfig& config = BluetoothServiceConfig{});

    /**
     * @brief Shutdown Bluetooth service
     * @return OS_OK on success, error code on failure
     */
    os_error_t shutdown();

    /**
     * @brief Update service (call from main loop)
     * @param deltaTime Time since last update in milliseconds
     * @return OS_OK on success, error code on failure
     */
    os_error_t update(uint32_t deltaTime);

    /**
     * @brief Check if service is initialized
     * @return true if service is initialized
     */
    bool isInitialized() const { return m_initialized; }

    // === Device Management ===

    /**
     * @brief Start device discovery
     * @param durationSeconds Discovery duration
     * @return OS_OK on success, error code on failure
     */
    os_error_t startDeviceDiscovery(uint32_t durationSeconds = 30);

    /**
     * @brief Stop device discovery
     * @return OS_OK on success, error code on failure
     */
    os_error_t stopDeviceDiscovery();

    /**
     * @brief Get discovered devices
     * @return Vector of discovered devices
     */
    std::vector<BluetoothDeviceInfo> getDiscoveredDevices() const;

    /**
     * @brief Get device profiles (paired devices)
     * @return Vector of device profiles
     */
    std::vector<DeviceProfile> getDeviceProfiles() const { return m_deviceProfiles; }

    /**
     * @brief Get connected devices
     * @return Vector of connected devices
     */
    std::vector<BluetoothDeviceInfo> getConnectedDevices() const;

    // === Pairing and Connection ===

    /**
     * @brief Start device pairing workflow
     * @param deviceAddress Device MAC address
     * @return OS_OK on success, error code on failure
     */
    os_error_t startPairing(const std::string& deviceAddress);

    /**
     * @brief Cancel ongoing pairing
     * @return OS_OK on success, error code on failure
     */
    os_error_t cancelPairing();

    /**
     * @brief Get current pairing state
     * @return Current pairing state
     */
    PairingState getPairingState() const { return m_pairingState; }

    /**
     * @brief Connect to paired device
     * @param deviceAddress Device MAC address
     * @return OS_OK on success, error code on failure
     */
    os_error_t connectDevice(const std::string& deviceAddress);

    /**
     * @brief Disconnect device
     * @param deviceAddress Device MAC address
     * @return OS_OK on success, error code on failure
     */
    os_error_t disconnectDevice(const std::string& deviceAddress);

    /**
     * @brief Remove device profile (unpair)
     * @param deviceAddress Device MAC address
     * @return OS_OK on success, error code on failure
     */
    os_error_t removeDevice(const std::string& deviceAddress);

    /**
     * @brief Set device auto-connect
     * @param deviceAddress Device MAC address
     * @param autoConnect Enable auto-connect
     * @return OS_OK on success, error code on failure
     */
    os_error_t setDeviceAutoConnect(const std::string& deviceAddress, bool autoConnect);

    /**
     * @brief Set device trusted status
     * @param deviceAddress Device MAC address
     * @param trusted Trusted device status
     * @return OS_OK on success, error code on failure
     */
    os_error_t setDeviceTrusted(const std::string& deviceAddress, bool trusted);

    /**
     * @brief Set custom device name
     * @param deviceAddress Device MAC address
     * @param customName Custom name
     * @return OS_OK on success, error code on failure
     */
    os_error_t setDeviceCustomName(const std::string& deviceAddress, const std::string& customName);

    // === HID Device Support ===

    /**
     * @brief Check if keyboard is connected and active
     * @return true if keyboard is active
     */
    bool isKeyboardActive() const { return m_keyboardActive; }

    /**
     * @brief Check if mouse is connected and active
     * @return true if mouse is active
     */
    bool isMouseActive() const { return m_mouseActive; }

    /**
     * @brief Get latest keyboard state
     * @param modifier Modifier keys state
     * @param keys Array of pressed keys (6 bytes)
     * @return true if keyboard data available
     */
    bool getKeyboardState(uint8_t& modifier, uint8_t keys[6]);

    /**
     * @brief Get mouse movement delta since last call
     * @param deltaX X movement delta
     * @param deltaY Y movement delta
     * @param buttons Button state
     * @param wheel Scroll wheel delta
     * @return true if mouse data available
     */
    bool getMouseDelta(int16_t& deltaX, int16_t& deltaY, uint8_t& buttons, int8_t& wheel);

    // === Audio Device Support ===

    /**
     * @brief Check if audio device is connected
     * @return true if audio device is connected
     */
    bool isAudioDeviceActive() const { return m_audioDeviceActive; }

    /**
     * @brief Get audio device information
     * @return Audio device information
     */
    AudioDeviceInfo getAudioDeviceInfo() const;

    /**
     * @brief Control audio playback
     * @param play true to play, false to pause
     * @return OS_OK on success, error code on failure
     */
    os_error_t controlAudio(bool play);

    /**
     * @brief Set audio volume for connected device
     * @param volume Volume level (0-100)
     * @return OS_OK on success, error code on failure
     */
    os_error_t setAudioVolume(uint8_t volume);

    /**
     * @brief Send media control command
     * @param command Media command (play, pause, next, previous, etc.)
     * @return OS_OK on success, error code on failure
     */
    os_error_t sendMediaCommand(const std::string& command);

    // === Event Handling ===

    /**
     * @brief Register service event callback
     * @param callback Event callback function
     */
    void setEventCallback(std::function<void(const BluetoothServiceEventData&)> callback) {
        m_eventCallback = callback;
    }

    // === Configuration and Status ===

    /**
     * @brief Get service configuration
     * @return Current service configuration
     */
    BluetoothServiceConfig getConfiguration() const { return m_config; }

    /**
     * @brief Update service configuration
     * @param config New configuration
     * @return OS_OK on success, error code on failure
     */
    os_error_t setConfiguration(const BluetoothServiceConfig& config);

    /**
     * @brief Get service statistics
     */
    void getServiceStats(uint32_t& totalPairedDevices, uint32_t& activeConnections,
                        uint32_t& totalConnectionAttempts, uint32_t& inputEvents);

    /**
     * @brief Reset service statistics
     */
    void resetStats();

    /**
     * @brief Print service status
     */
    void printStatus() const;

    // === Storage Management ===

    /**
     * @brief Save device profiles to storage
     * @return OS_OK on success, error code on failure
     */
    os_error_t saveDeviceProfiles();

    /**
     * @brief Load device profiles from storage
     * @return OS_OK on success, error code on failure
     */
    os_error_t loadDeviceProfiles();

    /**
     * @brief Clear all stored device profiles
     * @return OS_OK on success, error code on failure
     */
    os_error_t clearStoredProfiles();

private:
    /**
     * @brief Service management task
     * @param parameter Task parameter
     */
    static void serviceTask(void* parameter);

    /**
     * @brief Handle Bluetooth HAL events
     * @param device Device information
     * @param state Connection state
     */
    void handleConnectionStateChange(const BluetoothDeviceInfo& device, BluetoothConnectionState state);

    /**
     * @brief Handle device discovery
     * @param device Discovered device
     */
    void handleDeviceDiscovery(const BluetoothDeviceInfo& device);

    /**
     * @brief Handle HID keyboard input
     * @param report Keyboard report
     */
    void handleKeyboardInput(const HIDKeyboardReport& report);

    /**
     * @brief Handle HID mouse input
     * @param report Mouse report
     */
    void handleMouseInput(const HIDMouseReport& report);

    /**
     * @brief Update device profiles with connection info
     * @param address Device address
     * @param connected Connection status
     */
    void updateDeviceProfile(const std::string& address, bool connected);

    /**
     * @brief Find device profile by address
     * @param address Device address
     * @return Pointer to device profile or nullptr
     */
    DeviceProfile* findDeviceProfile(const std::string& address);

    /**
     * @brief Attempt auto-connect to trusted devices
     */
    void attemptAutoConnect();

    /**
     * @brief Send service event notification
     * @param event Event type
     * @param address Device address (optional)
     * @param name Device name (optional)
     * @param type Device type (optional)
     * @param errorMsg Error message (optional)
     */
    void sendEvent(BluetoothServiceEvent event, const std::string& address = "",
                  const std::string& name = "", BluetoothDeviceType type = BluetoothDeviceType::UNKNOWN,
                  const std::string& errorMsg = "");

    /**
     * @brief Process pairing workflow
     */
    void processPairingWorkflow();

    /**
     * @brief Check and handle connection timeouts
     */
    void checkConnectionTimeouts();

    // Core components
    BluetoothHAL m_bluetoothHAL;
    BluetoothServiceConfig m_config;

    // Service state
    bool m_initialized = false;
    bool m_discoveryActive = false;
    PairingState m_pairingState = PairingState::IDLE;
    std::string m_currentPairingDevice;
    uint32_t m_pairingStartTime = 0;

    // Device state
    bool m_keyboardActive = false;
    bool m_mouseActive = false;
    bool m_audioDeviceActive = false;

    // Device management
    std::vector<DeviceProfile> m_deviceProfiles;
    std::vector<BluetoothDeviceInfo> m_discoveredDevices;

    // Input state tracking
    HIDKeyboardReport m_lastKeyboardReport;
    HIDMouseReport m_lastMouseReport;
    int16_t m_mouseDeltaX = 0;
    int16_t m_mouseDeltaY = 0;
    int8_t m_mouseWheelDelta = 0;
    bool m_keyboardDataAvailable = false;
    bool m_mouseDataAvailable = false;

    // Threading and synchronization
    TaskHandle_t m_serviceTask = nullptr;
    QueueHandle_t m_eventQueue = nullptr;
    EventGroupHandle_t m_eventGroup = nullptr;

    // Event handling
    std::function<void(const BluetoothServiceEventData&)> m_eventCallback = nullptr;

    // Timing and intervals
    uint32_t m_lastDiscoveryTime = 0;
    uint32_t m_lastAutoConnectTime = 0;
    uint32_t m_lastUpdateTime = 0;

    // Statistics
    uint32_t m_totalConnectionAttempts = 0;
    uint32_t m_inputEvents = 0;

    // Configuration
    static constexpr uint32_t SERVICE_TASK_STACK_SIZE = 4096;
    static constexpr uint32_t EVENT_QUEUE_SIZE = 16;
    static constexpr uint32_t AUTO_CONNECT_INTERVAL = 60000; // 60 seconds
    static constexpr uint32_t MAX_DISCOVERY_DEVICES = 50;
    static constexpr const char* TAG = "BluetoothService";
    static constexpr const char* STORAGE_NAMESPACE = "bt_service";
    static constexpr const char* PROFILES_KEY = "profiles";

    // Event group bits
    static constexpr int DISCOVERY_COMPLETE_BIT = BIT0;
    static constexpr int PAIRING_COMPLETE_BIT = BIT1;
    static constexpr int CONNECTION_CHANGE_BIT = BIT2;
};

#endif // BLUETOOTH_SERVICE_H