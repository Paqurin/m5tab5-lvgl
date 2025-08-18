#ifndef RTL_SDR_SERVICE_H
#define RTL_SDR_SERVICE_H

#include "../system/os_config.h"
#include "../hal/usb_hal.h"
#include <vector>
#include <complex>
#include <string>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>

/**
 * @file rtl_sdr_service.h
 * @brief RTL-SDR USB Device Service
 * 
 * Handles USB communication with RTL2832U-based SDR dongles,
 * provides real-time I/Q data streaming, and manages device
 * configuration for frequency tuning and gain control.
 */

// RTL-SDR USB Device IDs (common dongles)
struct RTLSDRDeviceID {
    uint16_t vendorId;
    uint16_t productId;
    const char* name;
};

// RTL-SDR Tuner Types
enum class RTLSDRTuner {
    UNKNOWN = 0,
    E4000 = 1,
    FC0012 = 2,
    FC0013 = 3,
    FC2580 = 4,
    R820T = 5,
    R828D = 6
};

// RTL-SDR Configuration
struct RTLSDRConfig {
    uint32_t centerFreq = 100000000;    // 100 MHz
    uint32_t sampleRate = 2048000;      // 2.048 MHz
    float gain = 20.0f;                 // dB
    bool agcEnabled = false;
    bool biasTeePower = false;
    bool directSampling = false;
    uint32_t bufferSize = 262144;       // 256KB
    uint32_t bufferCount = 16;
};

// RTL-SDR Device Information
struct RTLSDRDeviceInfo {
    RTLSDRTuner tunerType = RTLSDRTuner::UNKNOWN;
    uint32_t tunerFreqMin = 24000000;   // 24 MHz
    uint32_t tunerFreqMax = 1700000000; // 1.7 GHz
    std::vector<float> gainValues;
    std::string serialNumber;
    std::string manufacturer;
    std::string product;
    bool supportsDirectSampling = false;
    bool supportsBiasTee = false;
};

// Sample Data Structure
struct IQSampleBuffer {
    std::vector<std::complex<float>> samples;
    uint32_t sampleRate;
    uint32_t centerFreq;
    uint64_t timestamp;
    bool overflow = false;
};

class RTLSDRService {
public:
    RTLSDRService();
    ~RTLSDRService();

    /**
     * @brief Initialize RTL-SDR service
     * @return OS_OK on success, error code on failure
     */
    os_error_t initialize();

    /**
     * @brief Shutdown RTL-SDR service
     * @return OS_OK on success, error code on failure
     */
    os_error_t shutdown();

    /**
     * @brief Detect and connect to RTL-SDR device
     * @return OS_OK on success, error code on failure
     */
    os_error_t detectDevice();

    /**
     * @brief Disconnect from RTL-SDR device
     * @return OS_OK on success, error code on failure
     */
    os_error_t disconnectDevice();

    /**
     * @brief Check if RTL-SDR service is initialized
     * @return true if service is initialized
     */
    bool isInitialized() const { return m_initialized; }

    /**
     * @brief Check if RTL-SDR device is connected
     * @return true if device is connected and ready
     */
    bool isDeviceConnected() const { return m_deviceConnected; }

    /**
     * @brief Get device information
     * @return Device information structure
     */
    const RTLSDRDeviceInfo& getDeviceInfo() const { return m_deviceInfo; }

    /**
     * @brief Set center frequency
     * @param frequency Frequency in Hz
     * @return OS_OK on success, error code on failure
     */
    os_error_t setFrequency(uint32_t frequency);

    /**
     * @brief Set sample rate
     * @param sampleRate Sample rate in samples/second
     * @return OS_OK on success, error code on failure
     */
    os_error_t setSampleRate(uint32_t sampleRate);

    /**
     * @brief Set gain
     * @param gain Gain in dB
     * @return OS_OK on success, error code on failure
     */
    os_error_t setGain(float gain);

    /**
     * @brief Enable/disable AGC
     * @param enabled AGC enable state
     * @return OS_OK on success, error code on failure
     */
    os_error_t setAGC(bool enabled);

    /**
     * @brief Enable/disable bias tee power
     * @param enabled Bias tee enable state
     * @return OS_OK on success, error code on failure
     */
    os_error_t setBiasTee(bool enabled);

    /**
     * @brief Start streaming I/Q data
     * @return OS_OK on success, error code on failure
     */
    os_error_t startStreaming();

    /**
     * @brief Stop streaming I/Q data
     * @return OS_OK on success, error code on failure
     */
    os_error_t stopStreaming();

    /**
     * @brief Check if streaming is active
     * @return true if streaming
     */
    bool isStreaming() const { return m_streaming; }

    /**
     * @brief Get next I/Q sample buffer
     * @param buffer Output buffer
     * @param timeoutMs Timeout in milliseconds
     * @return OS_OK on success, OS_ERROR_TIMEOUT on timeout
     */
    os_error_t getNextBuffer(IQSampleBuffer& buffer, uint32_t timeoutMs = 1000);

    /**
     * @brief Register data callback
     * @param callback Callback function
     * @param userData User data for callback
     */
    void registerDataCallback(void(*callback)(const IQSampleBuffer&, void*), void* userData);

    /**
     * @brief Get streaming statistics
     */
    void getStreamingStats(uint32_t& samplesReceived, uint32_t& bufferOverruns, float& dataRate);

    /**
     * @brief Reset streaming statistics
     */
    void resetStats();

    /**
     * @brief Get current configuration
     * @return Current RTL-SDR configuration
     */
    const RTLSDRConfig& getConfig() const { return m_config; }

    /**
     * @brief Apply configuration
     * @param config Configuration to apply
     * @return OS_OK on success, error code on failure
     */
    os_error_t applyConfig(const RTLSDRConfig& config);

private:
    /**
     * @brief Initialize USB communication with RTL-SDR
     * @return OS_OK on success, error code on failure
     */
    os_error_t initializeUSB();

    /**
     * @brief Send control command to RTL-SDR
     * @param request USB request type
     * @param value Request value
     * @param index Request index
     * @param data Data buffer
     * @param length Data length
     * @return OS_OK on success, error code on failure
     */
    os_error_t sendControlCommand(uint8_t request, uint16_t value, uint16_t index, 
                                  uint8_t* data = nullptr, uint16_t length = 0);

    /**
     * @brief Read register from RTL2832U
     * @param block Register block
     * @param addr Register address
     * @param data Output data
     * @param length Data length
     * @return OS_OK on success, error code on failure
     */
    os_error_t readRegister(uint8_t block, uint16_t addr, uint8_t* data, uint8_t length);

    /**
     * @brief Write register to RTL2832U
     * @param block Register block
     * @param addr Register address
     * @param data Input data
     * @param length Data length
     * @return OS_OK on success, error code on failure
     */
    os_error_t writeRegister(uint8_t block, uint16_t addr, uint8_t* data, uint8_t length);

    /**
     * @brief Detect tuner type
     * @return OS_OK on success, error code on failure
     */
    os_error_t detectTuner();

    /**
     * @brief Initialize tuner
     * @return OS_OK on success, error code on failure
     */
    os_error_t initializeTuner();

    /**
     * @brief Set tuner frequency
     * @param frequency Frequency in Hz
     * @return OS_OK on success, error code on failure
     */
    os_error_t setTunerFrequency(uint32_t frequency);

    /**
     * @brief Set tuner gain
     * @param gain Gain in dB
     * @return OS_OK on success, error code on failure
     */
    os_error_t setTunerGain(float gain);

    /**
     * @brief Convert raw samples to I/Q
     * @param rawData Raw 8-bit I/Q data
     * @param length Data length
     * @param iqSamples Output I/Q samples
     */
    void convertSamples(const uint8_t* rawData, size_t length, std::vector<std::complex<float>>& iqSamples);

    /**
     * @brief USB data reception task
     * @param parameter Task parameter
     */
    static void usbDataTask(void* parameter);

    /**
     * @brief Process received USB data
     * @param data Raw data buffer
     * @param length Data length
     */
    void processUSBData(const uint8_t* data, size_t length);

    /**
     * @brief USB event handler
     * @param portType USB port type
     * @param status Connection status
     * @param userData User data
     */
    static void usbEventHandler(USBPortType portType, USBConnectionStatus status, void* userData);

    // USB HAL reference
    USBHAL* m_usbHAL;
    
    // Device state
    bool m_initialized = false;
    bool m_deviceConnected = false;
    bool m_streaming = false;
    USBDeviceInfo m_usbDeviceInfo;
    RTLSDRDeviceInfo m_deviceInfo;
    RTLSDRConfig m_config;
    
    // USB transfer buffers
    std::vector<uint8_t> m_usbBuffer;
    std::vector<uint8_t*> m_transferBuffers;
    size_t m_currentBuffer = 0;
    
    // I/Q sample buffers
    QueueHandle_t m_sampleQueue;
    std::vector<IQSampleBuffer> m_sampleBuffers;
    size_t m_bufferIndex = 0;
    
    // Threading
    TaskHandle_t m_usbTask = nullptr;
    SemaphoreHandle_t m_deviceMutex = nullptr;
    
    // Data callback
    void(*m_dataCallback)(const IQSampleBuffer&, void*) = nullptr;
    void* m_callbackUserData = nullptr;
    
    // Statistics
    uint32_t m_samplesReceived = 0;
    uint32_t m_bufferOverruns = 0;
    uint32_t m_lastStatsTime = 0;
    float m_dataRate = 0.0f;
    
    // Known RTL-SDR devices
    static const RTLSDRDeviceID KNOWN_DEVICES[];
    static const size_t KNOWN_DEVICE_COUNT;
    
    // RTL2832U Register constants
    static constexpr uint8_t BLOCK_DEMOD = 0;
    static constexpr uint8_t BLOCK_USB = 1;
    static constexpr uint8_t BLOCK_SYS = 2;
    
    // USB endpoints
    static constexpr uint8_t BULK_IN_ENDPOINT = 0x81;
    static constexpr uint8_t CTRL_ENDPOINT = 0x00;
    
    static constexpr const char* TAG = "RTLSDRService";
};

#endif // RTL_SDR_SERVICE_H