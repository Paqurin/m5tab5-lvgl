#include "rtl_sdr_app.h"
#include "../system/os_config.h"
#include "../ui/theme_manager.h"
#include "../dsp/signal_processing.h"
#include <esp_log.h>
#include <algorithm>
#include <string>
#include <esp_heap_caps.h>

/**
 * @file rtl_sdr_app.cpp
 * @brief RTL-SDR Application Implementation
 */

// Predefined frequency bands
static const FrequencyRange FREQUENCY_BANDS[] = {
    {88000000, 108000000, "FM Radio", "FM Broadcast Band"},
    {144000000, 148000000, "2m Ham", "2 Meter Amateur Band"},
    {430000000, 440000000, "70cm Ham", "70 Centimeter Amateur Band"},
    {118000000, 137000000, "Aviation", "Aviation Communication"},
    {156000000, 162000000, "Marine", "Marine VHF"},
    {450000000, 470000000, "UHF", "UHF Business/Public Service"},
    {800000000, 900000000, "Cellular", "Cellular/Trunked Radio"},
    {1090000000, 1090000000, "ADS-B", "Aircraft Transponders"},
    {1575000000, 1575000000, "GPS L1", "GPS L1 Frequency"},
    {2400000000, 2500000000, "ISM", "2.4 GHz ISM Band"}
};

RTLSDRApp::RTLSDRApp() : BaseApp("rtl_sdr", "RTL-SDR", "1.0.0") {
    // Initialize frequency bands
    m_frequencyBands.assign(FREQUENCY_BANDS, FREQUENCY_BANDS + 
                           sizeof(FREQUENCY_BANDS) / sizeof(FREQUENCY_BANDS[0]));
    
    // Initialize configuration with defaults
    m_audioConfig.type = AudioDemodConfig::FM_WIDE;
    m_audioConfig.sampleRate = 1024000;
    m_audioConfig.bandwidth = 200000;
    m_audioConfig.volume = 0.7f;
    m_audioConfig.squelchEnabled = true;
    m_audioConfig.squelchLevel = -80.0f;
    
    m_spectrumConfig.fftSize = 1024;
    m_spectrumConfig.updateRate = 30;
    m_spectrumConfig.dynamicRange = 80.0f;
    m_spectrumConfig.referenceLevel = 0.0f;
    m_spectrumConfig.averagingEnabled = true;
    m_spectrumConfig.averagingFactor = 0.8f;
    m_spectrumConfig.peakHold = false;
    
    m_waterfallConfig.historyLines = 256;
    m_waterfallConfig.updateRate = 15;
    m_waterfallConfig.intensityScale = 1.0f;
    m_waterfallConfig.autoScale = true;
    
    // Initialize waterfall color map (rainbow)
    for (int i = 0; i < 256; i++) {
        float normalized = i / 255.0f;
        if (normalized < 0.25f) {
            // Blue to Cyan
            float t = normalized * 4.0f;
            m_waterfallConfig.colorMap[i] = lv_color_make(0, (uint8_t)(t * 255), 255);
        } else if (normalized < 0.5f) {
            // Cyan to Green
            float t = (normalized - 0.25f) * 4.0f;
            m_waterfallConfig.colorMap[i] = lv_color_make(0, 255, (uint8_t)((1.0f - t) * 255));
        } else if (normalized < 0.75f) {
            // Green to Yellow
            float t = (normalized - 0.5f) * 4.0f;
            m_waterfallConfig.colorMap[i] = lv_color_make((uint8_t)(t * 255), 255, 0);
        } else {
            // Yellow to Red
            float t = (normalized - 0.75f) * 4.0f;
            m_waterfallConfig.colorMap[i] = lv_color_make(255, (uint8_t)((1.0f - t) * 255), 0);
        }
    }
}

RTLSDRApp::~RTLSDRApp() {
    shutdown();
}

os_error_t RTLSDRApp::initialize() {
    ESP_LOGI(TAG, "Initializing RTL-SDR Application");
    
    // Initialize RTL-SDR service
    m_sdrService = new RTLSDRService();
    if (!m_sdrService) {
        ESP_LOGE(TAG, "Failed to create RTL-SDR service");
        return OS_ERROR_NO_MEMORY;
    }
    
    os_error_t result = m_sdrService->initialize();
    if (result != OS_OK) {
        ESP_LOGE(TAG, "Failed to initialize RTL-SDR service: %d", result);
        return result;
    }
    
    // Load configuration
    loadConfiguration();
    
    // Initialize spectrum data buffers
    m_spectrumData.resize(m_spectrumConfig.fftSize / 2);
    m_waterfallHistory.resize(m_waterfallConfig.historyLines * (m_spectrumConfig.fftSize / 2));
    m_iqBuffer.reserve(m_spectrumConfig.fftSize * 4); // 4x oversampling
    
    return OS_OK;
}

os_error_t RTLSDRApp::createUI(lv_obj_t* parent) {
    ESP_LOGI(TAG, "Creating RTL-SDR Application UI");
    
    // Detect RTL-SDR device
    os_error_t result = m_sdrService->detectDevice();
    if (result != OS_OK) {
        ESP_LOGW(TAG, "RTL-SDR device not detected, will retry when device is connected");
    }
    
    // Create main UI
    createMainUI();
    
    // Register for data callbacks
    m_sdrService->registerDataCallback(
        [](const IQSampleBuffer& buffer, void* userData) {
            RTLSDRApp* app = static_cast<RTLSDRApp*>(userData);
            app->processSpectrumData(buffer.samples);
            if (app->m_isDemodulating) {
                app->processAudioData(buffer.samples);
            }
        }, this);
    
    return OS_OK;
}

os_error_t RTLSDRApp::destroyUI() {
    ESP_LOGI(TAG, "Destroying RTL-SDR Application UI");
    
    // Stop any active operations
    if (m_isScanning) {
        stopFrequencyScan();
    }
    
    if (m_isRecording) {
        stopRecording();
    }
    
    if (m_sdrService && m_sdrService->isStreaming()) {
        m_sdrService->stopStreaming();
    }
    
    // Clear UI components
    if (m_mainContainer) {
        lv_obj_del(m_mainContainer);
        m_mainContainer = nullptr;
    }
    
    return OS_OK;
}

os_error_t RTLSDRApp::shutdown() {
    ESP_LOGI(TAG, "Shutting down RTL-SDR Application");
    
    destroyUI();
    
    // Save configuration
    saveConfiguration();
    
    // Cleanup RTL-SDR service
    if (m_sdrService) {
        m_sdrService->shutdown();
        delete m_sdrService;
        m_sdrService = nullptr;
    }
    
    return OS_OK;
}

os_error_t RTLSDRApp::update(uint32_t deltaTime) {
    // Update spectrum display at configured rate
    uint32_t currentTime = lv_tick_get();
    
    if (currentTime - m_lastSpectrumUpdate >= SPECTRUM_UPDATE_INTERVAL) {
        if (m_sdrService && m_sdrService->isStreaming()) {
            // Spectrum display is updated via data callback
        }
        m_lastSpectrumUpdate = currentTime;
    }
    
    // Update waterfall display at configured rate
    if (currentTime - m_lastWaterfallUpdate >= WATERFALL_UPDATE_INTERVAL) {
        // Waterfall display is updated via data callback
        m_lastWaterfallUpdate = currentTime;
    }
    
    // Update status bar
    updateStatusBar();
    
    return OS_OK;
}

os_error_t RTLSDRApp::handleEvent(uint32_t eventType, void* eventData, size_t dataSize) {
    switch (eventType) {
        case 1: // USB_DEVICE_CONNECTED
            {
                // Check if it's an RTL-SDR device
                os_error_t result = m_sdrService->detectDevice();
                if (result == OS_OK) {
                    ESP_LOGI(TAG, "RTL-SDR device connected");
                    updateStatusBar();
                }
            }
            break;
            
        case 2: // USB_DEVICE_DISCONNECTED
            if (m_sdrService && m_sdrService->isDeviceConnected()) {
                ESP_LOGI(TAG, "RTL-SDR device disconnected");
                m_sdrService->stopStreaming();
                updateStatusBar();
            }
            break;
            
        case 3: // MEMORY_LOW
            ESP_LOGW(TAG, "Low memory warning - reducing buffer sizes");
            if (m_iqBuffer.capacity() > m_spectrumConfig.fftSize) {
                m_iqBuffer.shrink_to_fit();
            }
            break;
            
        default:
            break;
    }
    
    return OS_OK;
}

void RTLSDRApp::createMainUI() {
    // Get current screen
    lv_obj_t* screen = lv_scr_act();
    
    // Create main container
    m_mainContainer = lv_obj_create(screen);
    lv_obj_set_size(m_mainContainer, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_pad_all(m_mainContainer, 0, 0);
    
    // Create spectrum display (top half)
    createSpectrumDisplay();
    
    // Create waterfall display (below spectrum)
    createWaterfallDisplay();
    
    // Create control panel (bottom)
    createControlPanel();
    
    ESP_LOGI(TAG, "RTL-SDR UI created successfully");
}

void RTLSDRApp::createSpectrumDisplay() {
    // Create spectrum chart container
    lv_obj_t* spectrumContainer = lv_obj_create(m_mainContainer);
    lv_obj_set_size(spectrumContainer, LV_PCT(100), LV_PCT(40));
    lv_obj_align(spectrumContainer, LV_ALIGN_TOP_MID, 0, 0);
    
    // Create spectrum chart
    m_spectrumChart = lv_chart_create(spectrumContainer);
    lv_obj_set_size(m_spectrumChart, LV_PCT(95), LV_PCT(90));
    lv_obj_center(m_spectrumChart);
    
    lv_chart_set_type(m_spectrumChart, LV_CHART_TYPE_LINE);
    lv_chart_set_point_count(m_spectrumChart, m_spectrumConfig.fftSize / 2);
    lv_chart_set_range(m_spectrumChart, LV_CHART_AXIS_PRIMARY_Y, 
                       (int32_t)(m_spectrumConfig.referenceLevel - m_spectrumConfig.dynamicRange), 
                       (int32_t)m_spectrumConfig.referenceLevel);
    
    // Add spectrum series
    lv_chart_series_t* spectrumSeries = lv_chart_add_series(m_spectrumChart, 
                                                             lv_color_hex(0x00FF00), 
                                                             LV_CHART_AXIS_PRIMARY_Y);
    
    // Configure chart appearance
    lv_chart_set_update_mode(m_spectrumChart, LV_CHART_UPDATE_MODE_CIRCULAR);
    lv_obj_set_style_line_width(m_spectrumChart, 2, LV_PART_ITEMS);
}

void RTLSDRApp::createWaterfallDisplay() {
    // Create waterfall container
    lv_obj_t* waterfallContainer = lv_obj_create(m_mainContainer);
    lv_obj_set_size(waterfallContainer, LV_PCT(100), LV_PCT(30));
    lv_obj_align_to(waterfallContainer, m_mainContainer, LV_ALIGN_TOP_MID, 0, LV_PCT(40));
    
    // Create canvas for waterfall
    m_waterfallCanvas = lv_canvas_create(waterfallContainer);
    lv_obj_set_size(m_waterfallCanvas, m_spectrumConfig.fftSize / 2, m_waterfallConfig.historyLines);
    lv_obj_center(m_waterfallCanvas);
    
    // Allocate canvas buffer
    size_t bufferSize = LV_CANVAS_BUF_SIZE_TRUE_COLOR(m_spectrumConfig.fftSize / 2, 
                                                       m_waterfallConfig.historyLines);
    void* canvasBuffer = heap_caps_malloc(bufferSize, MALLOC_CAP_SPIRAM);
    if (canvasBuffer) {
        lv_canvas_set_buffer(m_waterfallCanvas, canvasBuffer, 
                            m_spectrumConfig.fftSize / 2, 
                            m_waterfallConfig.historyLines, 
                            LV_IMG_CF_TRUE_COLOR);
        lv_canvas_fill_bg(m_waterfallCanvas, lv_color_black(), LV_OPA_COVER);
    }
}

void RTLSDRApp::createControlPanel() {
    // Create control panel container
    m_controlPanel = lv_obj_create(m_mainContainer);
    lv_obj_set_size(m_controlPanel, LV_PCT(100), LV_PCT(30));
    lv_obj_align_to(m_controlPanel, m_mainContainer, LV_ALIGN_BOTTOM_MID, 0, 0);
    
    createFrequencyControls();
    createDemodControls();
    createRecordingControls();
    
    // Create signal strength meter
    m_signalMeter = lv_bar_create(m_controlPanel);
    lv_obj_set_size(m_signalMeter, 200, 20);
    lv_obj_align(m_signalMeter, LV_ALIGN_TOP_RIGHT, -10, 10);
    lv_bar_set_range(m_signalMeter, -120, 0);
    lv_bar_set_value(m_signalMeter, -100, LV_ANIM_OFF);
    
    // Create status label
    m_statusLabel = lv_label_create(m_controlPanel);
    lv_label_set_text(m_statusLabel, "RTL-SDR Ready");
    lv_obj_align(m_statusLabel, LV_ALIGN_BOTTOM_LEFT, 10, -10);
}

void RTLSDRApp::createFrequencyControls() {
    // Frequency slider
    m_frequencySlider = lv_slider_create(m_controlPanel);
    lv_obj_set_size(m_frequencySlider, 300, 30);
    lv_obj_align(m_frequencySlider, LV_ALIGN_TOP_LEFT, 10, 10);
    lv_slider_set_range(m_frequencySlider, MIN_FREQUENCY / 1000000, MAX_FREQUENCY / 1000000);
    lv_slider_set_value(m_frequencySlider, m_currentFrequency / 1000000, LV_ANIM_OFF);
    lv_obj_add_event_cb(m_frequencySlider, frequencySliderCallback, LV_EVENT_VALUE_CHANGED, this);
    
    // Frequency label
    m_frequencyLabel = lv_label_create(m_controlPanel);
    lv_obj_align_to(m_frequencyLabel, m_frequencySlider, LV_ALIGN_OUT_BOTTOM_MID, 0, 5);
    updateFrequencyDisplay();
    
    // Band selection dropdown
    m_bandDropdown = lv_dropdown_create(m_controlPanel);
    lv_obj_align_to(m_bandDropdown, m_frequencySlider, LV_ALIGN_OUT_RIGHT_MID, 10, 0);
    
    // Populate band dropdown
    std::string bandOptions;
    for (const auto& band : m_frequencyBands) {
        if (!bandOptions.empty()) bandOptions += "\n";
        bandOptions += band.name;
    }
    lv_dropdown_set_options(m_bandDropdown, bandOptions.c_str());
    lv_obj_add_event_cb(m_bandDropdown, bandSelectCallback, LV_EVENT_VALUE_CHANGED, this);
    
    // Gain slider
    m_gainSlider = lv_slider_create(m_controlPanel);
    lv_obj_set_size(m_gainSlider, 150, 20);
    lv_obj_align(m_gainSlider, LV_ALIGN_TOP_LEFT, 10, 70);
    lv_slider_set_range(m_gainSlider, 0, 500);  // 0-50.0 dB
    lv_slider_set_value(m_gainSlider, (int32_t)(m_currentGain * 10), LV_ANIM_OFF);
    lv_obj_add_event_cb(m_gainSlider, gainControlCallback, LV_EVENT_VALUE_CHANGED, this);
}

void RTLSDRApp::createDemodControls() {
    // Demodulation mode dropdown
    m_demodDropdown = lv_dropdown_create(m_controlPanel);
    lv_obj_align(m_demodDropdown, LV_ALIGN_TOP_LEFT, 180, 70);
    lv_dropdown_set_options(m_demodDropdown, "FM Wide\nFM Narrow\nAM\nUSB\nLSB\nCW");
    lv_obj_add_event_cb(m_demodDropdown, demodModeCallback, LV_EVENT_VALUE_CHANGED, this);
    
    // Scan button
    m_scanButton = lv_btn_create(m_controlPanel);
    lv_obj_set_size(m_scanButton, 80, 40);
    lv_obj_align(m_scanButton, LV_ALIGN_TOP_LEFT, 10, 110);
    lv_obj_add_event_cb(m_scanButton, scanButtonCallback, LV_EVENT_CLICKED, this);
    
    lv_obj_t* scanLabel = lv_label_create(m_scanButton);
    lv_label_set_text(scanLabel, "SCAN");
    lv_obj_center(scanLabel);
}

void RTLSDRApp::createRecordingControls() {
    // Record button
    m_recordButton = lv_btn_create(m_controlPanel);
    lv_obj_set_size(m_recordButton, 80, 40);
    lv_obj_align(m_recordButton, LV_ALIGN_TOP_LEFT, 100, 110);
    lv_obj_add_event_cb(m_recordButton, recordButtonCallback, LV_EVENT_CLICKED, this);
    
    lv_obj_t* recordLabel = lv_label_create(m_recordButton);
    lv_label_set_text(recordLabel, "REC");
    lv_obj_center(recordLabel);
}

void RTLSDRApp::processSpectrumData(const std::vector<std::complex<float>>& samples) {
    if (samples.empty() || samples.size() < m_spectrumConfig.fftSize) {
        return;
    }
    
    // Use spectrum analyzer to compute spectrum
    SpectrumAnalyzer analyzer(m_spectrumConfig.fftSize, m_currentSampleRate);
    analyzer.setAveraging(m_spectrumConfig.averagingEnabled, m_spectrumConfig.averagingFactor);
    
    std::vector<float> spectrum;
    analyzer.analyzeSpectrum(samples, spectrum, m_currentFrequency);
    
    // Update displays
    updateSpectrumDisplay(spectrum);
    updateWaterfallDisplay(spectrum);
    
    // Update signal strength meter
    if (!spectrum.empty()) {
        float avgSignal = 0.0f;
        for (float val : spectrum) {
            avgSignal += val;
        }
        avgSignal /= spectrum.size();
        updateSignalStrengthMeter(avgSignal);
    }
    
    m_samplesProcessed += samples.size();
}

void RTLSDRApp::updateStatusBar() {
    if (!m_statusLabel) return;
    
    std::string status;
    if (!m_sdrService || !m_sdrService->isDeviceConnected()) {
        status = "No RTL-SDR Device";
    } else if (m_isRecording) {
        status = "Recording...";
    } else if (m_isScanning) {
        status = "Scanning...";
    } else if (m_sdrService->isStreaming()) {
        status = "Streaming";
    } else {
        status = "Ready";
    }
    
    lv_label_set_text(m_statusLabel, status.c_str());
}

void RTLSDRApp::setFrequency(uint32_t frequency) {
    if (frequency < MIN_FREQUENCY || frequency > MAX_FREQUENCY) {
        return;
    }
    
    m_currentFrequency = frequency;
    
    if (m_sdrService && m_sdrService->isDeviceConnected()) {
        m_sdrService->setFrequency(frequency);
    }
    
    updateFrequencyDisplay();
}

void RTLSDRApp::updateFrequencyDisplay() {
    if (!m_frequencyLabel) return;
    
    char freqStr[32];
    if (m_currentFrequency >= 1000000000) {
        snprintf(freqStr, sizeof(freqStr), "%.3f GHz", m_currentFrequency / 1000000000.0f);
    } else if (m_currentFrequency >= 1000000) {
        snprintf(freqStr, sizeof(freqStr), "%.3f MHz", m_currentFrequency / 1000000.0f);
    } else if (m_currentFrequency >= 1000) {
        snprintf(freqStr, sizeof(freqStr), "%.1f kHz", m_currentFrequency / 1000.0f);
    } else {
        snprintf(freqStr, sizeof(freqStr), "%u Hz", m_currentFrequency);
    }
    
    lv_label_set_text(m_frequencyLabel, freqStr);
}

void RTLSDRApp::loadConfiguration() {
    // TODO: Load configuration from storage
    ESP_LOGI(TAG, "Loading RTL-SDR configuration");
}

void RTLSDRApp::saveConfiguration() {
    // TODO: Save configuration to storage
    ESP_LOGI(TAG, "Saving RTL-SDR configuration");
}

// Event callbacks
void RTLSDRApp::frequencySliderCallback(lv_event_t* e) {
    RTLSDRApp* app = static_cast<RTLSDRApp*>(lv_event_get_user_data(e));
    if (!app) return;
    
    int32_t value = lv_slider_get_value(lv_event_get_target(e));
    uint32_t frequency = value * 1000000;  // Convert MHz to Hz
    app->setFrequency(frequency);
}

void RTLSDRApp::bandSelectCallback(lv_event_t* e) {
    RTLSDRApp* app = static_cast<RTLSDRApp*>(lv_event_get_user_data(e));
    if (!app) return;
    
    uint16_t selected = lv_dropdown_get_selected(lv_event_get_target(e));
    if (selected < app->m_frequencyBands.size()) {
        const auto& band = app->m_frequencyBands[selected];
        uint32_t centerFreq = (band.startFreq + band.endFreq) / 2;
        app->setFrequency(centerFreq);
    }
}

void RTLSDRApp::scanButtonCallback(lv_event_t* e) {
    RTLSDRApp* app = static_cast<RTLSDRApp*>(lv_event_get_user_data(e));
    if (!app) return;
    
    if (app->m_isScanning) {
        app->stopFrequencyScan();
    } else {
        // Start scan of current band
        uint16_t selectedBand = lv_dropdown_get_selected(app->m_bandDropdown);
        if (selectedBand < app->m_frequencyBands.size()) {
            const auto& band = app->m_frequencyBands[selectedBand];
            app->startFrequencyScan(band.startFreq, band.endFreq, 25000); // 25kHz steps
        }
    }
}

void RTLSDRApp::recordButtonCallback(lv_event_t* e) {
    RTLSDRApp* app = static_cast<RTLSDRApp*>(lv_event_get_user_data(e));
    if (!app) return;
    
    if (app->m_isRecording) {
        app->stopRecording();
    } else {
        char filename[64];
        snprintf(filename, sizeof(filename), "/sdcard/rtlsdr_%u.raw", 
                (unsigned int)(lv_tick_get() / 1000));
        app->startRecording(filename);
    }
}

void RTLSDRApp::gainControlCallback(lv_event_t* e) {
    RTLSDRApp* app = static_cast<RTLSDRApp*>(lv_event_get_user_data(e));
    if (!app) return;
    
    int32_t value = lv_slider_get_value(lv_event_get_target(e));
    float gain = value / 10.0f;  // Convert to dB
    app->setGain(gain);
}

void RTLSDRApp::demodModeCallback(lv_event_t* e) {
    RTLSDRApp* app = static_cast<RTLSDRApp*>(lv_event_get_user_data(e));
    if (!app) return;
    
    uint16_t selected = lv_dropdown_get_selected(lv_event_get_target(e));
    
    switch (selected) {
        case 0: app->m_audioConfig.type = AudioDemodConfig::FM_WIDE; break;
        case 1: app->m_audioConfig.type = AudioDemodConfig::FM_NARROW; break;
        case 2: app->m_audioConfig.type = AudioDemodConfig::AM; break;
        case 3: app->m_audioConfig.type = AudioDemodConfig::USB; break;
        case 4: app->m_audioConfig.type = AudioDemodConfig::LSB; break;
        case 5: app->m_audioConfig.type = AudioDemodConfig::CW; break;
    }
}

void RTLSDRApp::updateSpectrumDisplay(const std::vector<float>& magnitudes) {
    if (!m_spectrumChart || magnitudes.empty()) return;
    
    // Get the spectrum series
    lv_chart_series_t* series = lv_chart_get_series_next(m_spectrumChart, nullptr);
    if (!series) return;
    
    // Update chart data
    for (size_t i = 0; i < magnitudes.size() && i < m_spectrumConfig.fftSize / 2; i++) {
        lv_chart_set_next_value(m_spectrumChart, series, (int32_t)magnitudes[i]);
    }
    
    lv_chart_refresh(m_spectrumChart);
}

void RTLSDRApp::updateWaterfallDisplay(const std::vector<float>& magnitudes) {
    if (!m_waterfallCanvas || magnitudes.empty()) return;
    
    // Scroll waterfall up by one line
    lv_img_dsc_t* imgDsc = lv_canvas_get_img(m_waterfallCanvas);
    if (!imgDsc || !imgDsc->data) return;
    
    size_t lineWidth = m_spectrumConfig.fftSize / 2;
    size_t lineBytes = lineWidth * sizeof(lv_color_t);
    
    // Move existing lines up
    uint8_t* canvasData = (uint8_t*)imgDsc->data;
    memmove(canvasData, canvasData + lineBytes, 
            lineBytes * (m_waterfallConfig.historyLines - 1));
    
    // Add new line at bottom
    lv_color_t* bottomLine = (lv_color_t*)(canvasData + 
                                          lineBytes * (m_waterfallConfig.historyLines - 1));
    
    for (size_t i = 0; i < lineWidth && i < magnitudes.size(); i++) {
        // Convert magnitude to color index
        float normalized = (magnitudes[i] - (m_spectrumConfig.referenceLevel - m_spectrumConfig.dynamicRange)) 
                          / m_spectrumConfig.dynamicRange;
        normalized = std::max(0.0f, std::min(1.0f, normalized));
        
        int colorIndex = (int)(normalized * 255);
        bottomLine[i] = m_waterfallConfig.colorMap[colorIndex];
    }
    
    lv_obj_invalidate(m_waterfallCanvas);
}

void RTLSDRApp::updateSignalStrengthMeter(float strength) {
    if (!m_signalMeter) return;
    
    lv_bar_set_value(m_signalMeter, (int32_t)strength, LV_ANIM_ON);
}

void RTLSDRApp::setGain(float gain) {
    m_currentGain = gain;
    
    if (m_sdrService && m_sdrService->isDeviceConnected()) {
        m_sdrService->setGain(gain);
    }
}

void RTLSDRApp::startFrequencyScan(uint32_t startFreq, uint32_t endFreq, uint32_t stepSize) {
    if (m_isScanning) return;
    
    ESP_LOGI(TAG, "Starting frequency scan: %u - %u Hz, step %u Hz", 
             startFreq, endFreq, stepSize);
    
    m_isScanning = true;
    // TODO: Implement frequency scanning logic
}

void RTLSDRApp::stopFrequencyScan() {
    if (!m_isScanning) return;
    
    ESP_LOGI(TAG, "Stopping frequency scan");
    m_isScanning = false;
}

void RTLSDRApp::startRecording(const char* filename) {
    if (m_isRecording) return;
    
    ESP_LOGI(TAG, "Starting recording to: %s", filename);
    m_isRecording = true;
    // TODO: Implement recording logic
}

void RTLSDRApp::stopRecording() {
    if (!m_isRecording) return;
    
    ESP_LOGI(TAG, "Stopping recording");
    m_isRecording = false;
}

void RTLSDRApp::processAudioData(const std::vector<std::complex<float>>& samples) {
    if (!m_isDemodulating || samples.empty()) return;
    
    // TODO: Implement audio demodulation and output
    ESP_LOGD(TAG, "Processing %zu samples for audio", samples.size());
}