#ifndef USB_HAL_H
#define USB_HAL_H

#include "../system/os_config.h"
#include "hardware_config.h"
#include <driver/gpio.h>
#include <esp_log.h>
#include <string>

#ifdef CONFIG_USB_HOST_ENABLED
#include <usb/usb_host.h>
#endif

#ifdef CONFIG_USB_OTG_SUPPORTED
#include <hal/usb_dwc_hal.h>
#include <hal/usb_dwc_ll.h>
#endif

/**
 * @file usb_hal.h
 * @brief USB Hardware Abstraction Layer for M5Stack Tab5
 * 
 * Provides USB-C OTG and USB-A host support with automatic
 * host/device detection, power management, and device enumeration.
 */

enum class USBPortType {
    NONE,
    USB_A_HOST,     // USB-A port (host only)
    USB_C_OTG       // USB-C port (OTG capable)
};

enum class USBPortMode {
    USB_DISABLED,
    HOST_MODE,
    DEVICE_MODE,
    AUTO_DETECT
};

enum class USBDeviceClass {
    UNKNOWN,
    HID,            // Human Interface Device
    MSC,            // Mass Storage Class
    CDC,            // Communication Device Class
    AUDIO,          // Audio Device Class
    VIDEO,          // Video Device Class
    HUB,            // USB Hub
    PRINTER,        // Printer Class
    CUSTOM          // Custom/Vendor specific
};

enum class USBConnectionStatus {
    DISCONNECTED,
    CONNECTING,
    CONNECTED,
    ENUMERATING,
    CONFIGURED,
    ERROR
};

struct USBDeviceInfo {
    uint16_t vendorId = 0;
    uint16_t productId = 0;
    uint16_t deviceVersion = 0;
    USBDeviceClass deviceClass = USBDeviceClass::UNKNOWN;
    std::string manufacturer;
    std::string product;
    std::string serialNumber;
    uint8_t configurationCount = 0;
    uint8_t interfaceCount = 0;
    bool isHighSpeed = false;
    uint16_t maxPower = 0;  // mA
};

struct USBPortStatus {
    USBPortType portType = USBPortType::NONE;
    USBPortMode currentMode = USBPortMode::USB_DISABLED;
    USBConnectionStatus connectionStatus = USBConnectionStatus::DISCONNECTED;
    bool powerEnabled = false;
    bool vbusPresent = false;
    bool overcurrentDetected = false;
    uint16_t currentDraw = 0;  // mA
    USBDeviceInfo connectedDevice;
};

/**
 * @brief USB Hardware Abstraction Layer
 * Manages USB-C OTG and USB-A host functionality
 */
class USBHAL {
public:
    USBHAL() = default;
    ~USBHAL();

    /**
     * @brief Initialize USB HAL
     * @return OS_OK on success, error code on failure
     */
    os_error_t initialize();

    /**
     * @brief Shutdown USB HAL
     * @return OS_OK on success, error code on failure
     */
    os_error_t shutdown();

    /**
     * @brief Update USB HAL (check for device changes)
     * @param deltaTime Time since last update in milliseconds
     * @return OS_OK on success, error code on failure
     */
    os_error_t update(uint32_t deltaTime);

    /**
     * @brief Enable USB port
     * @param portType Port type to enable
     * @param mode Port mode
     * @return OS_OK on success, error code on failure
     */
    os_error_t enablePort(USBPortType portType, USBPortMode mode = USBPortMode::AUTO_DETECT);

    /**
     * @brief Disable USB port
     * @param portType Port type to disable
     * @return OS_OK on success, error code on failure
     */
    os_error_t disablePort(USBPortType portType);

    /**
     * @brief Set USB-C OTG mode
     * @param mode Mode to set (host/device/auto)
     * @return OS_OK on success, error code on failure
     */
    os_error_t setOTGMode(USBPortMode mode);

    /**
     * @brief Enable VBUS power output
     * @param portType Port type
     * @param enable Enable/disable VBUS
     * @return OS_OK on success, error code on failure
     */
    os_error_t enableVBUS(USBPortType portType, bool enable);

    /**
     * @brief Get USB port status
     * @param portType Port type
     * @return Port status structure
     */
    USBPortStatus getPortStatus(USBPortType portType) const;

    /**
     * @brief Check if USB device is connected
     * @param portType Port type to check
     * @return true if device is connected
     */
    bool isDeviceConnected(USBPortType portType) const;

    /**
     * @brief Get connected device information
     * @param portType Port type
     * @return Device information
     */
    USBDeviceInfo getDeviceInfo(USBPortType portType) const;

    /**
     * @brief Register device connection callback
     * @param callback Callback function
     * @param userData User data for callback
     */
    void registerDeviceCallback(void(*callback)(USBPortType, USBConnectionStatus, void*), void* userData);

    /**
     * @brief Enumerate connected devices
     * @return OS_OK on success, error code on failure
     */
    os_error_t enumerateDevices();

    /**
     * @brief Reset USB port
     * @param portType Port type to reset
     * @return OS_OK on success, error code on failure
     */
    os_error_t resetPort(USBPortType portType);

    /**
     * @brief Get USB HAL statistics
     */
    void printUSBStats() const;

    /**
     * @brief Check if USB HAL is initialized
     * @return true if initialized
     */
    bool isInitialized() const { return m_initialized; }

    /**
     * @brief Detect USB-C cable orientation and capabilities
     * @return OS_OK on success, error code on failure
     */
    os_error_t detectUSBCCapabilities();

    /**
     * @brief Enable USB power delivery negotiation
     * @param enable Enable/disable PD
     * @return OS_OK on success, error code on failure
     */
    os_error_t enablePowerDelivery(bool enable);

private:
    /**
     * @brief Initialize USB-A host port
     * @return OS_OK on success, error code on failure
     */
    os_error_t initializeUSBAHost();

    /**
     * @brief Initialize USB-C OTG port
     * @return OS_OK on success, error code on failure
     */
    os_error_t initializeUSBCOTG();

    /**
     * @brief Configure USB GPIO pins
     * @return OS_OK on success, error code on failure
     */
    os_error_t configureGPIOs();

    /**
     * @brief Check OTG ID pin state
     * @return true if host mode detected
     */
    bool checkOTGIDPin();

    /**
     * @brief Check VBUS presence
     * @param portType Port type
     * @return true if VBUS is present
     */
    bool checkVBUSPresence(USBPortType portType);

    /**
     * @brief Handle device connection event
     * @param portType Port where device was connected
     */
    void handleDeviceConnection(USBPortType portType);

    /**
     * @brief Handle device disconnection event
     * @param portType Port where device was disconnected
     */
    void handleDeviceDisconnection(USBPortType portType);

    /**
     * @brief Parse USB device descriptor
     * @param portType Port type
     * @return OS_OK on success, error code on failure
     */
    os_error_t parseDeviceDescriptor(USBPortType portType);

    /**
     * @brief USB host event handler
     * @param event USB host event
     */
#ifdef CONFIG_USB_HOST_ENABLED
    static void usbHostEventHandler(const usb_host_client_event_msg_t* event_msg, void* arg);
#endif

    /**
     * @brief OTG interrupt handler
     * @param arg Handler argument
     */
    static void IRAM_ATTR otgInterruptHandler(void* arg);

    // USB port status
    USBPortStatus m_usbAStatus;
    USBPortStatus m_usbCStatus;

    // Hardware handles
#ifdef CONFIG_USB_HOST_ENABLED
    usb_host_client_handle_t m_usbHostHandle = nullptr;
#endif
    
    // GPIO handles
    gpio_num_t m_usbADP = static_cast<gpio_num_t>(USB_HOST_DP_PIN);
    gpio_num_t m_usbADM = static_cast<gpio_num_t>(USB_HOST_DM_PIN);
    gpio_num_t m_usbCDP = static_cast<gpio_num_t>(USB_OTG_DP_PIN);
    gpio_num_t m_usbCDM = static_cast<gpio_num_t>(USB_OTG_DM_PIN);
    gpio_num_t m_usbCVBUS = static_cast<gpio_num_t>(USB_OTG_VBUS_PIN);
    gpio_num_t m_usbCID = static_cast<gpio_num_t>(USB_OTG_ID_PIN);

    // Device callbacks
    void(*m_deviceCallback)(USBPortType, USBConnectionStatus, void*) = nullptr;
    void* m_callbackUserData = nullptr;

    // State management
    bool m_initialized = false;
    bool m_usbHostInitialized = false;
    bool m_otgInitialized = false;
    uint32_t m_lastDeviceCheck = 0;
    
    // Statistics
    uint32_t m_devicesEnumerated = 0;
    uint32_t m_connectionEvents = 0;
    uint32_t m_errorCount = 0;
    uint32_t m_powerDeliveryEvents = 0;

    // Configuration
    static constexpr uint32_t DEVICE_CHECK_INTERVAL = 1000; // 1 second
    static constexpr uint32_t VBUS_SETTLE_TIME = 100; // 100ms
    static constexpr uint32_t RESET_DURATION = 50; // 50ms
    static constexpr const char* TAG = "USBHAL";

    // USB power limits
    static constexpr uint16_t USB_2_0_MAX_CURRENT = 500;  // 500mA
    static constexpr uint16_t USB_3_0_MAX_CURRENT = 900;  // 900mA
    static constexpr uint16_t USB_PD_MAX_CURRENT = 3000;  // 3A
};

#endif // USB_HAL_H