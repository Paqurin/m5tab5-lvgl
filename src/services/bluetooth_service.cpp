#include "bluetooth_service.h"
#include <esp_log.h>
#include <nvs_flash.h>
#include <nvs.h>
#include <string.h>
#include <algorithm>

BluetoothService::BluetoothService() {
    memset(&m_lastKeyboardReport, 0, sizeof(m_lastKeyboardReport));
    memset(&m_lastMouseReport, 0, sizeof(m_lastMouseReport));
}

BluetoothService::~BluetoothService() {
    shutdown();
}

os_error_t BluetoothService::initialize(const BluetoothServiceConfig& config) {
    if (m_initialized) {
        return OS_OK;
    }

    ESP_LOGI(TAG, "Initializing Bluetooth Service");

    m_config = config;

    // Initialize Bluetooth HAL
    os_error_t result = m_bluetoothHAL.initialize();
    if (result != OS_OK) {
        ESP_LOGE(TAG, "Failed to initialize Bluetooth HAL");
        return result;
    }

    // Create event queue and event group
    m_eventQueue = xQueueCreate(EVENT_QUEUE_SIZE, sizeof(BluetoothServiceEventData));
    if (!m_eventQueue) {
        ESP_LOGE(TAG, "Failed to create event queue");
        return OS_ERROR_NO_MEMORY;
    }

    m_eventGroup = xEventGroupCreate();
    if (!m_eventGroup) {
        ESP_LOGE(TAG, "Failed to create event group");
        vQueueDelete(m_eventQueue);
        return OS_ERROR_NO_MEMORY;
    }

    // Register Bluetooth HAL callbacks
    m_bluetoothHAL.setDeviceDiscoveryCallback([this](const BluetoothDeviceInfo& device) {
        this->handleDeviceDiscovery(device);
    });

    m_bluetoothHAL.setConnectionStateCallback([this](const BluetoothDeviceInfo& device, BluetoothConnectionState state) {
        this->handleConnectionStateChange(device, state);
    });

    m_bluetoothHAL.setKeyboardReportCallback([this](const HIDKeyboardReport& report) {
        this->handleKeyboardInput(report);
    });

    m_bluetoothHAL.setMouseReportCallback([this](const HIDMouseReport& report) {
        this->handleMouseInput(report);
    });

    // Enable HID host and A2DP sink
    m_bluetoothHAL.enableHIDHost();
    m_bluetoothHAL.enableA2DPSink();

    // Enable serial if configured
    if (m_config.deviceName.length() > 0) {
        m_bluetoothHAL.enableSerial(m_config.deviceName);
    }

    // Load device profiles from storage
    if (m_config.persistDevices) {
        loadDeviceProfiles();
    }

    // Create service task
    BaseType_t taskResult = xTaskCreate(
        serviceTask,
        "bluetooth_service",
        SERVICE_TASK_STACK_SIZE,
        this,
        tskIDLE_PRIORITY + 2,
        &m_serviceTask
    );

    if (taskResult != pdPASS) {
        ESP_LOGE(TAG, "Failed to create service task");
        vEventGroupDelete(m_eventGroup);
        vQueueDelete(m_eventQueue);
        return OS_ERROR_NO_MEMORY;
    }

    m_initialized = true;
    
    // Start auto-discovery if configured
    if (m_config.autoDiscovery) {
        startDeviceDiscovery(30);
    }

    ESP_LOGI(TAG, "Bluetooth Service initialized successfully");
    sendEvent(BluetoothServiceEvent::DISCOVERY_STARTED);

    return OS_OK;
}

os_error_t BluetoothService::shutdown() {
    if (!m_initialized) {
        return OS_OK;
    }

    ESP_LOGI(TAG, "Shutting down Bluetooth Service");

    // Stop discovery if active
    if (m_discoveryActive) {
        stopDeviceDiscovery();
    }

    // Cancel ongoing pairing
    if (m_pairingState != PairingState::IDLE) {
        cancelPairing();
    }

    // Save device profiles if configured
    if (m_config.persistDevices) {
        saveDeviceProfiles();
    }

    // Delete service task
    if (m_serviceTask) {
        vTaskDelete(m_serviceTask);
        m_serviceTask = nullptr;
    }

    // Clean up synchronization objects
    if (m_eventGroup) {
        vEventGroupDelete(m_eventGroup);
        m_eventGroup = nullptr;
    }

    if (m_eventQueue) {
        vQueueDelete(m_eventQueue);
        m_eventQueue = nullptr;
    }

    // Shutdown Bluetooth HAL
    m_bluetoothHAL.shutdown();

    m_initialized = false;
    ESP_LOGI(TAG, "Bluetooth Service shutdown complete");
    return OS_OK;
}

os_error_t BluetoothService::update(uint32_t deltaTime) {
    if (!m_initialized) {
        return OS_ERROR_INVALID_STATE;
    }

    m_lastUpdateTime += deltaTime;

    // Check for auto-discovery interval
    if (m_config.autoDiscovery && !m_discoveryActive && 
        (m_lastUpdateTime - m_lastDiscoveryTime) > m_config.discoveryInterval) {
        startDeviceDiscovery(10);
        m_lastDiscoveryTime = m_lastUpdateTime;
    }

    // Check for auto-connect interval
    if (m_config.autoConnect && 
        (m_lastUpdateTime - m_lastAutoConnectTime) > AUTO_CONNECT_INTERVAL) {
        attemptAutoConnect();
        m_lastAutoConnectTime = m_lastUpdateTime;
    }

    // Update device connection states
    auto connectedDevices = m_bluetoothHAL.getConnectedDevices();
    
    m_keyboardActive = false;
    m_mouseActive = false;
    m_audioDeviceActive = false;

    for (const auto& device : connectedDevices) {
        updateDeviceProfile(device.address, true);
        
        if (device.type == BluetoothDeviceType::KEYBOARD || 
            device.type == BluetoothDeviceType::COMBO_HID) {
            m_keyboardActive = true;
        }
        
        if (device.type == BluetoothDeviceType::MOUSE || 
            device.type == BluetoothDeviceType::COMBO_HID) {
            m_mouseActive = true;
        }
        
        if (device.type == BluetoothDeviceType::HEADPHONES) {
            m_audioDeviceActive = true;
        }
    }

    return OS_OK;
}

os_error_t BluetoothService::startDeviceDiscovery(uint32_t durationSeconds) {
    if (m_discoveryActive) {
        return OS_OK;
    }

    ESP_LOGI(TAG, "Starting device discovery for %d seconds", durationSeconds);

    os_error_t result = m_bluetoothHAL.startDiscovery(durationSeconds);
    if (result != OS_OK) {
        return result;
    }

    m_discoveryActive = true;
    m_discoveredDevices.clear();
    
    sendEvent(BluetoothServiceEvent::DISCOVERY_STARTED);
    return OS_OK;
}

os_error_t BluetoothService::stopDeviceDiscovery() {
    if (!m_discoveryActive) {
        return OS_OK;
    }

    ESP_LOGI(TAG, "Stopping device discovery");

    os_error_t result = m_bluetoothHAL.stopDiscovery();
    m_discoveryActive = false;
    
    sendEvent(BluetoothServiceEvent::DISCOVERY_COMPLETED);
    return result;
}

std::vector<BluetoothDeviceInfo> BluetoothService::getDiscoveredDevices() const {
    return m_discoveredDevices;
}

std::vector<BluetoothDeviceInfo> BluetoothService::getConnectedDevices() const {
    return m_bluetoothHAL.getConnectedDevices();
}

os_error_t BluetoothService::startPairing(const std::string& deviceAddress) {
    if (m_pairingState != PairingState::IDLE) {
        ESP_LOGW(TAG, "Pairing already in progress");
        return OS_ERROR_INVALID_STATE;
    }

    ESP_LOGI(TAG, "Starting pairing with device: %s", deviceAddress.c_str());

    m_pairingState = PairingState::AUTHENTICATING;
    m_currentPairingDevice = deviceAddress;
    m_pairingStartTime = m_lastUpdateTime;

    os_error_t result = m_bluetoothHAL.pairDevice(deviceAddress);
    if (result != OS_OK) {
        m_pairingState = PairingState::FAILED;
        sendEvent(BluetoothServiceEvent::PAIRING_FAILED, deviceAddress, "", BluetoothDeviceType::UNKNOWN, "Failed to start pairing");
        return result;
    }

    sendEvent(BluetoothServiceEvent::PAIRING_STARTED, deviceAddress);
    return OS_OK;
}

os_error_t BluetoothService::cancelPairing() {
    if (m_pairingState == PairingState::IDLE) {
        return OS_OK;
    }

    ESP_LOGI(TAG, "Cancelling pairing");

    m_pairingState = PairingState::IDLE;
    m_currentPairingDevice.clear();
    
    return OS_OK;
}

os_error_t BluetoothService::connectDevice(const std::string& deviceAddress) {
    ESP_LOGI(TAG, "Connecting to device: %s", deviceAddress.c_str());

    DeviceProfile* profile = findDeviceProfile(deviceAddress);
    if (!profile) {
        ESP_LOGE(TAG, "Device profile not found: %s", deviceAddress.c_str());
        return OS_ERROR_NOT_FOUND;
    }

    os_error_t result = m_bluetoothHAL.connectDevice(deviceAddress);
    if (result == OS_OK) {
        m_totalConnectionAttempts++;
        profile->connectionCount++;
    }

    return result;
}

os_error_t BluetoothService::disconnectDevice(const std::string& deviceAddress) {
    ESP_LOGI(TAG, "Disconnecting device: %s", deviceAddress.c_str());

    os_error_t result = m_bluetoothHAL.disconnectDevice(deviceAddress);
    if (result == OS_OK) {
        updateDeviceProfile(deviceAddress, false);
        sendEvent(BluetoothServiceEvent::DEVICE_DISCONNECTED, deviceAddress);
    }

    return result;
}

os_error_t BluetoothService::removeDevice(const std::string& deviceAddress) {
    ESP_LOGI(TAG, "Removing device profile: %s", deviceAddress.c_str());

    // Disconnect if connected
    disconnectDevice(deviceAddress);

    // Unpair from Bluetooth HAL
    os_error_t result = m_bluetoothHAL.unpairDevice(deviceAddress);
    
    // Remove from profiles
    auto it = std::remove_if(m_deviceProfiles.begin(), m_deviceProfiles.end(),
                            [&deviceAddress](const DeviceProfile& profile) {
                                return profile.address == deviceAddress;
                            });
    m_deviceProfiles.erase(it, m_deviceProfiles.end());

    // Save updated profiles
    if (m_config.persistDevices) {
        saveDeviceProfiles();
    }

    return result;
}

os_error_t BluetoothService::setDeviceAutoConnect(const std::string& deviceAddress, bool autoConnect) {
    DeviceProfile* profile = findDeviceProfile(deviceAddress);
    if (!profile) {
        return OS_ERROR_NOT_FOUND;
    }

    ESP_LOGI(TAG, "Setting auto-connect for %s: %s", deviceAddress.c_str(), autoConnect ? "enabled" : "disabled");
    
    profile->autoConnect = autoConnect;
    
    if (m_config.persistDevices) {
        saveDeviceProfiles();
    }

    return OS_OK;
}

os_error_t BluetoothService::setDeviceTrusted(const std::string& deviceAddress, bool trusted) {
    DeviceProfile* profile = findDeviceProfile(deviceAddress);
    if (!profile) {
        return OS_ERROR_NOT_FOUND;
    }

    ESP_LOGI(TAG, "Setting trusted status for %s: %s", deviceAddress.c_str(), trusted ? "trusted" : "not trusted");
    
    profile->trustedDevice = trusted;
    
    if (m_config.persistDevices) {
        saveDeviceProfiles();
    }

    return OS_OK;
}

os_error_t BluetoothService::setDeviceCustomName(const std::string& deviceAddress, const std::string& customName) {
    DeviceProfile* profile = findDeviceProfile(deviceAddress);
    if (!profile) {
        return OS_ERROR_NOT_FOUND;
    }

    ESP_LOGI(TAG, "Setting custom name for %s: %s", deviceAddress.c_str(), customName.c_str());
    
    profile->customName = customName;
    
    if (m_config.persistDevices) {
        saveDeviceProfiles();
    }

    return OS_OK;
}

bool BluetoothService::getKeyboardState(uint8_t& modifier, uint8_t keys[6]) {
    if (!m_keyboardDataAvailable || !m_keyboardActive) {
        return false;
    }

    modifier = m_lastKeyboardReport.modifier;
    memcpy(keys, m_lastKeyboardReport.keys, 6);
    m_keyboardDataAvailable = false;

    return true;
}

bool BluetoothService::getMouseDelta(int16_t& deltaX, int16_t& deltaY, uint8_t& buttons, int8_t& wheel) {
    if (!m_mouseDataAvailable || !m_mouseActive) {
        return false;
    }

    deltaX = m_mouseDeltaX;
    deltaY = m_mouseDeltaY;
    buttons = m_lastMouseReport.buttons;
    wheel = m_mouseWheelDelta;

    // Reset deltas after reading
    m_mouseDeltaX = 0;
    m_mouseDeltaY = 0;
    m_mouseWheelDelta = 0;
    m_mouseDataAvailable = false;

    return true;
}

AudioDeviceInfo BluetoothService::getAudioDeviceInfo() const {
    return m_bluetoothHAL.getAudioDeviceInfo();
}

os_error_t BluetoothService::controlAudio(bool play) {
    if (!m_audioDeviceActive) {
        return OS_ERROR_INVALID_STATE;
    }

    os_error_t result = m_bluetoothHAL.controlAudioPlayback(play);
    if (result == OS_OK) {
        sendEvent(play ? BluetoothServiceEvent::AUDIO_STARTED : BluetoothServiceEvent::AUDIO_STOPPED);
    }

    return result;
}

os_error_t BluetoothService::setAudioVolume(uint8_t volume) {
    if (!m_audioDeviceActive) {
        return OS_ERROR_INVALID_STATE;
    }

    // Convert 0-100 to 0-127 range
    uint8_t halVolume = (volume * 127) / 100;
    
    return m_bluetoothHAL.setAudioVolume(halVolume);
}

os_error_t BluetoothService::sendMediaCommand(const std::string& command) {
    if (!m_audioDeviceActive) {
        return OS_ERROR_INVALID_STATE;
    }

    // Use the HAL's sendMediaCommand method instead of AVRCP commands directly
    return m_bluetoothHAL.sendMediaCommand(command);
}

void BluetoothService::serviceTask(void* parameter) {
    BluetoothService* service = static_cast<BluetoothService*>(parameter);
    
    ESP_LOGI(TAG, "Bluetooth service task started");

    while (service->m_initialized) {
        // Process pairing workflow
        service->processPairingWorkflow();

        // Check connection timeouts
        service->checkConnectionTimeouts();

        // Process event queue
        BluetoothServiceEventData eventData;
        if (xQueueReceive(service->m_eventQueue, &eventData, pdMS_TO_TICKS(100)) == pdTRUE) {
            if (service->m_eventCallback) {
                service->m_eventCallback(eventData);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(50)); // 50ms task cycle
    }

    ESP_LOGI(TAG, "Bluetooth service task ended");
    vTaskDelete(nullptr);
}

void BluetoothService::handleConnectionStateChange(const BluetoothDeviceInfo& device, BluetoothConnectionState state) {
    ESP_LOGI(TAG, "Connection state change: %s (%s) - %d", 
             device.name.c_str(), device.address.c_str(), static_cast<int>(state));

    switch (state) {
        case BluetoothConnectionState::CONNECTED:
            updateDeviceProfile(device.address, true);
            sendEvent(BluetoothServiceEvent::DEVICE_CONNECTED, device.address, device.name, device.type);
            
            // Update pairing state if this was a pairing operation
            if (m_pairingState == PairingState::AUTHENTICATING && m_currentPairingDevice == device.address) {
                m_pairingState = PairingState::SUCCESS;
                sendEvent(BluetoothServiceEvent::PAIRING_SUCCESS, device.address, device.name, device.type);
                
                // Create device profile if it doesn't exist
                DeviceProfile* profile = findDeviceProfile(device.address);
                if (!profile) {
                    DeviceProfile newProfile;
                    newProfile.address = device.address;
                    newProfile.name = device.name;
                    newProfile.type = device.type;
                    newProfile.autoConnect = false;
                    newProfile.trustedDevice = false;
                    newProfile.lastConnected = m_lastUpdateTime;
                    newProfile.connectionCount = 1;
                    
                    m_deviceProfiles.push_back(newProfile);
                    
                    if (m_config.persistDevices) {
                        saveDeviceProfiles();
                    }
                }
            }
            break;

        case BluetoothConnectionState::DISCONNECTED:
            updateDeviceProfile(device.address, false);
            sendEvent(BluetoothServiceEvent::DEVICE_DISCONNECTED, device.address, device.name, device.type);
            break;

        case BluetoothConnectionState::ERROR:
            if (m_pairingState == PairingState::AUTHENTICATING && m_currentPairingDevice == device.address) {
                m_pairingState = PairingState::FAILED;
                sendEvent(BluetoothServiceEvent::PAIRING_FAILED, device.address, device.name, device.type, "Connection error");
            }
            sendEvent(BluetoothServiceEvent::ERROR_OCCURRED, device.address, device.name, device.type, "Connection error");
            break;

        default:
            break;
    }

    xEventGroupSetBits(m_eventGroup, CONNECTION_CHANGE_BIT);
}

void BluetoothService::handleDeviceDiscovery(const BluetoothDeviceInfo& device) {
    ESP_LOGI(TAG, "Device discovered: %s (%s) RSSI: %d", 
             device.name.c_str(), device.address.c_str(), device.rssi);

    // Add to discovered devices list (avoid duplicates)
    bool found = false;
    for (auto& existing : m_discoveredDevices) {
        if (existing.address == device.address) {
            existing = device; // Update existing entry
            found = true;
            break;
        }
    }
    
    if (!found && m_discoveredDevices.size() < MAX_DISCOVERY_DEVICES) {
        m_discoveredDevices.push_back(device);
    }

    sendEvent(BluetoothServiceEvent::DEVICE_FOUND, device.address, device.name, device.type);
}

void BluetoothService::handleKeyboardInput(const HIDKeyboardReport& report) {
    m_lastKeyboardReport = report;
    m_keyboardDataAvailable = true;
    m_inputEvents++;

    sendEvent(BluetoothServiceEvent::KEYBOARD_INPUT);
}

void BluetoothService::handleMouseInput(const HIDMouseReport& report) {
    // Accumulate mouse deltas
    m_mouseDeltaX += report.x;
    m_mouseDeltaY += report.y;
    m_mouseWheelDelta += report.wheel;
    
    m_lastMouseReport = report;
    m_mouseDataAvailable = true;
    m_inputEvents++;

    sendEvent(BluetoothServiceEvent::MOUSE_INPUT);
}

void BluetoothService::updateDeviceProfile(const std::string& address, bool connected) {
    DeviceProfile* profile = findDeviceProfile(address);
    if (profile) {
        if (connected) {
            profile->lastConnected = m_lastUpdateTime;
        }
    }
}

DeviceProfile* BluetoothService::findDeviceProfile(const std::string& address) {
    for (auto& profile : m_deviceProfiles) {
        if (profile.address == address) {
            return &profile;
        }
    }
    return nullptr;
}

void BluetoothService::attemptAutoConnect() {
    if (!m_config.autoConnect) {
        return;
    }

    // Try to connect to devices with auto-connect enabled
    for (const auto& profile : m_deviceProfiles) {
        if (profile.autoConnect) {
            // Check if device is not already connected
            bool isConnected = false;
            auto connectedDevices = getConnectedDevices();
            for (const auto& device : connectedDevices) {
                if (device.address == profile.address) {
                    isConnected = true;
                    break;
                }
            }

            if (!isConnected) {
                ESP_LOGI(TAG, "Attempting auto-connect to: %s", profile.address.c_str());
                connectDevice(profile.address);
                break; // Try one at a time
            }
        }
    }
}

void BluetoothService::sendEvent(BluetoothServiceEvent event, const std::string& address,
                                const std::string& name, BluetoothDeviceType type,
                                const std::string& errorMsg) {
    BluetoothServiceEventData eventData;
    eventData.event = event;
    eventData.deviceAddress = address;
    eventData.deviceName = name;
    eventData.deviceType = type;
    eventData.errorMessage = errorMsg;
    eventData.timestamp = m_lastUpdateTime;

    if (m_eventQueue) {
        xQueueSend(m_eventQueue, &eventData, 0);
    }
}

void BluetoothService::processPairingWorkflow() {
    if (m_pairingState == PairingState::IDLE) {
        return;
    }

    // Check for pairing timeout
    uint32_t elapsed = m_lastUpdateTime - m_pairingStartTime;
    if (elapsed > m_config.pairingTimeout) {
        ESP_LOGW(TAG, "Pairing timeout for device: %s", m_currentPairingDevice.c_str());
        m_pairingState = PairingState::TIMEOUT;
        sendEvent(BluetoothServiceEvent::PAIRING_FAILED, m_currentPairingDevice, "", BluetoothDeviceType::UNKNOWN, "Pairing timeout");
        m_currentPairingDevice.clear();
        m_pairingState = PairingState::IDLE;
    }
}

void BluetoothService::checkConnectionTimeouts() {
    // Implementation for connection timeout checks
    // This would monitor ongoing connection attempts and timeout them if necessary
}

os_error_t BluetoothService::saveDeviceProfiles() {
    ESP_LOGI(TAG, "Saving device profiles to NVS");

    nvs_handle_t handle;
    esp_err_t err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS: %s", esp_err_to_name(err));
        return OS_ERROR_STORAGE;
    }

    // For simplicity, we'll just save the count and basic info
    // In a real implementation, you'd use a more sophisticated serialization
    
    size_t profileCount = m_deviceProfiles.size();
    err = nvs_set_blob(handle, "profile_count", &profileCount, sizeof(profileCount));
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to save profile count: %s", esp_err_to_name(err));
        nvs_close(handle);
        return OS_ERROR_STORAGE;
    }

    // Save each profile
    for (size_t i = 0; i < m_deviceProfiles.size(); i++) {
        char key[32];
        snprintf(key, sizeof(key), "profile_%d", i);
        
        err = nvs_set_blob(handle, key, &m_deviceProfiles[i], sizeof(DeviceProfile));
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "Failed to save profile %d: %s", i, esp_err_to_name(err));
        }
    }

    err = nvs_commit(handle);
    nvs_close(handle);

    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Saved %d device profiles", m_deviceProfiles.size());
        return OS_OK;
    } else {
        ESP_LOGE(TAG, "Failed to commit profiles: %s", esp_err_to_name(err));
        return OS_ERROR_STORAGE;
    }
}

os_error_t BluetoothService::loadDeviceProfiles() {
    ESP_LOGI(TAG, "Loading device profiles from NVS");

    nvs_handle_t handle;
    esp_err_t err = nvs_open(STORAGE_NAMESPACE, NVS_READONLY, &handle);
    if (err != ESP_OK) {
        ESP_LOGI(TAG, "No stored profiles found (NVS open failed): %s", esp_err_to_name(err));
        return OS_OK; // Not an error if no profiles exist yet
    }

    size_t profileCount = 0;
    size_t required_size = sizeof(profileCount);
    err = nvs_get_blob(handle, "profile_count", &profileCount, &required_size);
    if (err != ESP_OK) {
        ESP_LOGI(TAG, "No stored profiles found (count not found)");
        nvs_close(handle);
        return OS_OK;
    }

    // Load each profile
    m_deviceProfiles.clear();
    m_deviceProfiles.reserve(profileCount);

    for (size_t i = 0; i < profileCount; i++) {
        char key[32];
        snprintf(key, sizeof(key), "profile_%d", i);
        
        DeviceProfile profile;
        required_size = sizeof(profile);
        err = nvs_get_blob(handle, key, &profile, &required_size);
        if (err == ESP_OK) {
            m_deviceProfiles.push_back(profile);
        } else {
            ESP_LOGW(TAG, "Failed to load profile %d: %s", i, esp_err_to_name(err));
        }
    }

    nvs_close(handle);

    ESP_LOGI(TAG, "Loaded %d device profiles", m_deviceProfiles.size());
    return OS_OK;
}

os_error_t BluetoothService::clearStoredProfiles() {
    ESP_LOGI(TAG, "Clearing stored device profiles");

    nvs_handle_t handle;
    esp_err_t err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        return OS_ERROR_STORAGE;
    }

    err = nvs_erase_all(handle);
    if (err == ESP_OK) {
        err = nvs_commit(handle);
    }

    nvs_close(handle);

    if (err == ESP_OK) {
        m_deviceProfiles.clear();
        ESP_LOGI(TAG, "Cleared all stored profiles");
        return OS_OK;
    } else {
        ESP_LOGE(TAG, "Failed to clear profiles: %s", esp_err_to_name(err));
        return OS_ERROR_STORAGE;
    }
}

void BluetoothService::getServiceStats(uint32_t& totalPairedDevices, uint32_t& activeConnections,
                                     uint32_t& totalConnectionAttempts, uint32_t& inputEvents) {
    totalPairedDevices = m_deviceProfiles.size();
    activeConnections = getConnectedDevices().size();
    totalConnectionAttempts = m_totalConnectionAttempts;
    inputEvents = m_inputEvents;
}

void BluetoothService::resetStats() {
    m_totalConnectionAttempts = 0;
    m_inputEvents = 0;
    ESP_LOGI(TAG, "Service statistics reset");
}

void BluetoothService::printStatus() const {
    ESP_LOGI(TAG, "=== Bluetooth Service Status ===");
    ESP_LOGI(TAG, "Initialized: %s", m_initialized ? "Yes" : "No");
    ESP_LOGI(TAG, "Discovery Active: %s", m_discoveryActive ? "Yes" : "No");
    ESP_LOGI(TAG, "Pairing State: %d", static_cast<int>(m_pairingState));
    ESP_LOGI(TAG, "Keyboard Active: %s", m_keyboardActive ? "Yes" : "No");
    ESP_LOGI(TAG, "Mouse Active: %s", m_mouseActive ? "Yes" : "No");
    ESP_LOGI(TAG, "Audio Device Active: %s", m_audioDeviceActive ? "Yes" : "No");
    ESP_LOGI(TAG, "Device Profiles: %d", m_deviceProfiles.size());
    ESP_LOGI(TAG, "Discovered Devices: %d", m_discoveredDevices.size());
    ESP_LOGI(TAG, "Connected Devices: %d", getConnectedDevices().size());
    ESP_LOGI(TAG, "Total Connection Attempts: %d", m_totalConnectionAttempts);
    ESP_LOGI(TAG, "Input Events: %d", m_inputEvents);
    ESP_LOGI(TAG, "=============================");
}