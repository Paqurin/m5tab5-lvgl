#include "audio_service.h"
#include "../system/os_manager.h"
#include <esp_log.h>
#include <driver/gpio.h>
// Conditional I2C include
#ifdef CONFIG_I2C_ENABLED
#include <driver/i2c.h>
#endif

static const char* TAG = "AudioService";

AudioService::~AudioService() {
    shutdown();
}

os_error_t AudioService::initialize() {
    if (m_initialized) {
        return OS_OK;
    }

    ESP_LOGI(TAG, "Initializing Audio Service");

    // Initialize I2S interface
    os_error_t result = initializeI2S();
    if (result != OS_OK) {
        ESP_LOGE(TAG, "Failed to initialize I2S");
        return result;
    }

    // Initialize NS4150 speaker amplifier
    result = initializeNS4150();
    if (result != OS_OK) {
        ESP_LOGW(TAG, "Failed to initialize NS4150, continuing without speaker support");
    }

    // Initialize ES8388 audio codec
    result = initializeES8388();
    if (result != OS_OK) {
        ESP_LOGW(TAG, "Failed to initialize ES8388, continuing without headphone support");
    }

    // Set default configuration
    m_config.outputDevice = AudioOutputDevice::AUTO;
    m_config.outputVolume = 50;
    m_config.autoSwitching = true;

    // Initial device detection
    updateAudioRouting();

    m_lastHeadphoneCheck = millis();
    m_initialized = true;

    ESP_LOGI(TAG, "Audio Service initialized successfully");
    return OS_OK;
}

os_error_t AudioService::shutdown() {
    if (!m_initialized) {
        return OS_OK;
    }

    ESP_LOGI(TAG, "Shutting down Audio Service");

    // Stop any ongoing playback
    stop();

    // Disable audio devices
    configureNS4150(false);
    configureES8388(false);

    // Deinitialize I2S
    #ifdef CONFIG_ESP_AUDIO_SUPPORTED
    if (m_i2sInitialized) {
        i2s_driver_uninstall(m_i2sOutputPort);
        m_i2sInitialized = false;
    }
    #endif

    m_initialized = false;
    return OS_OK;
}

os_error_t AudioService::update(uint32_t deltaTime) {
    if (!m_initialized) {
        return OS_ERROR_GENERIC;
    }

    uint32_t now = millis();

    // Periodic headphone detection
    if (now - m_lastHeadphoneCheck >= HEADPHONE_CHECK_INTERVAL) {
        bool previouslyConnected = m_headphonesConnected;
        m_headphonesConnected = checkHeadphoneConnection();
        
        if (previouslyConnected != m_headphonesConnected) {
            ESP_LOGI(TAG, "Headphone connection changed: %s", 
                    m_headphonesConnected ? "connected" : "disconnected");
            
            if (m_config.autoSwitching) {
                updateAudioRouting();
            }
            
            // Mute on headphone unplug if configured
            if (!m_headphonesConnected && m_config.muteOnHeadphoneUnplug) {
                setMute(true);
            }
        }
        
        m_lastHeadphoneCheck = now;
    }

    return OS_OK;
}

os_error_t AudioService::configure(const AudioConfig& config) {
    ESP_LOGI(TAG, "Configuring audio service");

    m_config = config;

    // Apply volume setting
    setVolume(config.outputVolume);

    // Apply output device setting
    if (config.outputDevice != AudioOutputDevice::AUTO) {
        setOutputDevice(config.outputDevice);
    } else {
        updateAudioRouting();
    }

    return OS_OK;
}

os_error_t AudioService::setOutputDevice(AudioOutputDevice device) {
    if (!m_initialized) {
        return OS_ERROR_GENERIC;
    }

    ESP_LOGI(TAG, "Setting output device: %d", (int)device);

    // Disable current device
    if (m_currentOutputDevice == AudioOutputDevice::SPEAKERS_NS4150) {
        configureNS4150(false);
    } else if (m_currentOutputDevice == AudioOutputDevice::HEADPHONES_ES8388) {
        configureES8388(false);
    }

    // Enable new device
    os_error_t result = OS_OK;
    switch (device) {
        case AudioOutputDevice::SPEAKERS_NS4150:
            if (m_ns4150Initialized) {
                result = configureNS4150(true);
                if (result == OS_OK) {
                    m_currentOutputDevice = device;
                    m_deviceSwitches++;
                }
            } else {
                result = OS_ERROR_NOT_AVAILABLE;
            }
            break;

        case AudioOutputDevice::HEADPHONES_ES8388:
            if (m_es8388Initialized) {
                result = configureES8388(true);
                if (result == OS_OK) {
                    m_currentOutputDevice = device;
                    m_deviceSwitches++;
                }
            } else {
                result = OS_ERROR_NOT_AVAILABLE;
            }
            break;

        case AudioOutputDevice::AUTO:
            return updateAudioRouting();

        case AudioOutputDevice::NONE:
            m_currentOutputDevice = device;
            break;

        default:
            result = OS_ERROR_INVALID_PARAM;
            break;
    }

    return result;
}

os_error_t AudioService::setVolume(uint8_t volume) {
    if (volume > 100) {
        volume = 100;
    }

    m_config.outputVolume = volume;

    ESP_LOGI(TAG, "Setting volume to %d%%", volume);

    // Apply volume to current device
    if (m_currentOutputDevice == AudioOutputDevice::SPEAKERS_NS4150) {
        return setNS4150Volume(volume);
    } else if (m_currentOutputDevice == AudioOutputDevice::HEADPHONES_ES8388) {
        return setES8388Volume(volume);
    }

    return OS_OK;
}

os_error_t AudioService::setMute(bool mute) {
    ESP_LOGI(TAG, "%s audio", mute ? "Muting" : "Unmuting");

    m_muted = mute;

    if (mute) {
        // Mute current device
        if (m_currentOutputDevice == AudioOutputDevice::SPEAKERS_NS4150) {
            configureNS4150(false);
        } else if (m_currentOutputDevice == AudioOutputDevice::HEADPHONES_ES8388) {
            configureES8388(false);
        }
    } else {
        // Unmute current device
        if (m_currentOutputDevice == AudioOutputDevice::SPEAKERS_NS4150) {
            configureNS4150(true);
        } else if (m_currentOutputDevice == AudioOutputDevice::HEADPHONES_ES8388) {
            configureES8388(true);
        }
    }

    return OS_OK;
}

os_error_t AudioService::playBuffer(const uint8_t* data, size_t length) {
    #ifdef CONFIG_ESP_AUDIO_SUPPORTED
    if (!m_initialized || !m_i2sInitialized || !data) {
        return OS_ERROR_INVALID_PARAM;
    }

    if (m_muted || m_currentOutputDevice == AudioOutputDevice::NONE) {
        return OS_OK; // Silently ignore playback when muted or no device
    }

    size_t bytesWritten;
    esp_err_t ret = i2s_write(m_i2sOutputPort, data, length, &bytesWritten, portMAX_DELAY);
    
    if (ret == ESP_OK) {
        m_bytesPlayed += bytesWritten;
        m_state = AudioState::PLAYING;
        return OS_OK;
    } else {
        ESP_LOGE(TAG, "Failed to write I2S data: %s", esp_err_to_name(ret));
        m_state = AudioState::ERROR;
        return OS_ERROR_GENERIC;
    }
    #else
    ESP_LOGW(TAG, "Audio playback not supported in this build");
    return OS_ERROR_NOT_SUPPORTED;
    #endif
}

os_error_t AudioService::stop() {
    #ifdef CONFIG_ESP_AUDIO_SUPPORTED
    if (m_i2sInitialized) {
        i2s_stop(m_i2sOutputPort);
        m_state = AudioState::STOPPED;
    }
    #endif
    return OS_OK;
}

os_error_t AudioService::pause() {
    #ifdef CONFIG_ESP_AUDIO_SUPPORTED
    if (m_i2sInitialized) {
        i2s_stop(m_i2sOutputPort);
        m_state = AudioState::PAUSED;
    }
    #endif
    return OS_OK;
}

os_error_t AudioService::resume() {
    #ifdef CONFIG_ESP_AUDIO_SUPPORTED
    if (m_i2sInitialized) {
        i2s_start(m_i2sOutputPort);
        m_state = AudioState::PLAYING;
    }
    #endif
    return OS_OK;
}

void AudioService::printAudioStats() const {
    ESP_LOGI(TAG, "Audio Statistics:");
    ESP_LOGI(TAG, "  Current Device: %d", (int)m_currentOutputDevice);
    ESP_LOGI(TAG, "  Volume: %d%%", m_config.outputVolume);
    ESP_LOGI(TAG, "  Muted: %s", m_muted ? "YES" : "NO");
    ESP_LOGI(TAG, "  Headphones Connected: %s", m_headphonesConnected ? "YES" : "NO");
    ESP_LOGI(TAG, "  Audio State: %d", (int)m_state);
    ESP_LOGI(TAG, "  Bytes Played: %u", m_bytesPlayed);
    ESP_LOGI(TAG, "  Device Switches: %u", m_deviceSwitches);
    ESP_LOGI(TAG, "  NS4150 Initialized: %s", m_ns4150Initialized ? "YES" : "NO");
    ESP_LOGI(TAG, "  ES8388 Initialized: %s", m_es8388Initialized ? "YES" : "NO");
}

os_error_t AudioService::initializeI2S() {
    #ifdef CONFIG_ESP_AUDIO_SUPPORTED
    ESP_LOGI(TAG, "Initializing I2S interface");

    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
        .sample_rate = m_config.sampleRate,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
        .communication_format = I2S_COMM_FORMAT_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = I2S_DMA_BUF_COUNT,
        .dma_buf_len = I2S_DMA_BUF_LEN,
        .use_apll = false,
        .tx_desc_auto_clear = true,
        .fixed_mclk = 0
    };

    esp_err_t ret = i2s_driver_install(m_i2sOutputPort, &i2s_config, 0, NULL);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to install I2S driver: %s", esp_err_to_name(ret));
        return OS_ERROR_HARDWARE;
    }

    i2s_pin_config_t pin_config = {
        .bck_io_num = I2S_BCK_PIN,
        .ws_io_num = I2S_WS_PIN,
        .data_out_num = I2S_DO_PIN,
        .data_in_num = I2S_DI_PIN
    };

    ret = i2s_set_pin(m_i2sOutputPort, &pin_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set I2S pins: %s", esp_err_to_name(ret));
        i2s_driver_uninstall(m_i2sOutputPort);
        return OS_ERROR_HARDWARE;
    }

    m_i2sInitialized = true;
    ESP_LOGI(TAG, "I2S interface initialized successfully");
    return OS_OK;
    #else
    ESP_LOGW(TAG, "I2S not supported in this build");
    return OS_ERROR_NOT_SUPPORTED;
    #endif
}

os_error_t AudioService::initializeNS4150() {
    ESP_LOGI(TAG, "Initializing NS4150 speaker amplifier");

    // Configure NS4150 control pins
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = (1ULL << NS4150_EN_PIN);
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;

    esp_err_t ret = gpio_config(&io_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure NS4150 pins: %s", esp_err_to_name(ret));
        return OS_ERROR_HARDWARE;
    }

    // Initialize to disabled state
    gpio_set_level(NS4150_EN_PIN, 0);

    m_ns4150Initialized = true;
    ESP_LOGI(TAG, "NS4150 speaker amplifier initialized");
    return OS_OK;
}

os_error_t AudioService::initializeES8388() {
    ESP_LOGI(TAG, "Initializing ES8388 audio codec");

    #ifdef CONFIG_I2C_ENABLED
    // Initialize I2C for ES8388 communication
    i2c_config_t i2c_config = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = ES8388_I2C_SDA_PIN,
        .scl_io_num = ES8388_I2C_SCL_PIN,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master = {
            .clk_speed = 100000, // 100kHz
        }
    };

    esp_err_t ret = i2c_param_config(I2C_NUM_1, &i2c_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure I2C for ES8388: %s", esp_err_to_name(ret));
        return OS_ERROR_HARDWARE;
    }

    ret = i2c_driver_install(I2C_NUM_1, I2C_MODE_MASTER, 0, 0, 0);
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "Failed to install I2C driver: %s", esp_err_to_name(ret));
        return OS_ERROR_HARDWARE;
    }
    #else
    ESP_LOGW(TAG, "I2C not supported in this build - ES8388 initialization skipped");
    #endif

    // Configure ES8388 power pin
    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = (1ULL << ES8388_PWR_PIN);
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    
    esp_err_t ret = gpio_config(&io_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure ES8388 power pin: %s", esp_err_to_name(ret));
        return OS_ERROR_HARDWARE;
    }

    // Power on ES8388
    gpio_set_level(ES8388_PWR_PIN, 1);
    vTaskDelay(pdMS_TO_TICKS(10)); // Power-on delay

    // Initialize ES8388 registers
    os_error_t result = configureES8388(false); // Start disabled
    if (result != OS_OK) {
        ESP_LOGE(TAG, "Failed to configure ES8388 registers");
        return result;
    }

    m_es8388Initialized = true;
    ESP_LOGI(TAG, "ES8388 audio codec initialized");
    return OS_OK;
}

os_error_t AudioService::configureNS4150(bool enabled) {
    if (!m_ns4150Initialized) {
        return OS_ERROR_GENERIC;
    }

    ESP_LOGI(TAG, "%s NS4150 speakers", enabled ? "Enabling" : "Disabling");

    gpio_set_level(NS4150_EN_PIN, enabled ? 1 : 0);
    
    if (enabled) {
        // Small delay for amplifier startup
        vTaskDelay(pdMS_TO_TICKS(10));
        
        // Apply current volume setting
        setNS4150Volume(m_config.outputVolume);
    }

    return OS_OK;
}

os_error_t AudioService::configureES8388(bool enabled) {
    if (!m_es8388Initialized) {
        return OS_ERROR_GENERIC;
    }

    ESP_LOGI(TAG, "%s ES8388 headphones", enabled ? "Enabling" : "Disabling");

    if (enabled) {
        // Power up ES8388
        writeES8388Register(ES8388_REG_CHIPPOWER, 0x00);  // Power up
        writeES8388Register(ES8388_REG_DACPOWER, 0x3C);   // Power up DAC
        writeES8388Register(ES8388_REG_CONTROL1, 0x12);   // Enable DAC
        writeES8388Register(ES8388_REG_CONTROL2, 0x50);   // Enable headphone output
        
        // Configure audio format
        writeES8388Register(ES8388_REG_MASTERMODE, 0x00); // Slave mode
        writeES8388Register(ES8388_REG_DACCONTROL1, 0x18); // I2S format
        writeES8388Register(ES8388_REG_DACCONTROL2, 0x02); // 16-bit samples
        
        // Apply volume
        setES8388Volume(m_config.outputVolume);
        
        vTaskDelay(pdMS_TO_TICKS(10)); // Startup delay
    } else {
        // Power down ES8388
        writeES8388Register(ES8388_REG_DACPOWER, 0xFF);   // Power down DAC
        writeES8388Register(ES8388_REG_CHIPPOWER, 0xFF);  // Power down chip
    }

    return OS_OK;
}

os_error_t AudioService::writeES8388Register(uint8_t reg, uint8_t data) {
    #ifdef CONFIG_I2C_ENABLED
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (AUDIO_ES8388_I2C_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg, true);
    i2c_master_write_byte(cmd, data, true);
    i2c_master_stop(cmd);
    
    esp_err_t ret = i2c_master_cmd_begin(I2C_NUM_1, cmd, pdMS_TO_TICKS(100));
    i2c_cmd_link_delete(cmd);
    
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to write ES8388 register 0x%02X: %s", reg, esp_err_to_name(ret));
        return OS_ERROR_GENERIC;
    }
    
    return OS_OK;
    #else
    ESP_LOGW(TAG, "I2C not supported - cannot write ES8388 register 0x%02X", reg);
    return OS_ERROR_NOT_AVAILABLE;
    #endif
}

os_error_t AudioService::readES8388Register(uint8_t reg, uint8_t* data) {
    if (!data) {
        return OS_ERROR_INVALID_PARAM;
    }

    #ifdef CONFIG_I2C_ENABLED
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (AUDIO_ES8388_I2C_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg, true);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (AUDIO_ES8388_I2C_ADDR << 1) | I2C_MASTER_READ, true);
    i2c_master_read_byte(cmd, data, I2C_MASTER_NACK);
    i2c_master_stop(cmd);
    
    esp_err_t ret = i2c_master_cmd_begin(I2C_NUM_1, cmd, pdMS_TO_TICKS(100));
    i2c_cmd_link_delete(cmd);
    
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read ES8388 register 0x%02X: %s", reg, esp_err_to_name(ret));
        return OS_ERROR_GENERIC;
    }
    
    return OS_OK;
    #else
    ESP_LOGW(TAG, "I2C not supported - cannot read ES8388 register 0x%02X", reg);
    *data = 0;
    return OS_ERROR_NOT_AVAILABLE;
    #endif
}

bool AudioService::checkHeadphoneConnection() {
    #ifdef ES8388_DETECT_PIN
    // Check headphone detect pin (usually active low)
    return (gpio_get_level(ES8388_DETECT_PIN) == 0);
    #else
    // Without detect pin, try to read from ES8388 to detect presence
    if (m_es8388Initialized) {
        uint8_t data;
        os_error_t result = readES8388Register(ES8388_REG_CHIPPOWER, &data);
        return (result == OS_OK); // If we can read, codec is present
    }
    return false;
    #endif
}

os_error_t AudioService::updateAudioRouting() {
    if (!m_config.autoSwitching) {
        return OS_OK;
    }

    AudioOutputDevice newDevice = AudioOutputDevice::NONE;

    // Priority: Headphones > Speakers
    if (m_headphonesConnected && m_es8388Initialized) {
        newDevice = AudioOutputDevice::HEADPHONES_ES8388;
    } else if (m_ns4150Initialized) {
        newDevice = AudioOutputDevice::SPEAKERS_NS4150;
    }

    if (newDevice != m_currentOutputDevice) {
        ESP_LOGI(TAG, "Auto-switching audio output from %d to %d", 
                (int)m_currentOutputDevice, (int)newDevice);
        return setOutputDevice(newDevice);
    }

    return OS_OK;
}

os_error_t AudioService::setNS4150Volume(uint8_t volume) {
    // NS4150 volume is controlled via GPIO enable/disable
    // For analog volume control, additional hardware would be needed
    ESP_LOGD(TAG, "NS4150 volume set to %d%% (digital control)", volume);
    return OS_OK;
}

os_error_t AudioService::setES8388Volume(uint8_t volume) {
    if (!m_es8388Initialized) {
        return OS_ERROR_GENERIC;
    }

    // Convert 0-100 volume to ES8388 register value (0-33, where 0 is max)
    uint8_t regValue = 33 - (volume * 33 / 100);
    
    // Set left and right headphone volume
    writeES8388Register(ES8388_REG_LOUT1VOL, regValue);
    writeES8388Register(ES8388_REG_ROUT1VOL, regValue);
    
    ESP_LOGD(TAG, "ES8388 volume set to %d%% (reg: %d)", volume, regValue);
    return OS_OK;
}