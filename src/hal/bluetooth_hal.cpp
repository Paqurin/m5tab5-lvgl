#include "bluetooth_hal.h"
#include <esp_log.h>
#include <string.h>
#include <cstdio>
#include <algorithm>

// Static instance for event handlers
BluetoothHAL* BluetoothHAL::s_instance = nullptr;

BluetoothHAL::BluetoothHAL() {
    s_instance = this;
    memset(&m_latestKeyboardReport, 0, sizeof(m_latestKeyboardReport));
    memset(&m_latestMouseReport, 0, sizeof(m_latestMouseReport));
    memset(&m_audioDeviceInfo, 0, sizeof(m_audioDeviceInfo));
}

BluetoothHAL::~BluetoothHAL() {
    shutdown();
    s_instance = nullptr;
}

os_error_t BluetoothHAL::initialize() {
    if (m_initialized) {
        return OS_OK;
    }

    ESP_LOGI(TAG, "Initializing Bluetooth HAL via ESP32-C6");

    // Configure GPIO pins
    os_error_t result = configurePinSettings();
    if (result != OS_OK) {
        ESP_LOGE(TAG, "Failed to configure GPIO pins");
        return result;
    }

    // Create mutexes and queues
    m_c6Mutex = xSemaphoreCreateMutex();
    if (!m_c6Mutex) {
        ESP_LOGE(TAG, "Failed to create C6 mutex");
        return OS_ERROR_NO_MEMORY;
    }

    m_serialMutex = xSemaphoreCreateMutex();
    if (!m_serialMutex) {
        ESP_LOGE(TAG, "Failed to create serial mutex");
        vSemaphoreDelete(m_c6Mutex);
        return OS_ERROR_NO_MEMORY;
    }

    m_eventQueue = xQueueCreate(SDIO_QUEUE_SIZE, sizeof(C6Packet));
    if (!m_eventQueue) {
        ESP_LOGE(TAG, "Failed to create event queue");
        vSemaphoreDelete(m_c6Mutex);
        vSemaphoreDelete(m_serialMutex);
        return OS_ERROR_NO_MEMORY;
    }

    // Initialize SDIO communication
    result = initializeSDIO();
    if (result != OS_OK) {
        ESP_LOGE(TAG, "Failed to initialize SDIO communication");
        vQueueDelete(m_eventQueue);
        vSemaphoreDelete(m_serialMutex);
        vSemaphoreDelete(m_c6Mutex);
        return result;
    }

    // Initialize ESP32-C6 communication
    result = initializeC6Communication();
    if (result != OS_OK) {
        ESP_LOGE(TAG, "Failed to initialize C6 communication");
        shutdownSDIO();
        vQueueDelete(m_eventQueue);
        vSemaphoreDelete(m_serialMutex);
        vSemaphoreDelete(m_c6Mutex);
        return result;
    }

    m_initialized = true;
    ESP_LOGI(TAG, "Bluetooth HAL initialized successfully");
    return OS_OK;
}

os_error_t BluetoothHAL::shutdown() {
    if (!m_initialized) {
        return OS_OK;
    }

    ESP_LOGI(TAG, "Shutting down Bluetooth HAL");

    // Stop discovery if active
    if (m_discovering) {
        stopDiscovery();
    }

    // Disconnect all devices
    for (const auto& device : getConnectedDevices()) {
        disconnectDevice(device.address);
    }

    // Disable Bluetooth services
    if (m_bluetoothEnabled) {
        enableBluetooth(false);
    }

    // Shutdown SDIO communication
    shutdownSDIO();

    // Clean up synchronization objects
    if (m_eventQueue) {
        vQueueDelete(m_eventQueue);
        m_eventQueue = nullptr;
    }

    if (m_serialMutex) {
        vSemaphoreDelete(m_serialMutex);
        m_serialMutex = nullptr;
    }

    if (m_c6Mutex) {
        vSemaphoreDelete(m_c6Mutex);
        m_c6Mutex = nullptr;
    }

    m_initialized = false;
    m_c6Ready = false;
    m_bluetoothEnabled = false;
    ESP_LOGI(TAG, "Bluetooth HAL shutdown complete");
    return OS_OK;
}

os_error_t BluetoothHAL::enableBluetooth(bool enable) {
    if (!m_initialized) {
        return OS_ERROR_INVALID_STATE;
    }

    if (enable == m_bluetoothEnabled) {
        return OS_OK;
    }

    ESP_LOGI(TAG, "%s Bluetooth via ESP32-C6", enable ? "Enabling" : "Disabling");

    C6Command command = enable ? C6Command::BT_ENABLE : C6Command::BT_DISABLE;
    os_error_t result = sendC6Command(command);
    if (result != OS_OK) {
        ESP_LOGE(TAG, "Failed to %s Bluetooth", enable ? "enable" : "disable");
        return result;
    }

    // Wait for response
    C6Packet response;
    result = receiveC6Event(response, C6_RESPONSE_TIMEOUT);
    if (result == OS_OK && response.command == static_cast<uint8_t>(C6Event::READY)) {
        m_bluetoothEnabled = enable;
        ESP_LOGI(TAG, "Bluetooth %s successfully", enable ? "enabled" : "disabled");
        return OS_OK;
    } else {
        ESP_LOGE(TAG, "Failed to receive Bluetooth enable response");
        return OS_ERROR_HARDWARE;
    }
}

os_error_t BluetoothHAL::startDiscovery(uint32_t durationSeconds) {
    if (!m_bluetoothEnabled) {
        ESP_LOGE(TAG, "Bluetooth not enabled");
        return OS_ERROR_INVALID_STATE;
    }

    if (m_discovering) {
        ESP_LOGW(TAG, "Discovery already in progress");
        return OS_OK;
    }

    ESP_LOGI(TAG, "Starting device discovery for %d seconds", durationSeconds);

    // Clear previous discovery results
    m_discoveredDevices.clear();
    m_discoveryDuration = durationSeconds;

    // Send discovery command with duration
    uint8_t data[4];
    data[0] = (durationSeconds >> 24) & 0xFF;
    data[1] = (durationSeconds >> 16) & 0xFF;
    data[2] = (durationSeconds >> 8) & 0xFF;
    data[3] = durationSeconds & 0xFF;

    os_error_t result = sendC6Command(C6Command::BT_START_DISCOVERY, data, 4);
    if (result != OS_OK) {
        ESP_LOGE(TAG, "Failed to start discovery");
        return result;
    }

    m_discovering = true;
    return OS_OK;
}

os_error_t BluetoothHAL::stopDiscovery() {
    if (!m_discovering) {
        return OS_OK;
    }

    ESP_LOGI(TAG, "Stopping device discovery");
    
    os_error_t result = sendC6Command(C6Command::BT_STOP_DISCOVERY);
    if (result == OS_OK) {
        m_discovering = false;
    }

    return result;
}

os_error_t BluetoothHAL::pairDevice(const std::string& deviceAddress) {
    if (!m_bluetoothEnabled) {
        return OS_ERROR_INVALID_STATE;
    }

    ESP_LOGI(TAG, "Attempting to pair with device: %s", deviceAddress.c_str());

    // Convert address string to binary format for transmission
    uint8_t addressData[6];
    if (sscanf(deviceAddress.c_str(), "%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx",
               &addressData[0], &addressData[1], &addressData[2], 
               &addressData[3], &addressData[4], &addressData[5]) != 6) {
        ESP_LOGE(TAG, "Invalid MAC address format: %s", deviceAddress.c_str());
        return OS_ERROR_INVALID_PARAM;
    }

    os_error_t result = sendC6Command(C6Command::BT_PAIR_DEVICE, addressData, 6);
    if (result == OS_OK) {
        m_connectionAttempts++;
    }

    return result;
}

os_error_t BluetoothHAL::unpairDevice(const std::string& deviceAddress) {
    ESP_LOGI(TAG, "Unpairing device: %s", deviceAddress.c_str());

    uint8_t addressData[6];
    if (sscanf(deviceAddress.c_str(), "%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx",
               &addressData[0], &addressData[1], &addressData[2], 
               &addressData[3], &addressData[4], &addressData[5]) != 6) {
        return OS_ERROR_INVALID_PARAM;
    }

    os_error_t result = sendC6Command(C6Command::BT_UNPAIR_DEVICE, addressData, 6);

    // Remove from paired devices list
    auto it = std::remove_if(m_pairedDevices.begin(), m_pairedDevices.end(),
                            [&deviceAddress](const BluetoothDeviceInfo& dev) {
                                return dev.address == deviceAddress;
                            });
    m_pairedDevices.erase(it, m_pairedDevices.end());

    return result;
}

os_error_t BluetoothHAL::connectDevice(const std::string& deviceAddress) {
    ESP_LOGI(TAG, "Connecting to device: %s", deviceAddress.c_str());

    uint8_t addressData[6];
    if (sscanf(deviceAddress.c_str(), "%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx",
               &addressData[0], &addressData[1], &addressData[2], 
               &addressData[3], &addressData[4], &addressData[5]) != 6) {
        return OS_ERROR_INVALID_PARAM;
    }

    os_error_t result = sendC6Command(C6Command::BT_CONNECT_DEVICE, addressData, 6);
    if (result == OS_OK) {
        m_connectionAttempts++;
    }

    return result;
}

os_error_t BluetoothHAL::disconnectDevice(const std::string& deviceAddress) {
    ESP_LOGI(TAG, "Disconnecting device: %s", deviceAddress.c_str());

    uint8_t addressData[6];
    if (sscanf(deviceAddress.c_str(), "%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx",
               &addressData[0], &addressData[1], &addressData[2], 
               &addressData[3], &addressData[4], &addressData[5]) != 6) {
        return OS_ERROR_INVALID_PARAM;
    }

    return sendC6Command(C6Command::BT_DISCONNECT_DEVICE, addressData, 6);
}

std::vector<BluetoothDeviceInfo> BluetoothHAL::getConnectedDevices() const {
    // This would normally query the C6 for current connections
    // For now, return devices marked as connected in our local list
    std::vector<BluetoothDeviceInfo> connected;
    for (const auto& device : m_discoveredDevices) {
        if (device.connected) {
            connected.push_back(device);
        }
    }
    for (const auto& device : m_pairedDevices) {
        if (device.connected) {
            // Check if not already in list
            bool found = false;
            for (const auto& conn : connected) {
                if (conn.address == device.address) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                connected.push_back(device);
            }
        }
    }
    return connected;
}

os_error_t BluetoothHAL::enableHIDHost() {
    if (m_hidHostEnabled) {
        return OS_OK;
    }

    ESP_LOGI(TAG, "Enabling HID host");

    os_error_t result = sendC6Command(C6Command::HID_ENABLE_HOST);
    if (result == OS_OK) {
        m_hidHostEnabled = true;
        ESP_LOGI(TAG, "HID host enabled");
    }

    return result;
}

os_error_t BluetoothHAL::getKeyboardReport(HIDKeyboardReport& report) {
    if (!m_keyboardReportAvailable) {
        return OS_ERROR_NOT_FOUND;
    }

    report = m_latestKeyboardReport;
    m_keyboardReportAvailable = false;
    return OS_OK;
}

os_error_t BluetoothHAL::getMouseReport(HIDMouseReport& report) {
    if (!m_mouseReportAvailable) {
        return OS_ERROR_NOT_FOUND;
    }

    report = m_latestMouseReport;
    m_mouseReportAvailable = false;
    return OS_OK;
}

os_error_t BluetoothHAL::enableA2DPSink() {
    if (m_a2dpSinkEnabled) {
        return OS_OK;
    }

    ESP_LOGI(TAG, "Enabling A2DP sink");

    os_error_t result = sendC6Command(C6Command::AUDIO_ENABLE_A2DP);
    if (result == OS_OK) {
        m_a2dpSinkEnabled = true;
        ESP_LOGI(TAG, "A2DP sink enabled");
    }

    return result;
}

os_error_t BluetoothHAL::controlAudioPlayback(bool play) {
    if (!m_audioDeviceConnected) {
        return OS_ERROR_INVALID_STATE;
    }

    ESP_LOGI(TAG, "%s audio playback", play ? "Starting" : "Pausing");

    C6Command cmd = play ? C6Command::AUDIO_PLAY : C6Command::AUDIO_PAUSE;
    return sendC6Command(cmd);
}

os_error_t BluetoothHAL::setAudioVolume(uint8_t volume) {
    if (!m_audioDeviceConnected) {
        return OS_ERROR_INVALID_STATE;
    }

    ESP_LOGI(TAG, "Setting audio volume to %d", volume);

    // Volume is 0-127, clamp to valid range
    volume = volume > 127 ? 127 : volume;
    m_audioDeviceInfo.volume = volume;

    return sendC6Command(C6Command::AUDIO_SET_VOLUME, &volume, 1);
}

os_error_t BluetoothHAL::sendMediaCommand(const std::string& command) {
    if (!m_audioDeviceConnected) {
        return OS_ERROR_INVALID_STATE;
    }

    ESP_LOGI(TAG, "Sending media command: %s", command.c_str());

    uint8_t cmdByte = 0;
    if (command == "play") {
        cmdByte = 1;
    } else if (command == "pause") {
        cmdByte = 2;
    } else if (command == "next") {
        cmdByte = 3;
    } else if (command == "previous") {
        cmdByte = 4;
    } else if (command == "stop") {
        cmdByte = 5;
    } else {
        return OS_ERROR_INVALID_PARAM;
    }

    return sendC6Command(C6Command::AUDIO_MEDIA_COMMAND, &cmdByte, 1);
}

os_error_t BluetoothHAL::enableSerial(const std::string& deviceName) {
    if (m_serialEnabled) {
        return OS_OK;
    }

    ESP_LOGI(TAG, "Enabling Bluetooth serial with name: %s", deviceName.c_str());

    // Send device name as data
    os_error_t result = sendC6Command(C6Command::SERIAL_ENABLE, 
                                     (const uint8_t*)deviceName.c_str(), 
                                     deviceName.length());
    if (result == OS_OK) {
        m_serialEnabled = true;
        ESP_LOGI(TAG, "Bluetooth serial enabled");
    }

    return result;
}

os_error_t BluetoothHAL::sendSerialData(const uint8_t* data, size_t length) {
    if (!m_serialEnabled || length == 0 || length > 255) {
        return OS_ERROR_INVALID_STATE;
    }

    return sendC6Command(C6Command::SERIAL_SEND_DATA, data, static_cast<uint16_t>(length));
}

size_t BluetoothHAL::serialAvailable() const {
    if (!m_serialEnabled || !m_serialMutex) {
        return 0;
    }

    xSemaphoreTake(m_serialMutex, portMAX_DELAY);
    size_t available = m_serialBuffer.size();
    xSemaphoreGive(m_serialMutex);
    
    return available;
}

size_t BluetoothHAL::readSerialData(uint8_t* buffer, size_t maxLength) {
    if (!m_serialEnabled || !buffer || maxLength == 0 || !m_serialMutex) {
        return 0;
    }

    xSemaphoreTake(m_serialMutex, portMAX_DELAY);
    
    size_t toRead = std::min(maxLength, m_serialBuffer.size());
    if (toRead > 0) {
        memcpy(buffer, m_serialBuffer.data(), toRead);
        m_serialBuffer.erase(m_serialBuffer.begin(), m_serialBuffer.begin() + toRead);
    }
    
    xSemaphoreGive(m_serialMutex);
    
    return toRead;
}

// Private methods

os_error_t BluetoothHAL::initializeC6Communication() {
    ESP_LOGI(TAG, "Initializing ESP32-C6 communication");

    // Reset C6 chip
    os_error_t result = resetC6Chip();
    if (result != OS_OK) {
        ESP_LOGE(TAG, "Failed to reset C6 chip");
        return result;
    }

    // Wait for C6 to be ready
    vTaskDelay(pdMS_TO_TICKS(1000));

    // Send ping command to verify communication
    result = sendC6Command(C6Command::PING);
    if (result != OS_OK) {
        ESP_LOGE(TAG, "Failed to send ping to C6");
        return result;
    }

    // Wait for ready response
    C6Packet response;
    result = receiveC6Event(response, C6_RESPONSE_TIMEOUT);
    if (result == OS_OK && response.command == static_cast<uint8_t>(C6Event::READY)) {
        m_c6Ready = true;
        ESP_LOGI(TAG, "ESP32-C6 communication established");
        return OS_OK;
    } else {
        ESP_LOGE(TAG, "Failed to establish C6 communication");
        return OS_ERROR_HARDWARE;
    }
}

os_error_t BluetoothHAL::resetC6Chip() {
    ESP_LOGI(TAG, "Resetting ESP32-C6 chip");

    // Assert reset pin
    gpio_set_level(static_cast<gpio_num_t>(ESP32_C6_RESET), 0);
    vTaskDelay(pdMS_TO_TICKS(100));

    // Release reset pin
    gpio_set_level(static_cast<gpio_num_t>(ESP32_C6_RESET), 1);
    vTaskDelay(pdMS_TO_TICKS(500));

    return OS_OK;
}

os_error_t BluetoothHAL::sendC6Command(C6Command command, const uint8_t* data, uint16_t length) {
    if (!m_c6Mutex) {
        return OS_ERROR_INVALID_STATE;
    }

    xSemaphoreTake(m_c6Mutex, portMAX_DELAY);

    C6Packet packet;
    packet.sync = SYNC_BYTE;
    packet.command = static_cast<uint8_t>(command);
    packet.length = length;
    
    if (data && length > 0) {
        memcpy(packet.data, data, length);
    }
    
    packet.checksum = calculateChecksum(packet);

    // Send packet via SDIO (stub implementation)
    // In real implementation, this would use SDIO slave interface
    ESP_LOGD(TAG, "Sending C6 command: 0x%02X, length: %d", packet.command, packet.length);

    xSemaphoreGive(m_c6Mutex);
    return OS_OK;
}

os_error_t BluetoothHAL::receiveC6Event(C6Packet& packet, uint32_t timeoutMs) {
    if (!m_eventQueue) {
        return OS_ERROR_INVALID_STATE;
    }

    TickType_t timeout = pdMS_TO_TICKS(timeoutMs);
    if (xQueueReceive(m_eventQueue, &packet, timeout) == pdTRUE) {
        if (validatePacket(packet)) {
            return OS_OK;
        } else {
            ESP_LOGE(TAG, "Invalid packet received");
            return OS_ERROR_INVALID_DATA;
        }
    }

    return OS_ERROR_TIMEOUT;
}

void BluetoothHAL::processC6Event(const C6Packet& packet) {
    C6Event event = static_cast<C6Event>(packet.command);
    
    ESP_LOGD(TAG, "Processing C6 event: 0x%02X", packet.command);

    switch (event) {
        case C6Event::BT_DEVICE_DISCOVERED: {
            if (packet.length >= 12) { // 6 bytes address + min other data
                BluetoothDeviceInfo device;
                
                // Extract MAC address
                device.address = formatMACAddress(packet.data);
                
                // Extract RSSI (2 bytes)
                device.rssi = (static_cast<int16_t>(packet.data[6]) << 8) | packet.data[7];
                
                // Extract device class (4 bytes)
                device.deviceClass = (static_cast<uint32_t>(packet.data[8]) << 24) |
                                   (static_cast<uint32_t>(packet.data[9]) << 16) |
                                   (static_cast<uint32_t>(packet.data[10]) << 8) |
                                   packet.data[11];
                
                device.type = classifyDevice(device.deviceClass);
                device.paired = false;
                device.connected = false;
                device.lastSeen = xTaskGetTickCount() * portTICK_PERIOD_MS;

                // Extract device name if present
                if (packet.length > 12) {
                    device.name = std::string(reinterpret_cast<const char*>(&packet.data[12]),
                                            packet.length - 12);
                } else {
                    device.name = "Unknown Device";
                }

                addDiscoveredDevice(device);
                m_devicesDiscovered++;

                ESP_LOGI(TAG, "Discovered device: %s (%s) RSSI: %d",
                         device.name.c_str(), device.address.c_str(), device.rssi);

                // Call discovery callback
                if (m_discoveryCallback) {
                    m_discoveryCallback(device);
                }
            }
            break;
        }

        case C6Event::BT_DEVICE_CONNECTED: {
            if (packet.length >= 6) {
                std::string address = formatMACAddress(packet.data);
                updateDeviceConnection(address, BluetoothConnectionState::CONNECTED);
                
                ESP_LOGI(TAG, "Device connected: %s", address.c_str());
                m_successfulConnections++;
            }
            break;
        }

        case C6Event::BT_DEVICE_DISCONNECTED: {
            if (packet.length >= 6) {
                std::string address = formatMACAddress(packet.data);
                updateDeviceConnection(address, BluetoothConnectionState::DISCONNECTED);
                
                ESP_LOGI(TAG, "Device disconnected: %s", address.c_str());
            }
            break;
        }

        case C6Event::BT_DISCOVERY_COMPLETE:
            m_discovering = false;
            ESP_LOGI(TAG, "Device discovery complete. Found %d devices", 
                     m_discoveredDevices.size());
            break;

        case C6Event::HID_KEYBOARD_REPORT:
            if (packet.length >= 8) {
                processHIDReport(packet.data, packet.length, HIDReportType::KEYBOARD);
                m_dataPacketsReceived++;
            }
            break;

        case C6Event::HID_MOUSE_REPORT:
            if (packet.length >= 4) {
                processHIDReport(packet.data, packet.length, HIDReportType::MOUSE);
                m_dataPacketsReceived++;
            }
            break;

        case C6Event::HID_DEVICE_CONNECTED:
            ESP_LOGI(TAG, "HID device connected");
            m_keyboardConnected = true;
            m_mouseConnected = true;
            break;

        case C6Event::HID_DEVICE_DISCONNECTED:
            ESP_LOGI(TAG, "HID device disconnected");
            m_keyboardConnected = false;
            m_mouseConnected = false;
            break;

        case C6Event::AUDIO_CONNECTED:
            ESP_LOGI(TAG, "Audio device connected");
            m_audioDeviceConnected = true;
            m_audioDeviceInfo.connectionTime = xTaskGetTickCount() * portTICK_PERIOD_MS;
            break;

        case C6Event::AUDIO_DISCONNECTED:
            ESP_LOGI(TAG, "Audio device disconnected");
            m_audioDeviceConnected = false;
            break;

        case C6Event::AUDIO_PLAYING:
            m_audioDeviceInfo.isPlaying = true;
            break;

        case C6Event::AUDIO_PAUSED:
            m_audioDeviceInfo.isPlaying = false;
            break;

        case C6Event::SERIAL_DATA_RECEIVED:
            if (packet.length > 0 && m_serialMutex) {
                xSemaphoreTake(m_serialMutex, portMAX_DELAY);
                m_serialBuffer.insert(m_serialBuffer.end(), packet.data, packet.data + packet.length);
                xSemaphoreGive(m_serialMutex);

                if (m_serialCallback) {
                    std::string data(reinterpret_cast<const char*>(packet.data), packet.length);
                    m_serialCallback("", data);
                }
            }
            break;

        case C6Event::ERROR:
            ESP_LOGE(TAG, "C6 reported error");
            break;

        default:
            ESP_LOGW(TAG, "Unknown C6 event: 0x%02X", packet.command);
            break;
    }
}

uint8_t BluetoothHAL::calculateChecksum(const C6Packet& packet) {
    uint8_t checksum = packet.sync + packet.command;
    checksum += (packet.length >> 8) + (packet.length & 0xFF);
    
    for (uint16_t i = 0; i < packet.length; i++) {
        checksum += packet.data[i];
    }
    
    return ~checksum + 1; // Two's complement
}

bool BluetoothHAL::validatePacket(const C6Packet& packet) {
    if (packet.sync != SYNC_BYTE) {
        return false;
    }
    
    uint8_t expectedChecksum = calculateChecksum(packet);
    return (packet.checksum == expectedChecksum);
}

os_error_t BluetoothHAL::initializeSDIO() {
    ESP_LOGI(TAG, "Initializing SDIO communication");

    // Create SDIO event task
    BaseType_t result = xTaskCreate(
        sdioEventTask,
        "sdio_bt_events",
        4096,
        this,
        tskIDLE_PRIORITY + 3,
        &m_sdioTask
    );

    if (result != pdPASS) {
        ESP_LOGE(TAG, "Failed to create SDIO event task");
        return OS_ERROR_NO_MEMORY;
    }

    // Initialize SDIO slave interface (stub)
    // In real implementation, this would configure SDIO slave
    ESP_LOGI(TAG, "SDIO communication initialized");
    return OS_OK;
}

os_error_t BluetoothHAL::shutdownSDIO() {
    ESP_LOGI(TAG, "Shutting down SDIO communication");

    if (m_sdioTask) {
        vTaskDelete(m_sdioTask);
        m_sdioTask = nullptr;
    }

    return OS_OK;
}

void BluetoothHAL::sdioEventTask(void* parameter) {
    BluetoothHAL* hal = static_cast<BluetoothHAL*>(parameter);
    
    ESP_LOGI(TAG, "SDIO event task started");

    while (hal->m_initialized) {
        hal->handleSDIOEvents();
        vTaskDelay(pdMS_TO_TICKS(10)); // 10ms poll interval
    }

    ESP_LOGI(TAG, "SDIO event task ended");
    vTaskDelete(nullptr);
}

void BluetoothHAL::handleSDIOEvents() {
    // Stub implementation - in real hardware this would:
    // 1. Check SDIO interface for incoming data
    // 2. Parse C6Packet structures
    // 3. Queue events for processing
    // 4. Handle any SDIO errors or timeouts
}

void BluetoothHAL::addDiscoveredDevice(const BluetoothDeviceInfo& device) {
    // Check if device already exists
    for (auto& existing : m_discoveredDevices) {
        if (existing.address == device.address) {
            // Update existing device
            existing.rssi = device.rssi;
            existing.lastSeen = device.lastSeen;
            if (!device.name.empty() && device.name != "Unknown Device") {
                existing.name = device.name;
            }
            return;
        }
    }
    
    // Add new device
    m_discoveredDevices.push_back(device);
}

void BluetoothHAL::updateDeviceConnection(const std::string& address, BluetoothConnectionState state) {
    BluetoothDeviceInfo* device = findDevice(address);
    if (!device) return;

    device->connected = (state == BluetoothConnectionState::CONNECTED);
    
    if (m_connectionCallback) {
        m_connectionCallback(*device, state);
    }
}

BluetoothDeviceInfo* BluetoothHAL::findDevice(const std::string& address) {
    for (auto& device : m_discoveredDevices) {
        if (device.address == address) {
            return &device;
        }
    }
    for (auto& device : m_pairedDevices) {
        if (device.address == address) {
            return &device;
        }
    }
    return nullptr;
}

void BluetoothHAL::processHIDReport(const uint8_t* data, size_t length, HIDReportType type) {
    if (type == HIDReportType::KEYBOARD && length >= 8) {
        m_latestKeyboardReport.modifier = data[0];
        m_latestKeyboardReport.reserved = data[1];
        memcpy(m_latestKeyboardReport.keys, &data[2], 6);
        m_latestKeyboardReport.timestamp = xTaskGetTickCount() * portTICK_PERIOD_MS;
        m_keyboardReportAvailable = true;

        if (m_keyboardCallback) {
            m_keyboardCallback(m_latestKeyboardReport);
        }
    } else if (type == HIDReportType::MOUSE && length >= 4) {
        m_latestMouseReport.buttons = data[0];
        m_latestMouseReport.x = (int16_t)((data[2] << 8) | data[1]);
        m_latestMouseReport.y = (int16_t)((data[4] << 8) | data[3]);
        if (length >= 5) {
            m_latestMouseReport.wheel = (int8_t)data[4];
        }
        if (length >= 6) {
            m_latestMouseReport.hWheel = (int8_t)data[5];
        }
        m_latestMouseReport.timestamp = xTaskGetTickCount() * portTICK_PERIOD_MS;
        m_mouseReportAvailable = true;

        if (m_mouseCallback) {
            m_mouseCallback(m_latestMouseReport);
        }
    }
}

os_error_t BluetoothHAL::configurePinSettings() {
    ESP_LOGI(TAG, "Configuring Bluetooth GPIO pins");

    // Configure ESP32-C6 reset pin
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << ESP32_C6_RESET),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };

    esp_err_t ret = gpio_config(&io_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure C6 reset pin: %s", esp_err_to_name(ret));
        return OS_ERROR_HARDWARE;
    }

    // Configure Bluetooth enable pin (if separate from C6)
    io_conf.pin_bit_mask = (1ULL << BT_EN_PIN);
    ret = gpio_config(&io_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure BT enable pin: %s", esp_err_to_name(ret));
        return OS_ERROR_HARDWARE;
    }

    // Enable Bluetooth power via C6
    gpio_set_level(static_cast<gpio_num_t>(BT_EN_PIN), 1);
    gpio_set_level(static_cast<gpio_num_t>(ESP32_C6_RESET), 1);
    
    vTaskDelay(pdMS_TO_TICKS(100)); // Allow time for power stabilization

    return OS_OK;
}

BluetoothDeviceType BluetoothHAL::classifyDevice(uint32_t deviceClass) {
    // Extract major device class
    uint8_t majorClass = (deviceClass >> 8) & 0x1F;
    uint8_t minorClass = (deviceClass >> 2) & 0x3F;

    switch (majorClass) {
        case 0x05: // Peripheral devices
            if (minorClass & 0x40) { // Keyboard bit
                if (minorClass & 0x80) { // Pointing device bit
                    return BluetoothDeviceType::COMBO_HID;
                }
                return BluetoothDeviceType::KEYBOARD;
            } else if (minorClass & 0x80) { // Pointing device bit
                return BluetoothDeviceType::MOUSE;
            } else if (minorClass & 0x08) { // Gamepad/joystick
                return BluetoothDeviceType::GAMEPAD;
            }
            break;

        case 0x04: // Audio/Video devices
            if (minorClass == 0x01 || minorClass == 0x02) { // Headset or headphones
                return BluetoothDeviceType::HEADPHONES;
            } else if (minorClass == 0x05 || minorClass == 0x06) { // Loudspeaker or portable audio
                return BluetoothDeviceType::SPEAKERS;
            }
            break;

        case 0x02: // Phone
            return BluetoothDeviceType::PHONE;

        default:
            break;
    }

    return BluetoothDeviceType::UNKNOWN;
}

std::string BluetoothHAL::formatMACAddress(const uint8_t* address) {
    char buffer[18];
    snprintf(buffer, sizeof(buffer), "%02X:%02X:%02X:%02X:%02X:%02X",
             address[0], address[1], address[2], address[3], address[4], address[5]);
    return std::string(buffer);
}

void BluetoothHAL::getBluetoothStats(uint32_t& devicesDiscovered, uint32_t& connectionAttempts, 
                                    uint32_t& successfulConnections, uint32_t& dataPacketsReceived) {
    devicesDiscovered = m_devicesDiscovered;
    connectionAttempts = m_connectionAttempts;
    successfulConnections = m_successfulConnections;
    dataPacketsReceived = m_dataPacketsReceived;
}

void BluetoothHAL::resetStats() {
    m_devicesDiscovered = 0;
    m_connectionAttempts = 0;
    m_successfulConnections = 0;
    m_dataPacketsReceived = 0;
    ESP_LOGI(TAG, "Statistics reset");
}

void BluetoothHAL::printStatus() const {
    ESP_LOGI(TAG, "=== Bluetooth HAL Status ===");
    ESP_LOGI(TAG, "Initialized: %s", m_initialized ? "Yes" : "No");
    ESP_LOGI(TAG, "C6 Ready: %s", m_c6Ready ? "Yes" : "No");
    ESP_LOGI(TAG, "Bluetooth Enabled: %s", m_bluetoothEnabled ? "Yes" : "No");
    ESP_LOGI(TAG, "Discovering: %s", m_discovering ? "Yes" : "No");
    ESP_LOGI(TAG, "HID Host: %s", m_hidHostEnabled ? "Enabled" : "Disabled");
    ESP_LOGI(TAG, "A2DP Sink: %s", m_a2dpSinkEnabled ? "Enabled" : "Disabled");
    ESP_LOGI(TAG, "Serial: %s", m_serialEnabled ? "Enabled" : "Disabled");
    ESP_LOGI(TAG, "Keyboard Connected: %s", m_keyboardConnected ? "Yes" : "No");
    ESP_LOGI(TAG, "Mouse Connected: %s", m_mouseConnected ? "Yes" : "No");
    ESP_LOGI(TAG, "Audio Device Connected: %s", m_audioDeviceConnected ? "Yes" : "No");
    ESP_LOGI(TAG, "Discovered Devices: %d", m_discoveredDevices.size());
    ESP_LOGI(TAG, "Paired Devices: %d", m_pairedDevices.size());
    ESP_LOGI(TAG, "Statistics: D:%d CA:%d SC:%d DP:%d", 
             m_devicesDiscovered, m_connectionAttempts, m_successfulConnections, m_dataPacketsReceived);
    ESP_LOGI(TAG, "============================");
}