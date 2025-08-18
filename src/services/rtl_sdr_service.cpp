#include "rtl_sdr_service.h"
#include "../hal/usb_hal.h"
#include <esp_log.h>
#include <esp_timer.h>
#include <esp_random.h>
#include <string.h>
#include <cfloat>

/**
 * @file rtl_sdr_service.cpp
 * @brief RTL-SDR Service Implementation
 */

// Known RTL-SDR device IDs
const RTLSDRDeviceID RTLSDRService::KNOWN_DEVICES[] = {
    {0x0bda, 0x2832, "Generic RTL2832U"},
    {0x0bda, 0x2838, "RTL2832U+R820T2"},
    {0x0ccd, 0x00a9, "Terratec Cinergy T Stick Black"},
    {0x0ccd, 0x00b3, "Terratec NOXON DAB/DAB+ Stick"},
    {0x0ccd, 0x00b4, "Terratec Deutschlandradio DAB Stick"},
    {0x0ccd, 0x00b7, "Terratec T Stick PLUS"},
    {0x0ccd, 0x00d3, "Terratec T Stick RC (Rev.3)"},
    {0x0ccd, 0x00d7, "Terratec T Stick+"},
    {0x0ccd, 0x00e0, "Terratec NOXON DAB Stick - Radio Energy"},
    {0x1554, 0x5020, "PixelView PV-DT235U(RN)"},
    {0x15f4, 0x0131, "HanfTek DAB+FM+DVB-T"},
    {0x185b, 0x0620, "Compro Videomate U620F"},
    {0x185b, 0x0650, "Compro Videomate U650F"},
    {0x185b, 0x0680, "Compro Videomate U680F"},
    {0x1b80, 0xd393, "GIGABYTE GT-U7300"},
    {0x1b80, 0xd394, "GIGABYTE GT-U7300"},
    {0x1b80, 0xd395, "GIGABYTE GT-U7300"},
    {0x1b80, 0xd397, "GIGABYTE GT-U7300"},
    {0x1b80, 0xd398, "GIGABYTE GT-U7300"},
    {0x1b80, 0xd39d, "GIGABYTE GT-U7300"},
    {0x1b80, 0xd3a4, "Twintech UT-40"},
    {0x1b80, 0xd3a8, "GIGABYTE GT-U7300"},
    {0x1d19, 0x1101, "Dexatek DK DVB-T Dongle (Logilink VG0002A)"},
    {0x1d19, 0x1102, "Dexatek DK DVB-T Dongle (MSI DigiVox mini II V3.0)"},
    {0x1d19, 0x1103, "Dexatek Technology Ltd. DK 5217 DVB-T Dongle"},
    {0x1f4d, 0xa803, "Sweex DVB-T USB"},
    {0x1f4d, 0xb803, "GTek T803"},
    {0x1f4d, 0xc803, "Lifeview LV5TDeluxe"},
    {0x1f4d, 0xd286, "MyGica TD312"},
    {0x1f4d, 0xd803, "PROlectrix DV107669"},
};

const size_t RTLSDRService::KNOWN_DEVICE_COUNT = sizeof(KNOWN_DEVICES) / sizeof(KNOWN_DEVICES[0]);

RTLSDRService::RTLSDRService() {
    // Initialize USB buffer
    m_usbBuffer.resize(m_config.bufferSize);
    
    // Initialize transfer buffers
    m_transferBuffers.resize(m_config.bufferCount);
    for (size_t i = 0; i < m_config.bufferCount; i++) {
        m_transferBuffers[i] = new uint8_t[m_config.bufferSize];
    }
    
    // Initialize sample buffers
    m_sampleBuffers.resize(m_config.bufferCount);
    for (size_t i = 0; i < m_config.bufferCount; i++) {
        m_sampleBuffers[i].samples.reserve(m_config.bufferSize / 2);
    }
}

RTLSDRService::~RTLSDRService() {
    shutdown();
    
    // Cleanup transfer buffers
    for (uint8_t* buffer : m_transferBuffers) {
        delete[] buffer;
    }
}

os_error_t RTLSDRService::initialize() {
    ESP_LOGI(TAG, "Initializing RTL-SDR Service");
    
    if (m_initialized) {
        return OS_OK;
    }
    
    // Create device mutex
    m_deviceMutex = xSemaphoreCreateMutex();
    if (!m_deviceMutex) {
        ESP_LOGE(TAG, "Failed to create device mutex");
        return OS_ERROR_NO_MEMORY;
    }
    
    // Create sample queue
    m_sampleQueue = xQueueCreate(m_config.bufferCount, sizeof(IQSampleBuffer));
    if (!m_sampleQueue) {
        ESP_LOGE(TAG, "Failed to create sample queue");
        return OS_ERROR_NO_MEMORY;
    }
    
    // Initialize USB HAL
    m_usbHAL = new USBHAL();
    if (!m_usbHAL) {
        ESP_LOGE(TAG, "Failed to create USB HAL");
        return OS_ERROR_NO_MEMORY;
    }
    
    os_error_t result = m_usbHAL->initialize();
    if (result != OS_OK) {
        ESP_LOGE(TAG, "Failed to initialize USB HAL: %d", result);
        return result;
    }
    
    // Register USB event callback
    m_usbHAL->registerDeviceCallback(usbEventHandler, this);
    
    m_initialized = true;
    ESP_LOGI(TAG, "RTL-SDR Service initialized successfully");
    
    return OS_OK;
}

os_error_t RTLSDRService::shutdown() {
    if (!m_initialized) {
        return OS_OK;
    }
    
    ESP_LOGI(TAG, "Shutting down RTL-SDR Service");
    
    // Stop streaming if active
    if (m_streaming) {
        stopStreaming();
    }
    
    // Disconnect device if connected
    if (m_deviceConnected) {
        disconnectDevice();
    }
    
    // Stop USB task
    if (m_usbTask) {
        vTaskDelete(m_usbTask);
        m_usbTask = nullptr;
    }
    
    // Cleanup USB HAL
    if (m_usbHAL) {
        m_usbHAL->shutdown();
        delete m_usbHAL;
        m_usbHAL = nullptr;
    }
    
    // Cleanup synchronization objects
    if (m_sampleQueue) {
        vQueueDelete(m_sampleQueue);
        m_sampleQueue = nullptr;
    }
    
    if (m_deviceMutex) {
        vSemaphoreDelete(m_deviceMutex);
        m_deviceMutex = nullptr;
    }
    
    m_initialized = false;
    return OS_OK;
}

os_error_t RTLSDRService::detectDevice() {
    if (!m_initialized || !m_usbHAL) {
        return OS_ERROR_NOT_INITIALIZED;
    }
    
    ESP_LOGI(TAG, "Detecting RTL-SDR device...");
    
    // Check for connected USB devices
    for (USBPortType port : {USBPortType::USB_A_HOST, USBPortType::USB_C_OTG}) {
        if (m_usbHAL->isDeviceConnected(port)) {
            USBDeviceInfo deviceInfo = m_usbHAL->getDeviceInfo(port);
            
            // Check if this is a known RTL-SDR device
            for (size_t i = 0; i < KNOWN_DEVICE_COUNT; i++) {
                if (deviceInfo.vendorId == KNOWN_DEVICES[i].vendorId &&
                    deviceInfo.productId == KNOWN_DEVICES[i].productId) {
                    
                    ESP_LOGI(TAG, "Found RTL-SDR device: %s (VID:%04X PID:%04X)", 
                             KNOWN_DEVICES[i].name, deviceInfo.vendorId, deviceInfo.productId);
                    
                    m_usbDeviceInfo = deviceInfo;
                    m_deviceInfo.manufacturer = deviceInfo.manufacturer;
                    m_deviceInfo.product = deviceInfo.product;
                    m_deviceInfo.serialNumber = deviceInfo.serialNumber;
                    m_deviceConnected = true;
                    
                    // Initialize RTL-SDR communication
                    os_error_t result = initializeUSB();
                    if (result == OS_OK) {
                        result = detectTuner();
                        if (result == OS_OK) {
                            result = initializeTuner();
                        }
                    }
                    
                    return result;
                }
            }
            
            ESP_LOGW(TAG, "Unknown USB device: VID:%04X PID:%04X", 
                     deviceInfo.vendorId, deviceInfo.productId);
        }
    }
    
    ESP_LOGW(TAG, "No RTL-SDR device found");
    return OS_ERROR_NOT_FOUND;
}

os_error_t RTLSDRService::disconnectDevice() {
    if (!m_deviceConnected) {
        return OS_OK;
    }
    
    ESP_LOGI(TAG, "Disconnecting RTL-SDR device");
    
    // Stop streaming if active
    if (m_streaming) {
        stopStreaming();
    }
    
    m_deviceConnected = false;
    
    return OS_OK;
}

os_error_t RTLSDRService::setFrequency(uint32_t frequency) {
    if (!m_deviceConnected) {
        return OS_ERROR_NOT_AVAILABLE;
    }
    
    if (frequency < m_deviceInfo.tunerFreqMin || frequency > m_deviceInfo.tunerFreqMax) {
        ESP_LOGW(TAG, "Frequency %u Hz out of range (%u - %u Hz)", 
                 frequency, m_deviceInfo.tunerFreqMin, m_deviceInfo.tunerFreqMax);
        return OS_ERROR_INVALID_PARAM;
    }
    
    m_config.centerFreq = frequency;
    
    return setTunerFrequency(frequency);
}

os_error_t RTLSDRService::setSampleRate(uint32_t sampleRate) {
    if (!m_deviceConnected) {
        return OS_ERROR_NOT_AVAILABLE;
    }
    
    // RTL-SDR typically supports 225 kHz - 3.2 MHz sample rates
    if (sampleRate < 225000 || sampleRate > 3200000) {
        ESP_LOGW(TAG, "Sample rate %u Hz out of supported range", sampleRate);
        return OS_ERROR_INVALID_PARAM;
    }
    
    m_config.sampleRate = sampleRate;
    
    // Configure RTL2832U sample rate
    // This is a simplified implementation - real RTL-SDR requires complex frequency synthesis
    ESP_LOGI(TAG, "Set sample rate to %u Hz", sampleRate);
    
    return OS_OK;
}

os_error_t RTLSDRService::setGain(float gain) {
    if (!m_deviceConnected) {
        return OS_ERROR_NOT_AVAILABLE;
    }
    
    // Find closest supported gain value
    float closestGain = 0.0f;
    float minDiff = FLT_MAX;
    
    for (float supportedGain : m_deviceInfo.gainValues) {
        float diff = fabs(gain - supportedGain);
        if (diff < minDiff) {
            minDiff = diff;
            closestGain = supportedGain;
        }
    }
    
    m_config.gain = closestGain;
    
    return setTunerGain(closestGain);
}

os_error_t RTLSDRService::setAGC(bool enabled) {
    if (!m_deviceConnected) {
        return OS_ERROR_NOT_AVAILABLE;
    }
    
    m_config.agcEnabled = enabled;
    
    ESP_LOGI(TAG, "AGC %s", enabled ? "enabled" : "disabled");
    
    return OS_OK;
}

os_error_t RTLSDRService::setBiasTee(bool enabled) {
    if (!m_deviceConnected || !m_deviceInfo.supportsBiasTee) {
        return OS_ERROR_NOT_SUPPORTED;
    }
    
    m_config.biasTeePower = enabled;
    
    ESP_LOGI(TAG, "Bias tee %s", enabled ? "enabled" : "disabled");
    
    return OS_OK;
}

os_error_t RTLSDRService::startStreaming() {
    if (!m_deviceConnected) {
        return OS_ERROR_NOT_AVAILABLE;
    }
    
    if (m_streaming) {
        return OS_OK;
    }
    
    ESP_LOGI(TAG, "Starting RTL-SDR data streaming");
    
    // Reset statistics
    resetStats();
    
    // Create USB data task
    BaseType_t result = xTaskCreate(
        usbDataTask,
        "rtlsdr_usb",
        8192,  // Stack size
        this,
        OS_TASK_PRIORITY_HIGH,
        &m_usbTask
    );
    
    if (result != pdPASS) {
        ESP_LOGE(TAG, "Failed to create USB data task");
        return OS_ERROR_GENERIC;
    }
    
    m_streaming = true;
    
    ESP_LOGI(TAG, "RTL-SDR streaming started");
    
    return OS_OK;
}

os_error_t RTLSDRService::stopStreaming() {
    if (!m_streaming) {
        return OS_OK;
    }
    
    ESP_LOGI(TAG, "Stopping RTL-SDR data streaming");
    
    m_streaming = false;
    
    // Stop USB task
    if (m_usbTask) {
        vTaskDelete(m_usbTask);
        m_usbTask = nullptr;
    }
    
    ESP_LOGI(TAG, "RTL-SDR streaming stopped");
    
    return OS_OK;
}

os_error_t RTLSDRService::getNextBuffer(IQSampleBuffer& buffer, uint32_t timeoutMs) {
    if (!m_streaming || !m_sampleQueue) {
        return OS_ERROR_NOT_AVAILABLE;
    }
    
    IQSampleBuffer queuedBuffer;
    if (xQueueReceive(m_sampleQueue, &queuedBuffer, pdMS_TO_TICKS(timeoutMs)) == pdTRUE) {
        buffer = std::move(queuedBuffer);
        return OS_OK;
    }
    
    return OS_ERROR_TIMEOUT;
}

void RTLSDRService::registerDataCallback(void(*callback)(const IQSampleBuffer&, void*), void* userData) {
    m_dataCallback = callback;
    m_callbackUserData = userData;
}

void RTLSDRService::getStreamingStats(uint32_t& samplesReceived, uint32_t& bufferOverruns, float& dataRate) {
    samplesReceived = m_samplesReceived;
    bufferOverruns = m_bufferOverruns;
    
    uint32_t currentTime = xTaskGetTickCount();
    uint32_t timeDiff = currentTime - m_lastStatsTime;
    
    if (timeDiff > 0) {
        m_dataRate = (float)(m_samplesReceived * 1000) / timeDiff;  // samples/second
    }
    
    dataRate = m_dataRate;
}

void RTLSDRService::resetStats() {
    m_samplesReceived = 0;
    m_bufferOverruns = 0;
    m_dataRate = 0.0f;
    m_lastStatsTime = xTaskGetTickCount();
}

os_error_t RTLSDRService::applyConfig(const RTLSDRConfig& config) {
    os_error_t result;
    
    result = setFrequency(config.centerFreq);
    if (result != OS_OK) return result;
    
    result = setSampleRate(config.sampleRate);
    if (result != OS_OK) return result;
    
    result = setGain(config.gain);
    if (result != OS_OK) return result;
    
    result = setAGC(config.agcEnabled);
    if (result != OS_OK) return result;
    
    result = setBiasTee(config.biasTeePower);
    if (result != OS_OK) return result;
    
    m_config = config;
    
    return OS_OK;
}

// Private method implementations

os_error_t RTLSDRService::initializeUSB() {
    ESP_LOGI(TAG, "Initializing USB communication");
    
    // This is a stub implementation
    // Real implementation would involve USB control transfers to initialize RTL2832U
    
    return OS_OK;
}

os_error_t RTLSDRService::sendControlCommand(uint8_t request, uint16_t value, uint16_t index, uint8_t* data, uint16_t length) {
    // This is a stub implementation for USB control transfers
    // Real implementation would use USB HAL to send control commands to RTL2832U
    
    ESP_LOGD(TAG, "Control command: req=0x%02X val=0x%04X idx=0x%04X len=%u", 
             request, value, index, length);
    
    return OS_OK;
}

os_error_t RTLSDRService::readRegister(uint8_t block, uint16_t addr, uint8_t* data, uint8_t length) {
    // RTL2832U register read via USB control transfer
    return sendControlCommand(0x00, addr, (block << 8) | 0x20, data, length);
}

os_error_t RTLSDRService::writeRegister(uint8_t block, uint16_t addr, uint8_t* data, uint8_t length) {
    // RTL2832U register write via USB control transfer  
    return sendControlCommand(0x00, addr, (block << 8) | 0x10, data, length);
}

os_error_t RTLSDRService::detectTuner() {
    ESP_LOGI(TAG, "Detecting tuner type");
    
    // This is a simplified tuner detection
    // Real implementation would read tuner ID registers
    
    m_deviceInfo.tunerType = RTLSDRTuner::R820T;  // Most common
    m_deviceInfo.tunerFreqMin = 24000000;   // 24 MHz
    m_deviceInfo.tunerFreqMax = 1700000000; // 1.7 GHz
    
    // Populate typical gain values for R820T
    m_deviceInfo.gainValues = {
        0.0f, 0.9f, 1.4f, 2.7f, 3.7f, 7.7f, 8.7f, 12.5f, 
        14.4f, 15.7f, 16.6f, 19.7f, 20.7f, 22.9f, 25.4f, 
        28.0f, 29.7f, 32.8f, 33.8f, 36.4f, 37.2f, 38.6f, 
        40.2f, 42.1f, 43.4f, 43.9f, 44.5f, 48.0f, 49.6f
    };
    
    ESP_LOGI(TAG, "Detected tuner: R820T (24 MHz - 1.7 GHz)");
    
    return OS_OK;
}

os_error_t RTLSDRService::initializeTuner() {
    ESP_LOGI(TAG, "Initializing tuner");
    
    // This would involve tuner-specific initialization
    // Each tuner type (E4000, FC0012, R820T, etc.) has different initialization sequences
    
    return OS_OK;
}

os_error_t RTLSDRService::setTunerFrequency(uint32_t frequency) {
    ESP_LOGD(TAG, "Setting tuner frequency to %u Hz", frequency);
    
    // This would involve tuner-specific frequency setting
    // Real implementation depends on the tuner type
    
    return OS_OK;
}

os_error_t RTLSDRService::setTunerGain(float gain) {
    ESP_LOGD(TAG, "Setting tuner gain to %.1f dB", gain);
    
    // This would involve tuner-specific gain setting
    // Real implementation depends on the tuner type
    
    return OS_OK;
}

void RTLSDRService::convertSamples(const uint8_t* rawData, size_t length, std::vector<std::complex<float>>& iqSamples) {
    // RTL-SDR outputs 8-bit unsigned I/Q samples interleaved (IQIQIQ...)
    // Convert to floating point complex samples centered around 0
    
    iqSamples.clear();
    iqSamples.reserve(length / 2);
    
    for (size_t i = 0; i < length; i += 2) {
        if (i + 1 < length) {
            float I = (rawData[i] - 127.5f) / 127.5f;      // Normalize to [-1, 1]
            float Q = (rawData[i + 1] - 127.5f) / 127.5f;  // Normalize to [-1, 1]
            iqSamples.emplace_back(I, Q);
        }
    }
}

void RTLSDRService::usbDataTask(void* parameter) {
    RTLSDRService* service = static_cast<RTLSDRService*>(parameter);
    
    ESP_LOGI(TAG, "USB data task started");
    
    while (service->m_streaming) {
        // Simulate receiving USB data
        // Real implementation would receive bulk transfers from RTL-SDR
        
        vTaskDelay(pdMS_TO_TICKS(10)); // 10ms delay to simulate data reception
        
        // Create synthetic I/Q data for testing
        IQSampleBuffer buffer;
        buffer.sampleRate = service->m_config.sampleRate;
        buffer.centerFreq = service->m_config.centerFreq;
        buffer.timestamp = esp_timer_get_time();
        buffer.overflow = false;
        
        // Generate some test I/Q samples (noise)
        buffer.samples.reserve(1024);
        for (int i = 0; i < 1024; i++) {
            float I = (float)(esp_random() % 256 - 128) / 128.0f;
            float Q = (float)(esp_random() % 256 - 128) / 128.0f;
            buffer.samples.emplace_back(I, Q);
        }
        
        // Call data callback if registered
        if (service->m_dataCallback && service->m_callbackUserData) {
            service->m_dataCallback(buffer, service->m_callbackUserData);
        }
        
        // Add to queue if there's space
        if (service->m_sampleQueue) {
            if (xQueueSend(service->m_sampleQueue, &buffer, 0) != pdTRUE) {
                service->m_bufferOverruns++;
            }
        }
        
        service->m_samplesReceived += buffer.samples.size();
    }
    
    ESP_LOGI(TAG, "USB data task ended");
    vTaskDelete(nullptr);
}

void RTLSDRService::processUSBData(const uint8_t* data, size_t length) {
    if (!data || length == 0) {
        return;
    }
    
    // Convert raw USB data to I/Q samples
    IQSampleBuffer buffer;
    convertSamples(data, length, buffer.samples);
    
    buffer.sampleRate = m_config.sampleRate;
    buffer.centerFreq = m_config.centerFreq;
    buffer.timestamp = esp_timer_get_time();
    buffer.overflow = false;
    
    // Call data callback if registered
    if (m_dataCallback && m_callbackUserData) {
        m_dataCallback(buffer, m_callbackUserData);
    }
    
    // Add to queue if there's space
    if (m_sampleQueue) {
        if (xQueueSend(m_sampleQueue, &buffer, 0) != pdTRUE) {
            m_bufferOverruns++;
        }
    }
    
    m_samplesReceived += buffer.samples.size();
}

void RTLSDRService::usbEventHandler(USBPortType portType, USBConnectionStatus status, void* userData) {
    RTLSDRService* service = static_cast<RTLSDRService*>(userData);
    if (!service) {
        return;
    }
    
    ESP_LOGI(TAG, "USB event: port=%d status=%d", (int)portType, (int)status);
    
    switch (status) {
        case USBConnectionStatus::CONNECTED:
            // Device connected - attempt to detect RTL-SDR
            service->detectDevice();
            break;
            
        case USBConnectionStatus::DISCONNECTED:
            // Device disconnected
            if (service->m_deviceConnected) {
                service->disconnectDevice();
            }
            break;
            
        default:
            break;
    }
}