#include "usb_otg_service.h"
#include "../system/os_manager.h"
#include <esp_log.h>
#include <driver/gpio.h>

#ifdef CONFIG_USB_HOST_SUPPORTED
#include <usb/usb_host.h>
#endif

static const char* TAG = "USBOTGService";

USBOTGService::~USBOTGService() {
    shutdown();
}

os_error_t USBOTGService::initialize() {
    if (m_initialized) {
        return OS_OK;
    }

    ESP_LOGI(TAG, "Initializing USB OTG Service");

#ifndef HW_HAS_USB_OTG
    ESP_LOGW(TAG, "USB OTG not supported by hardware");
    return OS_ERROR_NOT_SUPPORTED;
#endif

    // Initialize hardware
    os_error_t result = initializeHardware();
    if (result != OS_OK) {
        ESP_LOGE(TAG, "Failed to initialize USB OTG hardware");
        return result;
    }

    // Configure pins
    result = configurePins();
    if (result != OS_OK) {
        ESP_LOGE(TAG, "Failed to configure USB OTG pins");
        return result;
    }

    // Set default configuration
    m_config = USBOTGConfig();
    
    // Initial mode detection
    if (m_config.mode == USBOTGMode::AUTO) {
        bool isHost = checkOTGID();
        m_currentMode = isHost ? USBOTGMode::HOST : USBOTGMode::DEVICE;
    } else {
        m_currentMode = m_config.mode;
    }

    ESP_LOGI(TAG, "USB OTG initialized in %s mode", 
             m_currentMode == USBOTGMode::HOST ? "HOST" : "DEVICE");

    m_initialized = true;
    return OS_OK;
}

os_error_t USBOTGService::shutdown() {
    if (!m_initialized) {
        return OS_OK;
    }

    ESP_LOGI(TAG, "Shutting down USB OTG Service");

    // Disable VBUS if enabled
    if (m_vbusEnabled) {
        enableVBUS(false);
    }

    // Reset state
    m_state = USBOTGState::DISCONNECTED;
    m_deviceConnected = false;
    m_connectedDevices.clear();

    m_initialized = false;
    return OS_OK;
}

os_error_t USBOTGService::update(uint32_t deltaTime) {
    if (!m_initialized) {
        return OS_ERROR_NOT_INITIALIZED;
    }

    uint32_t currentTime = xTaskGetTickCount() * portTICK_PERIOD_MS;

    // Check for connection changes
    if (currentTime - m_lastConnectionCheck > CONNECTION_CHECK_INTERVAL) {
        m_lastConnectionCheck = currentTime;

        // Check for mode changes in AUTO mode
        if (m_config.mode == USBOTGMode::AUTO) {
            bool isHost = checkOTGID();
            USBOTGMode detectedMode = isHost ? USBOTGMode::HOST : USBOTGMode::DEVICE;
            
            if (detectedMode != m_currentMode) {
                ESP_LOGI(TAG, "Mode changed from %s to %s",
                         m_currentMode == USBOTGMode::HOST ? "HOST" : "DEVICE",
                         detectedMode == USBOTGMode::HOST ? "HOST" : "DEVICE");
                m_currentMode = detectedMode;
                
                // Handle mode change
                if (m_deviceConnected) {
                    handleDeviceDisconnection();
                }
            }
        }

        // Check device connection status
        bool vbusPresent = checkVBUS();
        
        if (m_currentMode == USBOTGMode::HOST) {
            // In host mode, we control VBUS
            if (!m_deviceConnected && vbusPresent) {
                handleDeviceConnection();
            } else if (m_deviceConnected && !vbusPresent) {
                handleDeviceDisconnection();
            }
        } else {
            // In device mode, check if host is providing VBUS
            if (!m_deviceConnected && vbusPresent) {
                m_deviceConnected = true;
                m_state = USBOTGState::CONNECTED;
                ESP_LOGI(TAG, "Connected as USB device");
            } else if (m_deviceConnected && !vbusPresent) {
                m_deviceConnected = false;
                m_state = USBOTGState::DISCONNECTED;
                ESP_LOGI(TAG, "Disconnected as USB device");
            }
        }

        // Check for overcurrent
        if (m_vbusEnabled && checkOvercurrent()) {
            handleOvercurrent();
        }
    }

    return OS_OK;
}

os_error_t USBOTGService::configure(const USBOTGConfig& config) {
    m_config = config;
    
    // Apply configuration changes
    if (config.mode != USBOTGMode::AUTO && config.mode != m_currentMode) {
        setMode(config.mode);
    }

    return OS_OK;
}

os_error_t USBOTGService::setMode(USBOTGMode mode) {
    if (mode == m_currentMode) {
        return OS_OK;
    }

    ESP_LOGI(TAG, "Setting USB OTG mode to %s", 
             mode == USBOTGMode::HOST ? "HOST" : "DEVICE");

    // Disconnect current device if connected
    if (m_deviceConnected) {
        handleDeviceDisconnection();
    }

    m_currentMode = mode;
    m_state = USBOTGState::DISCONNECTED;

    return OS_OK;
}

os_error_t USBOTGService::setPowerDelivery(bool enabled) {
    m_config.powerDeliveryEnabled = enabled;
    
    if (m_currentMode == USBOTGMode::HOST) {
        return enableVBUS(enabled && m_deviceConnected);
    }
    
    return OS_OK;
}

os_error_t USBOTGService::setMaxOutputCurrent(uint16_t current) {
    if (current > 5000) { // 5A maximum
        return OS_ERROR_INVALID_PARAM;
    }
    
    m_config.maxOutputCurrent = current;
    ESP_LOGI(TAG, "Set maximum output current to %u mA", current);
    
    return OS_OK;
}

os_error_t USBOTGService::enumerateDevice() {
    if (!m_deviceConnected || m_currentMode != USBOTGMode::HOST) {
        return OS_ERROR_NOT_AVAILABLE;
    }

    ESP_LOGI(TAG, "Enumerating connected device");
    m_state = USBOTGState::ENUMERATING;
    
    os_error_t result = enumerateConnectedDevice();
    if (result == OS_OK) {
        m_state = USBOTGState::READY;
        m_enumerationCount++;
        ESP_LOGI(TAG, "Device enumeration successful");
    } else {
        m_state = USBOTGState::ERROR;
        ESP_LOGE(TAG, "Device enumeration failed");
    }

    return result;
}

os_error_t USBOTGService::getDeviceList(std::vector<USBDeviceInfo>& devices) {
    devices = m_connectedDevices;
    return OS_OK;
}

void USBOTGService::printUSBStats() const {
    ESP_LOGI(TAG, "=== USB OTG Statistics ===");
    ESP_LOGI(TAG, "Mode: %s", m_currentMode == USBOTGMode::HOST ? "HOST" : "DEVICE");
    ESP_LOGI(TAG, "State: %s", m_state == USBOTGState::CONNECTED ? "CONNECTED" : "DISCONNECTED");
    ESP_LOGI(TAG, "Device Connected: %s", m_deviceConnected ? "YES" : "NO");
    ESP_LOGI(TAG, "VBUS Enabled: %s", m_vbusEnabled ? "YES" : "NO");
    ESP_LOGI(TAG, "Current Consumption: %u mA", m_currentConsumption);
    ESP_LOGI(TAG, "Connection Count: %u", m_connectionCount);
    ESP_LOGI(TAG, "Enumeration Count: %u", m_enumerationCount);
    ESP_LOGI(TAG, "Overcurrent Events: %u", m_overcurrentEvents);
}

// Private methods implementation

os_error_t USBOTGService::initializeHardware() {
    ESP_LOGI(TAG, "Initializing USB OTG hardware");

#ifdef CONFIG_USB_HOST_SUPPORTED
    // Initialize USB host stack if supported
    usb_host_config_t host_config = {
        .skip_phy_setup = false,
        .intr_flags = ESP_INTR_FLAG_LEVEL1,
    };
    
    esp_err_t ret = usb_host_install(&host_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to install USB host stack: %s", esp_err_to_name(ret));
        return OS_ERROR_HARDWARE;
    }
#endif

    m_hardwareInitialized = true;
    return OS_OK;
}

os_error_t USBOTGService::configurePins() {
    ESP_LOGI(TAG, "Configuring USB OTG pins");

    // Configure OTG ID pin as input with pullup
    gpio_config_t id_config = {
        .pin_bit_mask = (1ULL << USB_OTG_ID_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&id_config);

    // Configure VBUS control pin as output
    gpio_config_t vbus_config = {
        .pin_bit_mask = (1ULL << USB_OTG_VBUS_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&vbus_config);
    
    // Initialize VBUS to disabled
    gpio_set_level(static_cast<gpio_num_t>(USB_OTG_VBUS_PIN), 0);

    return OS_OK;
}

bool USBOTGService::checkOTGID() {
    // ID pin low = host mode, high = device mode
    return gpio_get_level(static_cast<gpio_num_t>(USB_OTG_ID_PIN)) == 0;
}

bool USBOTGService::checkVBUS() {
    // In a real implementation, this would read a VBUS detection pin
    // For now, we'll simulate based on mode and VBUS enable state
    if (m_currentMode == USBOTGMode::HOST) {
        return m_vbusEnabled;
    } else {
        // In device mode, would read external VBUS presence
        return true; // Simplified for this implementation
    }
}

os_error_t USBOTGService::enableVBUS(bool enabled) {
    if (enabled == m_vbusEnabled) {
        return OS_OK;
    }

    ESP_LOGI(TAG, "%s VBUS", enabled ? "Enabling" : "Disabling");
    
    gpio_set_level(static_cast<gpio_num_t>(USB_OTG_VBUS_PIN), enabled ? 1 : 0);
    m_vbusEnabled = enabled;
    
    if (enabled) {
        // Small delay for VBUS to stabilize
        vTaskDelay(pdMS_TO_TICKS(VBUS_ENABLE_DELAY));
    }

    return OS_OK;
}

os_error_t USBOTGService::handleDeviceConnection() {
    if (m_deviceConnected) {
        return OS_OK;
    }

    ESP_LOGI(TAG, "Handling device connection");
    
    m_deviceConnected = true;
    m_connectionCount++;
    m_state = USBOTGState::CONNECTING;

    // Enable VBUS for host mode
    if (m_currentMode == USBOTGMode::HOST && m_config.powerDeliveryEnabled) {
        enableVBUS(true);
    }

    // Auto-enumerate if enabled
    if (m_config.autoEnumeration && m_currentMode == USBOTGMode::HOST) {
        vTaskDelay(pdMS_TO_TICKS(100)); // Wait for device to stabilize
        enumerateDevice();
    } else {
        m_state = USBOTGState::CONNECTED;
    }

    return OS_OK;
}

os_error_t USBOTGService::handleDeviceDisconnection() {
    if (!m_deviceConnected) {
        return OS_OK;
    }

    ESP_LOGI(TAG, "Handling device disconnection");
    
    m_deviceConnected = false;
    m_state = USBOTGState::DISCONNECTED;

    // Disable VBUS for host mode
    if (m_currentMode == USBOTGMode::HOST) {
        enableVBUS(false);
    }

    // Clear device information
    m_deviceInfo = USBDeviceInfo();
    m_connectedDevices.clear();

    return OS_OK;
}

os_error_t USBOTGService::enumerateConnectedDevice() {
    // This is a simplified enumeration implementation
    // In a real implementation, this would:
    // 1. Send GET_DESCRIPTOR requests
    // 2. Parse device descriptors
    // 3. Set device configuration
    // 4. Store device information

    ESP_LOGI(TAG, "Enumerating device (simplified)");
    
    // Simulate device information
    m_deviceInfo.vendorId = 0x1234;
    m_deviceInfo.productId = 0x5678;
    m_deviceInfo.deviceVersion = 0x0100;
    m_deviceInfo.deviceClass = USBDeviceClass::STORAGE;
    m_deviceInfo.manufacturer = "Generic";
    m_deviceInfo.product = "USB Storage Device";
    m_deviceInfo.serialNumber = "123456789";
    m_deviceInfo.configurationCount = 1;
    m_deviceInfo.maxPower = 500;
    m_deviceInfo.selfPowered = false;

    m_connectedDevices.clear();
    m_connectedDevices.push_back(m_deviceInfo);

    return OS_OK;
}

os_error_t USBOTGService::parseDeviceDescriptor(const uint8_t* descriptor, USBDeviceInfo& info) {
    // USB device descriptor parsing implementation
    // This is simplified - real implementation would parse actual USB descriptors
    ESP_LOGI(TAG, "Parsing device descriptor");
    return OS_OK;
}

bool USBOTGService::checkOvercurrent() {
    // In a real implementation, this would read current sense circuitry
    // For now, simulate based on current consumption
    return m_currentConsumption > OVERCURRENT_THRESHOLD;
}

os_error_t USBOTGService::handleOvercurrent() {
    ESP_LOGW(TAG, "Overcurrent detected! Disabling VBUS");
    
    m_overcurrentDetected = true;
    m_overcurrentEvents++;
    
    // Disable VBUS immediately
    enableVBUS(false);
    
    // Disconnect device
    if (m_deviceConnected) {
        handleDeviceDisconnection();
    }
    
    m_state = USBOTGState::ERROR;
    
    return OS_OK;
}