#ifndef BLUETOOTH_HAL_H
#define BLUETOOTH_HAL_H

#include "../system/os_config.h"
#include "hardware_config.h"
#include <driver/gpio.h>
#include <driver/sdio_slave.h>
#include <esp_log.h>
#include <vector>
#include <string>
#include <functional>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>

/**
 * @file bluetooth_hal.h
 * @brief Bluetooth Hardware Abstraction Layer for M5Stack Tab5
 * 
 * Bluetooth support via ESP32-C6 companion chip communication.
 * Handles HID devices (keyboards, mice), A2DP audio streaming (headphones),
 * and Bluetooth serial communication through SDIO interface.
 */

// Bluetooth device types
enum class BluetoothDeviceType {
    UNKNOWN,
    KEYBOARD,
    MOUSE,
    COMBO_HID,      // Keyboard + Mouse combo
    HEADPHONES,
    SPEAKERS,
    GAMEPAD,
    SERIAL_PORT,
    PHONE
};

// Bluetooth connection states
enum class BluetoothConnectionState {
    DISCONNECTED,
    SCANNING,
    CONNECTING,
    CONNECTED,
    PAIRED,
    AUTHENTICATING,
    ERROR
};

// HID report types
enum class HIDReportType {
    KEYBOARD,
    MOUSE,
    CONSUMER,       // Media keys
    GAMEPAD
};

// Bluetooth device information
struct BluetoothDeviceInfo {
    std::string address;            // MAC address (XX:XX:XX:XX:XX:XX)
    std::string name;               // Device friendly name
    BluetoothDeviceType type;       // Device type classification
    int32_t rssi;                  // Signal strength
    bool paired;                   // Is device paired
    bool connected;                // Is device connected
    uint32_t deviceClass;          // Bluetooth class of device
    std::vector<uint8_t> serviceUUIDs; // Supported service UUIDs
    uint32_t lastSeen;             // Last time device was seen
};

// HID keyboard report structure
struct HIDKeyboardReport {
    uint8_t modifier;              // Modifier keys (Ctrl, Alt, Shift, etc.)
    uint8_t reserved;              // Reserved byte
    uint8_t keys[6];               // Up to 6 simultaneous keys
    uint32_t timestamp;            // Report timestamp
};

// HID mouse report structure
struct HIDMouseReport {
    uint8_t buttons;               // Mouse button state
    int16_t x;                     // X movement delta
    int16_t y;                     // Y movement delta
    int8_t wheel;                  // Scroll wheel delta
    int8_t hWheel;                 // Horizontal scroll wheel delta
    uint32_t timestamp;            // Report timestamp
};

// Audio codec information
struct AudioCodecInfo {
    uint8_t type;                  // Codec type (SBC, AAC, APTX, etc.)
    uint32_t sampleRate;           // Sample rate (Hz)
    uint8_t channels;              // Number of channels
    uint8_t bitDepth;              // Bit depth
    uint32_t bitrate;              // Bitrate (bps)
};

// Audio device information
struct AudioDeviceInfo {
    std::string deviceAddress;     // Device MAC address
    std::string deviceName;        // Device name
    AudioCodecInfo codecInfo;      // Codec information
    bool isPlaying;               // Is currently playing audio
    uint8_t volume;               // Current volume (0-127)
    uint32_t connectionTime;      // When connected
};

// ESP32-C6 Communication Commands
enum class C6Command : uint8_t {
    // System commands
    PING = 0x01,
    RESET = 0x02,
    GET_STATUS = 0x03,
    
    // Bluetooth commands
    BT_ENABLE = 0x10,
    BT_DISABLE = 0x11,
    BT_START_DISCOVERY = 0x12,
    BT_STOP_DISCOVERY = 0x13,
    BT_PAIR_DEVICE = 0x14,
    BT_UNPAIR_DEVICE = 0x15,
    BT_CONNECT_DEVICE = 0x16,
    BT_DISCONNECT_DEVICE = 0x17,
    BT_GET_PAIRED_DEVICES = 0x18,
    BT_GET_CONNECTED_DEVICES = 0x19,
    
    // HID commands
    HID_ENABLE_HOST = 0x20,
    HID_DISABLE_HOST = 0x21,
    HID_GET_REPORT = 0x22,
    
    // Audio commands
    AUDIO_ENABLE_A2DP = 0x30,
    AUDIO_DISABLE_A2DP = 0x31,
    AUDIO_PLAY = 0x32,
    AUDIO_PAUSE = 0x33,
    AUDIO_SET_VOLUME = 0x34,
    AUDIO_MEDIA_COMMAND = 0x35,
    
    // Serial commands
    SERIAL_ENABLE = 0x40,
    SERIAL_DISABLE = 0x41,
    SERIAL_SEND_DATA = 0x42
};

// ESP32-C6 Communication Events
enum class C6Event : uint8_t {
    // System events
    READY = 0x81,
    ERROR = 0x82,
    
    // Bluetooth events
    BT_DEVICE_DISCOVERED = 0x90,
    BT_DEVICE_CONNECTED = 0x91,
    BT_DEVICE_DISCONNECTED = 0x92,
    BT_PAIRING_SUCCESS = 0x93,
    BT_PAIRING_FAILED = 0x94,
    BT_DISCOVERY_COMPLETE = 0x95,
    
    // HID events
    HID_KEYBOARD_REPORT = 0xA0,
    HID_MOUSE_REPORT = 0xA1,
    HID_DEVICE_CONNECTED = 0xA2,
    HID_DEVICE_DISCONNECTED = 0xA3,
    
    // Audio events
    AUDIO_CONNECTED = 0xB0,
    AUDIO_DISCONNECTED = 0xB1,
    AUDIO_PLAYING = 0xB2,
    AUDIO_PAUSED = 0xB3,
    AUDIO_VOLUME_CHANGED = 0xB4,
    
    // Serial events
    SERIAL_DATA_RECEIVED = 0xC0,
    SERIAL_CONNECTED = 0xC1,
    SERIAL_DISCONNECTED = 0xC2
};

// Command/Event packet structure
struct C6Packet {
    uint8_t sync;                  // Sync byte (0xAA)
    uint8_t command;               // Command or event code
    uint16_t length;               // Data length
    uint8_t data[256];             // Variable length data
    uint8_t checksum;              // Simple checksum
};

// Bluetooth callbacks
typedef std::function<void(const BluetoothDeviceInfo&)> DeviceDiscoveryCallback;
typedef std::function<void(const BluetoothDeviceInfo&, BluetoothConnectionState)> ConnectionStateCallback;
typedef std::function<void(const HIDKeyboardReport&)> KeyboardReportCallback;
typedef std::function<void(const HIDMouseReport&)> MouseReportCallback;
typedef std::function<void(const AudioDeviceInfo&, const uint8_t*, size_t)> AudioDataCallback;
typedef std::function<void(const std::string&, const std::string&)> SerialDataCallback;

/**
 * @brief Bluetooth Hardware Abstraction Layer
 * Manages Bluetooth functionality via ESP32-C6 companion chip
 */
class BluetoothHAL {
public:
    BluetoothHAL();
    ~BluetoothHAL();

    /**
     * @brief Initialize Bluetooth HAL
     * @return OS_OK on success, error code on failure
     */
    os_error_t initialize();

    /**
     * @brief Shutdown Bluetooth HAL
     * @return OS_OK on success, error code on failure
     */
    os_error_t shutdown();

    /**
     * @brief Enable/disable Bluetooth radio
     * @param enable Enable or disable
     * @return OS_OK on success, error code on failure
     */
    os_error_t enableBluetooth(bool enable);

    /**
     * @brief Check if Bluetooth is enabled
     * @return true if Bluetooth is enabled
     */
    bool isBluetoothEnabled() const { return m_bluetoothEnabled; }

    /**
     * @brief Start device discovery scan
     * @param durationSeconds Scan duration in seconds
     * @return OS_OK on success, error code on failure
     */
    os_error_t startDiscovery(uint32_t durationSeconds = 10);

    /**
     * @brief Stop device discovery scan
     * @return OS_OK on success, error code on failure
     */
    os_error_t stopDiscovery();

    /**
     * @brief Check if discovery is active
     * @return true if scanning for devices
     */
    bool isDiscovering() const { return m_discovering; }

    /**
     * @brief Get list of discovered devices
     * @return Vector of discovered device information
     */
    std::vector<BluetoothDeviceInfo> getDiscoveredDevices() const { return m_discoveredDevices; }

    /**
     * @brief Get list of paired devices
     * @return Vector of paired device information
     */
    std::vector<BluetoothDeviceInfo> getPairedDevices() const { return m_pairedDevices; }

    /**
     * @brief Pair with a discovered device
     * @param deviceAddress Device MAC address
     * @return OS_OK on success, error code on failure
     */
    os_error_t pairDevice(const std::string& deviceAddress);

    /**
     * @brief Unpair a device
     * @param deviceAddress Device MAC address
     * @return OS_OK on success, error code on failure
     */
    os_error_t unpairDevice(const std::string& deviceAddress);

    /**
     * @brief Connect to a paired device
     * @param deviceAddress Device MAC address
     * @return OS_OK on success, error code on failure
     */
    os_error_t connectDevice(const std::string& deviceAddress);

    /**
     * @brief Disconnect from a device
     * @param deviceAddress Device MAC address
     * @return OS_OK on success, error code on failure
     */
    os_error_t disconnectDevice(const std::string& deviceAddress);

    /**
     * @brief Get connected devices
     * @return Vector of connected device information
     */
    std::vector<BluetoothDeviceInfo> getConnectedDevices() const;

    // === HID Device Support ===

    /**
     * @brief Enable HID host mode
     * @return OS_OK on success, error code on failure
     */
    os_error_t enableHIDHost();

    /**
     * @brief Check if HID keyboard is connected
     * @return true if keyboard is connected
     */
    bool isKeyboardConnected() const { return m_keyboardConnected; }

    /**
     * @brief Check if HID mouse is connected
     * @return true if mouse is connected
     */
    bool isMouseConnected() const { return m_mouseConnected; }

    /**
     * @brief Get latest keyboard report
     * @param report Output keyboard report
     * @return OS_OK if report available, error code otherwise
     */
    os_error_t getKeyboardReport(HIDKeyboardReport& report);

    /**
     * @brief Get latest mouse report
     * @param report Output mouse report
     * @return OS_OK if report available, error code otherwise
     */
    os_error_t getMouseReport(HIDMouseReport& report);

    // === Audio Device Support ===

    /**
     * @brief Enable A2DP sink (audio receiver) mode
     * @return OS_OK on success, error code on failure
     */
    os_error_t enableA2DPSink();

    /**
     * @brief Check if audio device is connected
     * @return true if audio device is connected
     */
    bool isAudioDeviceConnected() const { return m_audioDeviceConnected; }

    /**
     * @brief Get connected audio device information
     * @return Audio device information
     */
    AudioDeviceInfo getAudioDeviceInfo() const { return m_audioDeviceInfo; }

    /**
     * @brief Control audio playback
     * @param play true to play, false to pause
     * @return OS_OK on success, error code on failure
     */
    os_error_t controlAudioPlayback(bool play);

    /**
     * @brief Set audio volume
     * @param volume Volume level (0-127)
     * @return OS_OK on success, error code on failure
     */
    os_error_t setAudioVolume(uint8_t volume);

    /**
     * @brief Send media command (next, previous, etc.)
     * @param command Media command string
     * @return OS_OK on success, error code on failure
     */
    os_error_t sendMediaCommand(const std::string& command);

    // === Serial Communication ===

    /**
     * @brief Enable Bluetooth serial
     * @param deviceName Local device name for serial
     * @return OS_OK on success, error code on failure
     */
    os_error_t enableSerial(const std::string& deviceName = "M5Stack-Tab5");

    /**
     * @brief Send data via Bluetooth serial
     * @param data Data to send
     * @param length Data length
     * @return OS_OK on success, error code on failure
     */
    os_error_t sendSerialData(const uint8_t* data, size_t length);

    /**
     * @brief Check if serial data is available
     * @return Number of bytes available
     */
    size_t serialAvailable() const;

    /**
     * @brief Read serial data
     * @param buffer Output buffer
     * @param maxLength Maximum bytes to read
     * @return Number of bytes read
     */
    size_t readSerialData(uint8_t* buffer, size_t maxLength);

    // === Callback Registration ===

    /**
     * @brief Register device discovery callback
     * @param callback Callback function
     */
    void setDeviceDiscoveryCallback(DeviceDiscoveryCallback callback) {
        m_discoveryCallback = callback;
    }

    /**
     * @brief Register connection state callback
     * @param callback Callback function
     */
    void setConnectionStateCallback(ConnectionStateCallback callback) {
        m_connectionCallback = callback;
    }

    /**
     * @brief Register keyboard report callback
     * @param callback Callback function
     */
    void setKeyboardReportCallback(KeyboardReportCallback callback) {
        m_keyboardCallback = callback;
    }

    /**
     * @brief Register mouse report callback
     * @param callback Callback function
     */
    void setMouseReportCallback(MouseReportCallback callback) {
        m_mouseCallback = callback;
    }

    /**
     * @brief Register audio data callback
     * @param callback Callback function
     */
    void setAudioDataCallback(AudioDataCallback callback) {
        m_audioCallback = callback;
    }

    /**
     * @brief Register serial data callback
     * @param callback Callback function
     */
    void setSerialDataCallback(SerialDataCallback callback) {
        m_serialCallback = callback;
    }

    // === Utility Functions ===

    /**
     * @brief Get Bluetooth statistics
     */
    void getBluetoothStats(uint32_t& devicesDiscovered, uint32_t& connectionAttempts, 
                          uint32_t& successfulConnections, uint32_t& dataPacketsReceived);

    /**
     * @brief Reset Bluetooth statistics
     */
    void resetStats();

    /**
     * @brief Print Bluetooth status
     */
    void printStatus() const;

    /**
     * @brief Convert device class to device type
     * @param deviceClass Bluetooth device class
     * @return Device type classification
     */
    static BluetoothDeviceType classifyDevice(uint32_t deviceClass);

    /**
     * @brief Format MAC address string
     * @param address Binary MAC address
     * @return Formatted MAC address string
     */
    static std::string formatMACAddress(const uint8_t* address);

private:
    // ESP32-C6 Communication
    os_error_t initializeC6Communication();
    os_error_t resetC6Chip();
    os_error_t sendC6Command(C6Command command, const uint8_t* data = nullptr, uint16_t length = 0);
    os_error_t receiveC6Event(C6Packet& packet, uint32_t timeoutMs = 1000);
    void processC6Event(const C6Packet& packet);
    uint8_t calculateChecksum(const C6Packet& packet);
    bool validatePacket(const C6Packet& packet);

    // SDIO Communication
    os_error_t initializeSDIO();
    os_error_t shutdownSDIO();
    static void sdioEventTask(void* parameter);
    void handleSDIOEvents();

    // Device management
    void addDiscoveredDevice(const BluetoothDeviceInfo& device);
    void updateDeviceConnection(const std::string& address, BluetoothConnectionState state);
    BluetoothDeviceInfo* findDevice(const std::string& address);
    void processHIDReport(const uint8_t* data, size_t length, HIDReportType type);

    // Hardware control
    os_error_t configurePinSettings();

    // ESP32-C6 state
    bool m_initialized = false;
    bool m_c6Ready = false;
    bool m_bluetoothEnabled = false;
    bool m_discovering = false;
    bool m_hidHostEnabled = false;
    bool m_a2dpSinkEnabled = false;
    bool m_serialEnabled = false;

    // Device connection states
    bool m_keyboardConnected = false;
    bool m_mouseConnected = false;
    bool m_audioDeviceConnected = false;

    // Device lists
    std::vector<BluetoothDeviceInfo> m_discoveredDevices;
    std::vector<BluetoothDeviceInfo> m_pairedDevices;

    // Latest HID reports
    HIDKeyboardReport m_latestKeyboardReport;
    HIDMouseReport m_latestMouseReport;
    bool m_keyboardReportAvailable = false;
    bool m_mouseReportAvailable = false;

    // Audio device information
    AudioDeviceInfo m_audioDeviceInfo;

    // Serial data buffer
    std::vector<uint8_t> m_serialBuffer;
    SemaphoreHandle_t m_serialMutex = nullptr;

    // SDIO communication
    TaskHandle_t m_sdioTask = nullptr;
    QueueHandle_t m_eventQueue = nullptr;
    SemaphoreHandle_t m_c6Mutex = nullptr;

    // Callbacks
    DeviceDiscoveryCallback m_discoveryCallback = nullptr;
    ConnectionStateCallback m_connectionCallback = nullptr;
    KeyboardReportCallback m_keyboardCallback = nullptr;
    MouseReportCallback m_mouseCallback = nullptr;
    AudioDataCallback m_audioCallback = nullptr;
    SerialDataCallback m_serialCallback = nullptr;

    // Statistics
    uint32_t m_devicesDiscovered = 0;
    uint32_t m_connectionAttempts = 0;
    uint32_t m_successfulConnections = 0;
    uint32_t m_dataPacketsReceived = 0;

    // Configuration
    uint32_t m_discoveryDuration = 10; // seconds
    static constexpr const char* TAG = "BluetoothHAL";
    
    // Communication protocol constants
    static constexpr uint8_t SYNC_BYTE = 0xAA;
    static constexpr uint32_t C6_RESPONSE_TIMEOUT = 5000; // 5 seconds
    static constexpr uint32_t SDIO_QUEUE_SIZE = 32;
    static constexpr uint32_t MAX_PACKET_SIZE = 512;
    
    // Static instance for event handlers
    static BluetoothHAL* s_instance;
};

#endif // BLUETOOTH_HAL_H