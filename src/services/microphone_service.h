#ifndef MICROPHONE_SERVICE_H
#define MICROPHONE_SERVICE_H

#include "../system/os_config.h"
#include "../hal/hardware_config.h"
#include <vector>
#include <memory>

// Forward declarations for I2S types
#ifdef CONFIG_ESP_AUDIO_SUPPORTED
#include <driver/i2s.h>
#else
typedef enum {
    I2S_MODE_MASTER = 1,
    I2S_MODE_SLAVE = 2,
    I2S_MODE_TX = 4,
    I2S_MODE_RX = 8,
    I2S_MODE_PDM = 64
} i2s_mode_t;

typedef enum {
    I2S_BITS_PER_SAMPLE_16BIT = 16,
    I2S_BITS_PER_SAMPLE_32BIT = 32
} i2s_bits_per_sample_t;

typedef enum {
    I2S_CHANNEL_FMT_RIGHT_LEFT,
    I2S_CHANNEL_FMT_ALL_RIGHT,
    I2S_CHANNEL_FMT_ALL_LEFT,
    I2S_CHANNEL_FMT_ONLY_RIGHT,
    I2S_CHANNEL_FMT_ONLY_LEFT
} i2s_channel_fmt_t;

typedef enum {
    I2S_COMM_FORMAT_I2S = 0x01,
    I2S_COMM_FORMAT_PCM = 0x04
} i2s_comm_format_t;

typedef enum {
    I2S_NUM_0 = 0,
    I2S_NUM_1 = 1
} i2s_port_t;
#endif

/**
 * @file microphone_service.h
 * @brief Dual Microphone Service for M5Stack Tab5
 * 
 * Provides dual microphone support with noise cancellation, beamforming,
 * and advanced audio processing capabilities using PDM digital microphones
 * and analog microphone via ES8388 ADC.
 */

enum class MicrophoneType {
    PRIMARY_PDM,      // Front-facing PDM microphone
    SECONDARY_PDM,    // Rear-facing PDM microphone
    ANALOG_ES8388,    // Analog microphone via ES8388 ADC
    DUAL_PDM,         // Both PDM microphones combined
    ALL               // All available microphones
};

enum class MicrophoneMode {
    MONO,             // Single channel recording
    STEREO,           // Dual channel recording
    BEAMFORMING,      // Directional audio capture
    NOISE_CANCEL,     // Noise cancellation mode
    VOICE_ENHANCE     // Voice enhancement mode
};

enum class NoiseReductionLevel {
    OFF,
    LOW,
    MEDIUM,
    HIGH,
    ADAPTIVE          // Adaptive based on environment
};

struct MicrophoneConfig {
    MicrophoneType primarySource = MicrophoneType::DUAL_PDM;
    MicrophoneMode mode = MicrophoneMode::VOICE_ENHANCE;
    uint32_t sampleRate = 16000;        // 16kHz for voice
    i2s_bits_per_sample_t bitDepth = I2S_BITS_PER_SAMPLE_16BIT;
    uint8_t gain = 50;                  // 0-100
    bool autoGainControl = true;
    NoiseReductionLevel noiseReduction = NoiseReductionLevel::ADAPTIVE;
    bool echoCancellation = true;
    bool voiceActivityDetection = true;
    float beamformingAngle = 0.0f;      // Degrees, 0 = front
    uint16_t bufferSize = 1024;         // Samples per buffer
};

struct AudioBuffer {
    std::vector<int16_t> data;
    uint32_t sampleRate;
    uint8_t channels;
    uint32_t timestamp;
    float amplitude;
    bool voiceDetected;
};

struct VoiceMetrics {
    float signalLevel;          // dB
    float noiseLevel;           // dB
    float snrRatio;             // Signal-to-noise ratio
    float voiceProbability;     // 0.0-1.0
    uint32_t voiceDuration;     // ms
    uint32_t silenceDuration;   // ms
};

class MicrophoneService {
public:
    MicrophoneService() = default;
    ~MicrophoneService();

    /**
     * @brief Initialize microphone service
     * @return OS_OK on success, error code on failure
     */
    os_error_t initialize();

    /**
     * @brief Shutdown microphone service
     * @return OS_OK on success, error code on failure
     */
    os_error_t shutdown();

    /**
     * @brief Update microphone service
     * @param deltaTime Time since last update in milliseconds
     * @return OS_OK on success, error code on failure
     */
    os_error_t update(uint32_t deltaTime);

    /**
     * @brief Configure microphone settings
     * @param config Microphone configuration
     * @return OS_OK on success, error code on failure
     */
    os_error_t configure(const MicrophoneConfig& config);

    /**
     * @brief Start audio recording
     * @return OS_OK on success, error code on failure
     */
    os_error_t startRecording();

    /**
     * @brief Stop audio recording
     * @return OS_OK on success, error code on failure
     */
    os_error_t stopRecording();

    /**
     * @brief Check if currently recording
     * @return true if recording is active
     */
    bool isRecording() const { return m_recording; }

    /**
     * @brief Read audio buffer
     * @param buffer Output audio buffer
     * @return OS_OK on success, error code on failure
     */
    os_error_t readAudioBuffer(AudioBuffer& buffer);

    /**
     * @brief Set microphone gain
     * @param gain Gain level (0-100)
     * @return OS_OK on success, error code on failure
     */
    os_error_t setGain(uint8_t gain);

    /**
     * @brief Get current gain level
     * @return Gain level (0-100)
     */
    uint8_t getGain() const { return m_config.gain; }

    /**
     * @brief Set noise reduction level
     * @param level Noise reduction level
     * @return OS_OK on success, error code on failure
     */
    os_error_t setNoiseReduction(NoiseReductionLevel level);

    /**
     * @brief Enable/disable voice activity detection
     * @param enabled true to enable VAD
     * @return OS_OK on success, error code on failure
     */
    os_error_t setVoiceActivityDetection(bool enabled);

    /**
     * @brief Set beamforming direction
     * @param angle Angle in degrees (0 = front, 90 = right, etc.)
     * @return OS_OK on success, error code on failure
     */
    os_error_t setBeamformingAngle(float angle);

    /**
     * @brief Get current voice metrics
     * @return Voice metrics structure
     */
    const VoiceMetrics& getVoiceMetrics() const { return m_voiceMetrics; }

    /**
     * @brief Calibrate microphones for environment
     * @param calibrationTime Calibration duration in milliseconds
     * @return OS_OK on success, error code on failure
     */
    os_error_t calibrateForEnvironment(uint32_t calibrationTime = 3000);

    /**
     * @brief Get available microphone types
     * @param types Vector to store available types
     * @return OS_OK on success, error code on failure
     */
    os_error_t getAvailableMicrophones(std::vector<MicrophoneType>& types);

    /**
     * @brief Test microphone functionality
     * @param type Microphone type to test
     * @return OS_OK on success, error code on failure
     */
    os_error_t testMicrophone(MicrophoneType type);

    /**
     * @brief Get microphone statistics
     */
    void printMicrophoneStats() const;

private:
    /**
     * @brief Initialize PDM microphones
     * @return OS_OK on success, error code on failure
     */
    os_error_t initializePDMMicrophones();

    /**
     * @brief Initialize analog microphone via ES8388
     * @return OS_OK on success, error code on failure
     */
    os_error_t initializeAnalogMicrophone();

    /**
     * @brief Configure I2S for microphone input
     * @param port I2S port number
     * @param isPDM true for PDM configuration
     * @return OS_OK on success, error code on failure
     */
    os_error_t configureI2S(i2s_port_t port, bool isPDM);

    /**
     * @brief Read raw audio data from I2S
     * @param port I2S port
     * @param buffer Output buffer
     * @param length Buffer length
     * @param bytesRead Bytes actually read
     * @return OS_OK on success, error code on failure
     */
    os_error_t readI2SData(i2s_port_t port, uint8_t* buffer, size_t length, size_t* bytesRead);

    /**
     * @brief Apply noise reduction to audio data
     * @param input Input audio buffer
     * @param output Output processed buffer
     * @return OS_OK on success, error code on failure
     */
    os_error_t applyNoiseReduction(const AudioBuffer& input, AudioBuffer& output);

    /**
     * @brief Apply beamforming to dual microphone input
     * @param primary Primary microphone buffer
     * @param secondary Secondary microphone buffer
     * @param output Beamformed output buffer
     * @return OS_OK on success, error code on failure
     */
    os_error_t applyBeamforming(const AudioBuffer& primary, const AudioBuffer& secondary, AudioBuffer& output);

    /**
     * @brief Detect voice activity in audio buffer
     * @param buffer Audio buffer to analyze
     * @return true if voice activity detected
     */
    bool detectVoiceActivity(const AudioBuffer& buffer);

    /**
     * @brief Calculate audio amplitude
     * @param buffer Audio buffer
     * @return RMS amplitude
     */
    float calculateAmplitude(const AudioBuffer& buffer);

    /**
     * @brief Update voice metrics
     * @param buffer Current audio buffer
     */
    void updateVoiceMetrics(const AudioBuffer& buffer);

    /**
     * @brief Apply automatic gain control
     * @param buffer Audio buffer to process
     */
    void applyAutoGainControl(AudioBuffer& buffer);

    /**
     * @brief Process audio data
     * @param rawData Raw I2S data
     * @param length Data length
     * @param output Processed audio buffer
     * @return OS_OK on success, error code on failure
     */
    os_error_t processAudioData(const uint8_t* rawData, size_t length, AudioBuffer& output);

    // Configuration
    MicrophoneConfig m_config;
    
    // State
    bool m_initialized = false;
    bool m_pdmInitialized = false;
    bool m_analogInitialized = false;
    bool m_recording = false;
    bool m_calibrated = false;

    // Hardware handles
    i2s_port_t m_pdmPort = I2S_NUM_1;
    i2s_port_t m_analogPort = I2S_NUM_0;

    // Audio processing
    std::vector<AudioBuffer> m_bufferQueue;
    VoiceMetrics m_voiceMetrics;
    float m_noiseFloor = -60.0f;        // dB
    float m_voiceThreshold = -40.0f;    // dB
    uint32_t m_voiceStartTime = 0;
    uint32_t m_silenceStartTime = 0;

    // Noise reduction state
    std::vector<float> m_noiseProfile;
    float m_adaptiveGain = 1.0f;
    uint32_t m_lastNoiseUpdate = 0;

    // Beamforming parameters
    float m_microphoneDistance = 0.02f; // 2cm spacing
    float m_soundSpeed = 343.0f;        // m/s at 20°C

    // Statistics
    uint32_t m_samplesProcessed = 0;
    uint32_t m_voiceSegments = 0;
    uint32_t m_noiseEvents = 0;
    uint32_t m_bufferOverruns = 0;
    uint32_t m_calibrationCount = 0;

    // Configuration constants
    static constexpr size_t MAX_BUFFER_QUEUE = 8;
    static constexpr float VAD_THRESHOLD = 0.3f;
    static constexpr uint32_t NOISE_UPDATE_INTERVAL = 1000; // ms
    static constexpr uint32_t VOICE_TIMEOUT = 2000;         // ms
    static constexpr uint32_t SILENCE_TIMEOUT = 1000;       // ms
};

#endif // MICROPHONE_SERVICE_H