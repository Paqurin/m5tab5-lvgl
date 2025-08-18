#ifndef AUDIO_HAL_H
#define AUDIO_HAL_H

#include "../system/os_config.h"
#include "hardware_config.h"
#include <driver/i2s.h>
#include <driver/i2c.h>
#include <driver/gpio.h>
#include <esp_log.h>

/**
 * @file audio_hal.h
 * @brief Audio Hardware Abstraction Layer for M5Stack Tab5
 * 
 * Provides comprehensive audio support including ES8388 codec,
 * dual PDM microphones with noise cancellation, NS4150 amplifiers,
 * and advanced audio processing capabilities.
 */

enum class AudioSampleRate {
    RATE_8KHZ = 8000,
    RATE_16KHZ = 16000,
    RATE_22KHZ = 22050,
    RATE_32KHZ = 32000,
    RATE_44_1KHZ = 44100,
    RATE_48KHZ = 48000,
    RATE_96KHZ = 96000
};

enum class AudioBitDepth {
    BITS_16 = 16,
    BITS_24 = 24,
    BITS_32 = 32
};

enum class AudioChannelMode {
    MONO,
    STEREO,
    DUAL_MONO
};

enum class AudioInputSource {
    MICROPHONE_PRIMARY,     // Front-facing PDM microphone
    MICROPHONE_SECONDARY,   // Rear-facing PDM microphone (noise cancellation)
    MICROPHONE_ANALOG,      // Analog microphone via ES8388
    LINE_IN,                // Line input via ES8388
    BLUETOOTH,              // Bluetooth audio input
    USB_AUDIO               // USB audio input
};

enum class AudioOutputDevice {
    SPEAKERS,               // Internal speakers via NS4150
    HEADPHONES,             // Headphones via ES8388
    LINE_OUT,               // Line output via ES8388
    BLUETOOTH,              // Bluetooth audio output
    USB_AUDIO               // USB audio output
};

enum class NoiseReductionMode {
    DISABLED,
    LIGHT,                  // Light noise reduction
    MODERATE,               // Moderate noise reduction
    AGGRESSIVE,             // Aggressive noise reduction
    ADAPTIVE                // Adaptive noise reduction based on environment
};

enum class AudioProcessingMode {
    PASSTHROUGH,            // No processing
    VOICE_ENHANCEMENT,      // Voice call optimization
    MUSIC_ENHANCEMENT,      // Music playback optimization
    NOISE_CANCELLATION,     // Active noise cancellation
    VOICE_RECOGNITION,      // Voice recognition optimization
    CONFERENCE_CALL         // Conference call optimization
};

struct AudioConfig {
    AudioSampleRate sampleRate = AudioSampleRate::RATE_44_1KHZ;
    AudioBitDepth bitDepth = AudioBitDepth::BITS_16;
    AudioChannelMode channelMode = AudioChannelMode::STEREO;
    AudioInputSource inputSource = AudioInputSource::MICROPHONE_PRIMARY;
    AudioOutputDevice outputDevice = AudioOutputDevice::SPEAKERS;
    NoiseReductionMode noiseReduction = NoiseReductionMode::LIGHT;
    AudioProcessingMode processingMode = AudioProcessingMode::PASSTHROUGH;
    uint8_t inputGain = 50;       // 0-100%
    uint8_t outputVolume = 50;    // 0-100%
    bool enableAGC = true;        // Automatic Gain Control
    bool enableAEC = false;       // Acoustic Echo Cancellation
    bool enableVAD = false;       // Voice Activity Detection
};

struct AudioLevels {
    float inputLevel = 0.0f;      // dB
    float outputLevel = 0.0f;     // dB
    float noiseLevel = 0.0f;      // dB
    float signalToNoise = 0.0f;   // dB
    bool voiceDetected = false;
    bool clipping = false;
};

struct MicrophoneCalibration {
    float sensitivity = 1.0f;     // Microphone sensitivity correction
    float frequencyResponse[10];  // 10-band frequency response correction
    float noiseFloor = -60.0f;    // Noise floor in dB
    bool isCalibrated = false;
};

/**
 * @brief Audio Hardware Abstraction Layer
 * Manages ES8388 codec, dual microphones, and NS4150 amplifiers
 */
class AudioHAL {
public:
    AudioHAL() = default;
    ~AudioHAL();

    /**
     * @brief Initialize audio HAL
     * @return OS_OK on success, error code on failure
     */
    os_error_t initialize();

    /**
     * @brief Shutdown audio HAL
     * @return OS_OK on success, error code on failure
     */
    os_error_t shutdown();

    /**
     * @brief Update audio HAL (process audio, check levels)
     * @param deltaTime Time since last update in milliseconds
     * @return OS_OK on success, error code on failure
     */
    os_error_t update(uint32_t deltaTime);

    /**
     * @brief Configure audio parameters
     * @param config Audio configuration
     * @return OS_OK on success, error code on failure
     */
    os_error_t configure(const AudioConfig& config);

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
     * @brief Start audio playback
     * @return OS_OK on success, error code on failure
     */
    os_error_t startPlayback();

    /**
     * @brief Stop audio playback
     * @return OS_OK on success, error code on failure
     */
    os_error_t stopPlayback();

    /**
     * @brief Read audio data
     * @param buffer Buffer to store audio data
     * @param bufferSize Buffer size in bytes
     * @param bytesRead Number of bytes actually read
     * @param timeout Timeout in milliseconds
     * @return OS_OK on success, error code on failure
     */
    os_error_t readAudio(void* buffer, size_t bufferSize, size_t& bytesRead, uint32_t timeout = 1000);

    /**
     * @brief Write audio data
     * @param buffer Buffer containing audio data
     * @param bufferSize Buffer size in bytes
     * @param bytesWritten Number of bytes actually written
     * @param timeout Timeout in milliseconds
     * @return OS_OK on success, error code on failure
     */
    os_error_t writeAudio(const void* buffer, size_t bufferSize, size_t& bytesWritten, uint32_t timeout = 1000);

    /**
     * @brief Set input gain
     * @param gain Gain level (0-100%)
     * @return OS_OK on success, error code on failure
     */
    os_error_t setInputGain(uint8_t gain);

    /**
     * @brief Set output volume
     * @param volume Volume level (0-100%)
     * @return OS_OK on success, error code on failure
     */
    os_error_t setOutputVolume(uint8_t volume);

    /**
     * @brief Mute/unmute audio output
     * @param mute true to mute, false to unmute
     * @return OS_OK on success, error code on failure
     */
    os_error_t setMute(bool mute);

    /**
     * @brief Get current audio levels
     * @return Audio levels structure
     */
    AudioLevels getAudioLevels() const { return m_currentLevels; }

    /**
     * @brief Enable/disable noise cancellation
     * @param enable Enable noise cancellation
     * @param mode Noise reduction mode
     * @return OS_OK on success, error code on failure
     */
    os_error_t enableNoiseReduction(bool enable, NoiseReductionMode mode = NoiseReductionMode::MODERATE);

    /**
     * @brief Check if headphones are connected
     * @return true if headphones are detected
     */
    bool isHeadphonesConnected() const;

    /**
     * @brief Calibrate microphones
     * @return OS_OK on success, error code on failure
     */
    os_error_t calibrateMicrophones();

    /**
     * @brief Get microphone calibration data
     * @param micIndex Microphone index (0=primary, 1=secondary)
     * @return Calibration data
     */
    MicrophoneCalibration getMicrophoneCalibration(uint8_t micIndex) const;

    /**
     * @brief Enable/disable voice activity detection
     * @param enable Enable VAD
     * @return OS_OK on success, error code on failure
     */
    os_error_t enableVAD(bool enable);

    /**
     * @brief Get audio HAL statistics
     */
    void printAudioStats() const;

    /**
     * @brief Check if audio HAL is initialized
     * @return true if initialized
     */
    bool isInitialized() const { return m_initialized; }

    /**
     * @brief Register audio level callback
     * @param callback Callback function
     * @param userData User data for callback
     */
    void registerLevelCallback(void(*callback)(const AudioLevels&, void*), void* userData);

    /**
     * @brief Perform audio loopback test
     * @param duration Test duration in milliseconds
     * @return OS_OK on success, error code on failure
     */
    os_error_t audioLoopbackTest(uint32_t duration = 5000);

    /**
     * @brief Initialize text-to-speech synthesis
     * @return OS_OK on success, error code on failure
     */
    os_error_t initializeTTS();

    /**
     * @brief Synthesize text to speech audio
     * @param text Text to synthesize
     * @param audioBuffer Output audio buffer
     * @param bufferSize Buffer size in bytes
     * @param actualSize Actual bytes generated
     * @return OS_OK on success, error code on failure
     */
    os_error_t synthesizeText(const std::string& text, uint8_t* audioBuffer, 
                             size_t bufferSize, size_t& actualSize);

    /**
     * @brief Apply voice characteristics filter
     * @param audioBuffer Audio buffer to process
     * @param bufferSize Buffer size in bytes
     * @param pitch Voice pitch (Hz)
     * @param speed Speech speed (WPM)
     * @return OS_OK on success, error code on failure
     */
    os_error_t applyVoiceEffects(uint8_t* audioBuffer, size_t bufferSize, 
                                float pitch = 120.0f, float speed = 120.0f);

    /**
     * @brief Enable voice activity detection
     * @param enable Enable VAD
     * @param threshold Energy threshold for voice detection
     * @return OS_OK on success, error code on failure
     */
    os_error_t enableVoiceActivityDetection(bool enable, float threshold = 0.01f);

    /**
     * @brief Check if voice activity is detected
     * @return true if voice is detected in current audio stream
     */
    bool isVoiceActivityDetected() const { return m_voiceActivityDetected; }

private:
    /**
     * @brief Initialize ES8388 codec
     * @return OS_OK on success, error code on failure
     */
    os_error_t initializeES8388();

    /**
     * @brief Initialize PDM microphones
     * @return OS_OK on success, error code on failure
     */
    os_error_t initializePDMMicrophones();

    /**
     * @brief Initialize NS4150 amplifiers
     * @return OS_OK on success, error code on failure
     */
    os_error_t initializeNS4150();

    /**
     * @brief Configure I2S for audio
     * @return OS_OK on success, error code on failure
     */
    os_error_t configureI2S();

    /**
     * @brief Configure I2C for codec communication
     * @return OS_OK on success, error code on failure
     */
    os_error_t configureI2C();

    /**
     * @brief Write to ES8388 register
     * @param reg Register address
     * @param value Register value
     * @return OS_OK on success, error code on failure
     */
    os_error_t writeES8388Register(uint8_t reg, uint8_t value);

    /**
     * @brief Read from ES8388 register
     * @param reg Register address
     * @param value Pointer to store register value
     * @return OS_OK on success, error code on failure
     */
    os_error_t readES8388Register(uint8_t reg, uint8_t* value);

    /**
     * @brief Process dual microphone input for noise cancellation
     * @param primaryBuffer Primary microphone buffer
     * @param secondaryBuffer Secondary microphone buffer
     * @param outputBuffer Processed output buffer
     * @param bufferSize Buffer size in samples
     */
    void processDualMicrophoneInput(const int16_t* primaryBuffer, const int16_t* secondaryBuffer, 
                                  int16_t* outputBuffer, size_t bufferSize);

    /**
     * @brief Update audio level measurements
     */
    void updateAudioLevels();

    /**
     * @brief Detect headphone connection
     */
    void detectHeadphones();

    /**
     * @brief Audio processing task
     * @param parameter Task parameter
     */
    static void audioProcessingTask(void* parameter);

    /**
     * @brief TTS processing task
     * @param parameter Task parameter
     */
    static void ttsProcessingTask(void* parameter);

    /**
     * @brief Process voice activity detection
     * @param audioBuffer Audio buffer to analyze
     * @param bufferSize Buffer size in samples
     * @return Voice activity energy level
     */
    float processVoiceActivityDetection(const int16_t* audioBuffer, size_t bufferSize);

    /**
     * @brief Generate basic phoneme audio
     * @param phoneme Phoneme identifier
     * @param duration Duration in samples
     * @param pitch Base pitch frequency
     * @param outputBuffer Output audio buffer
     * @return Number of samples generated
     */
    size_t generatePhonemeAudio(const std::string& phoneme, size_t duration, 
                               float pitch, int16_t* outputBuffer);

    /**
     * @brief Apply formant synthesis
     * @param audioBuffer Audio buffer to process
     * @param bufferSize Buffer size in samples
     * @param formants Formant frequencies array
     * @param formantCount Number of formants
     */
    void applyFormantSynthesis(int16_t* audioBuffer, size_t bufferSize, 
                              const float* formants, size_t formantCount);

    /**
     * @brief Headphone detection ISR
     * @param arg ISR argument
     */
    static void IRAM_ATTR headphoneDetectISR(void* arg);

    // Audio configuration
    AudioConfig m_config;
    AudioLevels m_currentLevels;
    MicrophoneCalibration m_micCalibration[2]; // Primary and secondary microphones

    // Hardware state
    bool m_initialized = false;
    bool m_recording = false;
    bool m_playing = false;
    bool m_headphonesConnected = false;
    bool m_muted = false;

    // I2S configuration
    i2s_config_t m_i2sConfig = {};
    i2s_pin_config_t m_i2sPinConfig = {};

    // Audio buffers
    int16_t* m_primaryMicBuffer = nullptr;
    int16_t* m_secondaryMicBuffer = nullptr;
    int16_t* m_processedBuffer = nullptr;
    int16_t* m_outputBuffer = nullptr;
    int16_t* m_ttsBuffer = nullptr;           // TTS synthesis buffer
    uint8_t* m_phonemeBuffer = nullptr;       // Phoneme processing buffer
    size_t m_bufferSize = 0;
    size_t m_ttsBufferSize = 0;

    // Level callback
    void(*m_levelCallback)(const AudioLevels&, void*) = nullptr;
    void* m_callbackUserData = nullptr;

    // Processing task
    TaskHandle_t m_audioTaskHandle = nullptr;
    TaskHandle_t m_ttsTaskHandle = nullptr;
    bool m_taskRunning = false;
    bool m_ttsEnabled = false;
    bool m_voiceActivityDetected = false;
    float m_vadThreshold = 0.01f;

    // Statistics
    uint32_t m_samplesProcessed = 0;
    uint32_t m_bufferUnderruns = 0;
    uint32_t m_bufferOverruns = 0;
    uint32_t m_headphoneEvents = 0;
    uint32_t m_ttsRequestsProcessed = 0;
    uint32_t m_voiceActivations = 0;
    float m_averageVoiceEnergy = 0.0f;

    // Configuration constants
    static constexpr size_t AUDIO_BUFFER_SIZE = 1024;  // Samples per channel
    static constexpr uint32_t AUDIO_TASK_STACK_SIZE = 8192;
    static constexpr UBaseType_t AUDIO_TASK_PRIORITY = 10;
    static constexpr const char* TAG = "AudioHAL";

    // ES8388 register addresses
    static constexpr uint8_t ES8388_CONTROL1 = 0x00;
    static constexpr uint8_t ES8388_CONTROL2 = 0x01;
    static constexpr uint8_t ES8388_CHIPPOWER = 0x02;
    static constexpr uint8_t ES8388_ADCPOWER = 0x03;
    static constexpr uint8_t ES8388_DACPOWER = 0x04;
    static constexpr uint8_t ES8388_CHIPLOPOW1 = 0x05;
    static constexpr uint8_t ES8388_CHIPLOPOW2 = 0x06;
    static constexpr uint8_t ES8388_ANAVOLMANAG = 0x07;
    static constexpr uint8_t ES8388_MASTERMODE = 0x08;
    // ... Additional ES8388 registers as needed
};

#endif // AUDIO_HAL_H