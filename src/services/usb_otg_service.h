#ifndef USB_OTG_SERVICE_H
#define USB_OTG_SERVICE_H

#include "../system/os_config.h"
#include "../hal/hardware_config.h"
#include <vector>
#include <string>

/**
 * @file usb_otg_service.h
 * @brief USB-C OTG Service for M5Stack Tab5
 * 
 * Provides USB-C On-The-Go functionality with automatic host/device detection,
 * power delivery management, and device enumeration support.
 */

enum class USBOTGMode {
    NONE,
    DEVICE,         // Acting as USB device
    HOST,           // Acting as USB host
    AUTO            // Automatic detection based on ID pin
};

enum class USBOTGState {
    DISCONNECTED,
    CONNECTING,
    CONNECTED,
    ENUMERATING,
    READY,
    ERROR
};

enum class USBDeviceClass {
    UNKNOWN,
    STORAGE,        // Mass storage device
    HID,            // Human interface device (keyboard, mouse)
    AUDIO,          // Audio device
    VIDEO,          // Video device
    COMMUNICATION,  // Communication device (CDC)
    HUB,            // USB hub
    PRINTER,        // Printer
    CUSTOM          // Custom device class
};

struct USBDeviceInfo {
    uint16_t vendorId;
    uint16_t productId;
    uint16_t deviceVersion;
    USBDeviceClass deviceClass;
    std::string manufacturer;
    std::string product;
    std::string serialNumber;
    uint8_t configurationCount;
    uint16_t maxPower;        // mA
    bool selfPowered;
};

struct USBOTGConfig {
    USBOTGMode mode = USBOTGMode::AUTO;
    bool powerDeliveryEnabled = true;
    uint16_t maxOutputCurrent = 3000;  // mA
    uint16_t maxInputCurrent = 3000;   // mA
    bool overcurrentProtection = true;
    bool autoEnumeration = true;
    uint32_t enumerationTimeout = 5000; // ms
};

class USBOTGService {
public:
    USBOTGService() = default;
    ~USBOTGService();

    /**
     * @brief Initialize USB OTG service
     * @return OS_OK on success, error code on failure
     */
    os_error_t initialize();

    /**
     * @brief Shutdown USB OTG service
     * @return OS_OK on success, error code on failure
     */
    os_error_t shutdown();

    /**
     * @brief Update USB OTG service (check for device changes)
     * @param deltaTime Time since last update in milliseconds
     * @return OS_OK on success, error code on failure
     */
    os_error_t update(uint32_t deltaTime);

    /**
     * @brief Configure USB OTG settings
     * @param config USB OTG configuration
     * @return OS_OK on success, error code on failure
     */
    os_error_t configure(const USBOTGConfig& config);

    /**
     * @brief Set USB OTG mode
     * @param mode USB OTG mode
     * @return OS_OK on success, error code on failure
     */
    os_error_t setMode(USBOTGMode mode);

    /**
     * @brief Get current USB OTG mode
     * @return Current USB OTG mode
     */
    USBOTGMode getMode() const { return m_currentMode; }

    /**
     * @brief Get USB OTG state
     * @return Current USB OTG state
     */
    USBOTGState getState() const { return m_state; }

    /**
     * @brief Check if device is connected
     * @return true if device is connected
     */
    bool isDeviceConnected() const { return m_deviceConnected; }

    /**
     * @brief Get connected device information
     * @return Device information structure
     */
    const USBDeviceInfo& getDeviceInfo() const { return m_deviceInfo; }

    /**
     * @brief Enable/disable power delivery
     * @param enabled true to enable power delivery
     * @return OS_OK on success, error code on failure
     */
    os_error_t setPowerDelivery(bool enabled);

    /**
     * @brief Set maximum output current
     * @param current Maximum current in mA
     * @return OS_OK on success, error code on failure
     */
    os_error_t setMaxOutputCurrent(uint16_t current);

    /**
     * @brief Get current consumption
     * @return Current consumption in mA
     */
    uint16_t getCurrentConsumption() const { return m_currentConsumption; }

    /**
     * @brief Force device enumeration
     * @return OS_OK on success, error code on failure
     */
    os_error_t enumerateDevice();

    /**
     * @brief Get list of connected devices
     * @param devices Vector to store device list
     * @return OS_OK on success, error code on failure
     */
    os_error_t getDeviceList(std::vector<USBDeviceInfo>& devices);

    /**
     * @brief Get USB OTG statistics
     */
    void printUSBStats() const;

private:
    /**
     * @brief Initialize USB OTG hardware
     * @return OS_OK on success, error code on failure
     */
    os_error_t initializeHardware();

    /**
     * @brief Configure USB pins
     * @return OS_OK on success, error code on failure
     */
    os_error_t configurePins();

    /**
     * @brief Check OTG ID pin state
     * @return true if host mode detected
     */
    bool checkOTGID();

    /**
     * @brief Check VBUS state
     * @return true if VBUS is present
     */
    bool checkVBUS();

    /**
     * @brief Enable VBUS output
     * @param enabled true to enable VBUS
     * @return OS_OK on success, error code on failure
     */
    os_error_t enableVBUS(bool enabled);

    /**
     * @brief Handle device connection
     * @return OS_OK on success, error code on failure
     */
    os_error_t handleDeviceConnection();

    /**
     * @brief Handle device disconnection
     * @return OS_OK on success, error code on failure
     */
    os_error_t handleDeviceDisconnection();

    /**
     * @brief Enumerate connected device
     * @return OS_OK on success, error code on failure
     */
    os_error_t enumerateConnectedDevice();

    /**
     * @brief Parse device descriptor
     * @param descriptor Raw descriptor data
     * @param info Device information to fill
     * @return OS_OK on success, error code on failure
     */
    os_error_t parseDeviceDescriptor(const uint8_t* descriptor, USBDeviceInfo& info);

    /**
     * @brief Check for overcurrent condition
     * @return true if overcurrent detected
     */
    bool checkOvercurrent();

    /**
     * @brief Handle overcurrent protection
     * @return OS_OK on success, error code on failure
     */
    os_error_t handleOvercurrent();

    // Configuration
    USBOTGConfig m_config;
    USBOTGMode m_currentMode = USBOTGMode::NONE;
    USBOTGState m_state = USBOTGState::DISCONNECTED;

    // Device states
    bool m_initialized = false;
    bool m_hardwareInitialized = false;
    bool m_deviceConnected = false;
    bool m_vbusEnabled = false;
    bool m_overcurrentDetected = false;

    // Device information
    USBDeviceInfo m_deviceInfo;
    std::vector<USBDeviceInfo> m_connectedDevices;

    // Statistics
    uint32_t m_connectionCount = 0;
    uint32_t m_enumerationCount = 0;
    uint32_t m_overcurrentEvents = 0;
    uint32_t m_lastConnectionCheck = 0;
    uint32_t m_lastEnumeration = 0;
    uint16_t m_currentConsumption = 0;

    // Configuration
    static constexpr uint32_t CONNECTION_CHECK_INTERVAL = 100; // 100ms
    static constexpr uint32_t ENUMERATION_RETRY_DELAY = 1000;  // 1s
    static constexpr uint32_t OVERCURRENT_THRESHOLD = 3100;    // mA
    static constexpr uint32_t VBUS_ENABLE_DELAY = 100;         // ms
};

#endif // USB_OTG_SERVICE_H