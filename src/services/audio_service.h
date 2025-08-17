#ifndef AUDIO_SERVICE_H
#define AUDIO_SERVICE_H

#include "../system/os_config.h"
#include "../hal/hardware_config.h"

// Conditional compilation for audio support
#ifdef CONFIG_ESP_AUDIO_SUPPORTED
#include <driver/i2s.h>
#include <driver/i2c.h>
#else
// Stub definitions for I2S types when not available
typedef enum {
    I2S_MODE_MASTER = 1,
    I2S_MODE_SLAVE = 2,
    I2S_MODE_TX = 4,
    I2S_MODE_RX = 8
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
    I2S_COMM_FORMAT_I2S_MSB = 0x02,
    I2S_COMM_FORMAT_I2S_LSB = 0x04
} i2s_comm_format_t;

typedef struct {
    i2s_mode_t mode;
    uint32_t sample_rate;
    i2s_bits_per_sample_t bits_per_sample;
    i2s_channel_fmt_t channel_format;
    i2s_comm_format_t communication_format;
    int intr_alloc_flags;
    int dma_buf_count;
    int dma_buf_len;
    bool use_apll;
    bool tx_desc_auto_clear;
    int fixed_mclk;
} i2s_config_t;

typedef struct {
    int bck_io_num;
    int ws_io_num;
    int data_out_num;
    int data_in_num;
} i2s_pin_config_t;

typedef enum {
    I2S_NUM_0 = 0,
    I2S_NUM_1 = 1
} i2s_port_t;
#endif

/**
 * @file audio_service.h
 * @brief Audio Service for M5Stack Tab5
 * 
 * Provides audio management with automatic switching between
 * NS4150 speaker amplifiers and ES8388 headphone codec,
 * including volume control and audio routing.
 */

enum class AudioOutputDevice {
    NONE,
    SPEAKERS_NS4150,
    HEADPHONES_ES8388,
    AUTO
};

enum class AudioFormat {
    PCM_16BIT_44KHZ,
    PCM_16BIT_48KHZ,
    PCM_24BIT_44KHZ,
    PCM_24BIT_48KHZ
};

enum class AudioState {
    STOPPED,
    PLAYING,
    PAUSED,
    ERROR
};

struct AudioConfig {
    AudioOutputDevice outputDevice = AudioOutputDevice::AUTO;
    AudioFormat format = AudioFormat::PCM_16BIT_44KHZ;
    uint8_t volume = 50; // 0-100
    bool muteOnHeadphoneUnplug = true;
    bool autoSwitching = true;
    uint32_t sampleRate = 44100;
    uint8_t channels = 2;
};

class AudioService {
public:
    AudioService() = default;
    ~AudioService();

    /**
     * @brief Initialize audio service
     * @return OS_OK on success, error code on failure
     */
    os_error_t initialize();

    /**
     * @brief Shutdown audio service
     * @return OS_OK on success, error code on failure
     */
    os_error_t shutdown();

    /**
     * @brief Update audio service (check for device changes)
     * @param deltaTime Time since last update in milliseconds
     * @return OS_OK on success, error code on failure
     */
    os_error_t update(uint32_t deltaTime);

    /**
     * @brief Configure audio settings
     * @param config Audio configuration
     * @return OS_OK on success, error code on failure
     */
    os_error_t configure(const AudioConfig& config);

    /**
     * @brief Set output device
     * @param device Audio output device
     * @return OS_OK on success, error code on failure
     */
    os_error_t setOutputDevice(AudioOutputDevice device);

    /**
     * @brief Set volume level
     * @param volume Volume level (0-100)
     * @return OS_OK on success, error code on failure
     */
    os_error_t setVolume(uint8_t volume);

    /**
     * @brief Get current volume level
     * @return Volume level (0-100)
     */
    uint8_t getVolume() const { return m_config.volume; }

    /**
     * @brief Mute/unmute audio
     * @param mute true to mute, false to unmute
     * @return OS_OK on success, error code on failure
     */
    os_error_t setMute(bool mute);

    /**
     * @brief Check if audio is muted
     * @return true if muted
     */
    bool isMuted() const { return m_muted; }

    /**
     * @brief Check if headphones are connected
     * @return true if headphones are detected
     */
    bool isHeadphonesConnected() const { return m_headphonesConnected; }

    /**
     * @brief Get current output device
     * @return Current audio output device
     */
    AudioOutputDevice getCurrentOutputDevice() const { return m_currentOutputDevice; }

    /**
     * @brief Get audio state
     * @return Current audio state
     */
    AudioState getState() const { return m_state; }

    /**
     * @brief Play audio buffer
     * @param data Audio data buffer
     * @param length Buffer length in bytes
     * @return OS_OK on success, error code on failure
     */
    os_error_t playBuffer(const uint8_t* data, size_t length);

    /**
     * @brief Stop audio playback
     * @return OS_OK on success, error code on failure
     */
    os_error_t stop();

    /**
     * @brief Pause audio playback
     * @return OS_OK on success, error code on failure
     */
    os_error_t pause();

    /**
     * @brief Resume audio playback
     * @return OS_OK on success, error code on failure
     */
    os_error_t resume();

    /**
     * @brief Get audio statistics
     */
    void printAudioStats() const;

private:
    /**
     * @brief Initialize I2S audio interface
     * @return OS_OK on success, error code on failure
     */
    os_error_t initializeI2S();

    /**
     * @brief Initialize NS4150 speaker amplifier
     * @return OS_OK on success, error code on failure
     */
    os_error_t initializeNS4150();

    /**
     * @brief Initialize ES8388 audio codec
     * @return OS_OK on success, error code on failure
     */
    os_error_t initializeES8388();

    /**
     * @brief Configure NS4150 amplifier
     * @param enabled true to enable speakers
     * @return OS_OK on success, error code on failure
     */
    os_error_t configureNS4150(bool enabled);

    /**
     * @brief Configure ES8388 codec
     * @param enabled true to enable headphones
     * @return OS_OK on success, error code on failure
     */
    os_error_t configureES8388(bool enabled);

    /**
     * @brief Write to ES8388 register
     * @param reg Register address
     * @param data Data to write
     * @return OS_OK on success, error code on failure
     */
    os_error_t writeES8388Register(uint8_t reg, uint8_t data);

    /**
     * @brief Read from ES8388 register
     * @param reg Register address
     * @param data Output data
     * @return OS_OK on success, error code on failure
     */
    os_error_t readES8388Register(uint8_t reg, uint8_t* data);

    /**
     * @brief Check headphone connection status
     * @return true if headphones are connected
     */
    bool checkHeadphoneConnection();

    /**
     * @brief Update audio routing based on device detection
     * @return OS_OK on success, error code on failure
     */
    os_error_t updateAudioRouting();

    /**
     * @brief Set NS4150 volume
     * @param volume Volume level (0-100)
     * @return OS_OK on success, error code on failure
     */
    os_error_t setNS4150Volume(uint8_t volume);

    /**
     * @brief Set ES8388 volume
     * @param volume Volume level (0-100)
     * @return OS_OK on success, error code on failure
     */
    os_error_t setES8388Volume(uint8_t volume);

    // Audio configuration
    AudioConfig m_config;
    AudioOutputDevice m_currentOutputDevice = AudioOutputDevice::NONE;
    AudioState m_state = AudioState::STOPPED;

    // Device states
    bool m_initialized = false;
    bool m_i2sInitialized = false;
    bool m_ns4150Initialized = false;
    bool m_es8388Initialized = false;
    bool m_headphonesConnected = false;
    bool m_muted = false;

    // Hardware handles
    i2s_port_t m_i2sPort = I2S_NUM_0;

    // Statistics
    uint32_t m_bytesPlayed = 0;
    uint32_t m_deviceSwitches = 0;
    uint32_t m_lastHeadphoneCheck = 0;

    // Configuration
    static constexpr uint32_t HEADPHONE_CHECK_INTERVAL = 500; // 500ms
    static constexpr size_t I2S_DMA_BUF_COUNT = 8;
    static constexpr size_t I2S_DMA_BUF_LEN = 1024;
    static constexpr uint8_t AUDIO_ES8388_I2C_ADDR = 0x10;

    // ES8388 Registers
    static constexpr uint8_t ES8388_REG_CONTROL1 = 0x00;
    static constexpr uint8_t ES8388_REG_CONTROL2 = 0x01;
    static constexpr uint8_t ES8388_REG_CHIPPOWER = 0x02;
    static constexpr uint8_t ES8388_REG_ADCPOWER = 0x03;
    static constexpr uint8_t ES8388_REG_DACPOWER = 0x04;
    static constexpr uint8_t ES8388_REG_CHIPLOPOW1 = 0x05;
    static constexpr uint8_t ES8388_REG_CHIPLOPOW2 = 0x06;
    static constexpr uint8_t ES8388_REG_ANAVOLMANAG = 0x07;
    static constexpr uint8_t ES8388_REG_MASTERMODE = 0x08;
    static constexpr uint8_t ES8388_REG_ADCCONTROL1 = 0x09;
    static constexpr uint8_t ES8388_REG_ADCCONTROL2 = 0x0A;
    static constexpr uint8_t ES8388_REG_ADCCONTROL3 = 0x0B;
    static constexpr uint8_t ES8388_REG_ADCCONTROL4 = 0x0C;
    static constexpr uint8_t ES8388_REG_ADCCONTROL5 = 0x0D;
    static constexpr uint8_t ES8388_REG_ADCCONTROL6 = 0x0E;
    static constexpr uint8_t ES8388_REG_ADCCONTROL7 = 0x0F;
    static constexpr uint8_t ES8388_REG_ADCCONTROL8 = 0x10;
    static constexpr uint8_t ES8388_REG_ADCCONTROL9 = 0x11;
    static constexpr uint8_t ES8388_REG_ADCCONTROL10 = 0x12;
    static constexpr uint8_t ES8388_REG_ADCCONTROL11 = 0x13;
    static constexpr uint8_t ES8388_REG_ADCCONTROL12 = 0x14;
    static constexpr uint8_t ES8388_REG_ADCCONTROL13 = 0x15;
    static constexpr uint8_t ES8388_REG_ADCCONTROL14 = 0x16;
    static constexpr uint8_t ES8388_REG_DACCONTROL1 = 0x17;
    static constexpr uint8_t ES8388_REG_DACCONTROL2 = 0x18;
    static constexpr uint8_t ES8388_REG_DACCONTROL3 = 0x19;
    static constexpr uint8_t ES8388_REG_DACCONTROL4 = 0x1A;
    static constexpr uint8_t ES8388_REG_DACCONTROL5 = 0x1B;
    static constexpr uint8_t ES8388_REG_DACCONTROL6 = 0x1C;
    static constexpr uint8_t ES8388_REG_DACCONTROL7 = 0x1D;
    static constexpr uint8_t ES8388_REG_DACCONTROL8 = 0x1E;
    static constexpr uint8_t ES8388_REG_DACCONTROL9 = 0x1F;
    static constexpr uint8_t ES8388_REG_DACCONTROL10 = 0x20;
    static constexpr uint8_t ES8388_REG_DACCONTROL11 = 0x21;
    static constexpr uint8_t ES8388_REG_DACCONTROL12 = 0x22;
    static constexpr uint8_t ES8388_REG_DACCONTROL13 = 0x23;
    static constexpr uint8_t ES8388_REG_DACCONTROL14 = 0x24;
    static constexpr uint8_t ES8388_REG_DACCONTROL15 = 0x25;
    static constexpr uint8_t ES8388_REG_DACCONTROL16 = 0x26;
    static constexpr uint8_t ES8388_REG_DACCONTROL17 = 0x27;
    static constexpr uint8_t ES8388_REG_DACCONTROL18 = 0x28;
    static constexpr uint8_t ES8388_REG_DACCONTROL19 = 0x29;
    static constexpr uint8_t ES8388_REG_DACCONTROL20 = 0x2A;
    static constexpr uint8_t ES8388_REG_DACCONTROL21 = 0x2B;
    static constexpr uint8_t ES8388_REG_DACCONTROL22 = 0x2C;
    static constexpr uint8_t ES8388_REG_DACCONTROL23 = 0x2D;
    static constexpr uint8_t ES8388_REG_LOUT1VOL = 0x2E;
    static constexpr uint8_t ES8388_REG_LOUT2VOL = 0x2F;
    static constexpr uint8_t ES8388_REG_ROUT1VOL = 0x30;
    static constexpr uint8_t ES8388_REG_ROUT2VOL = 0x31;
};

#endif // AUDIO_SERVICE_H