#ifndef RTL_SDR_APP_H
#define RTL_SDR_APP_H

#include "base_app.h"
#include "../services/rtl_sdr_service.h"
#include <lvgl.h>
#include <vector>
#include <complex>

/**
 * @file rtl_sdr_app.h
 * @brief RTL-SDR Software Defined Radio Application
 * 
 * Provides real-time spectrum analysis, waterfall display,
 * frequency scanning, and audio demodulation capabilities
 * using USB RTL-SDR dongles.
 */

struct FrequencyRange {
    uint32_t startFreq;  // Hz
    uint32_t endFreq;    // Hz
    const char* name;
    const char* description;
};

struct AudioDemodConfig {
    enum Type {
        AM,
        FM_NARROW,
        FM_WIDE,
        USB,
        LSB,
        CW
    } type = FM_WIDE;
    
    uint32_t sampleRate = 1024000;  // 1.024 MHz
    uint32_t bandwidth = 200000;     // 200 kHz
    float volume = 0.7f;
    bool squelchEnabled = true;
    float squelchLevel = -80.0f;     // dBm
};

struct SpectrumConfig {
    uint32_t fftSize = 1024;
    uint32_t updateRate = 30;        // Hz
    float dynamicRange = 80.0f;      // dB
    float referenceLevel = 0.0f;     // dBm
    bool averagingEnabled = true;
    float averagingFactor = 0.8f;
    bool peakHold = false;
};

struct WaterfallConfig {
    uint32_t historyLines = 256;
    uint32_t updateRate = 15;        // Hz
    float intensityScale = 1.0f;
    bool autoScale = true;
    lv_color_t colorMap[256];        // Color palette
};

class RTLSDRApp : public BaseApp {
public:
    RTLSDRApp();
    virtual ~RTLSDRApp();

    // BaseApp interface
    os_error_t initialize() override;
    os_error_t update(uint32_t deltaTime) override;
    os_error_t shutdown() override;
    os_error_t createUI(lv_obj_t* parent) override;
    os_error_t destroyUI() override;
    os_error_t handleEvent(uint32_t eventType, void* eventData, size_t dataSize) override;
    
    // Additional RTL-SDR specific methods
    bool requiresUSB() const { return true; }
    size_t getRequiredMemory() const { return 8 * 1024 * 1024; } // 8MB
    AppPriority getRTLSDRPriority() const { return AppPriority::APP_HIGH; }

private:
    // UI Creation
    void createMainUI();
    void createSpectrumDisplay();
    void createWaterfallDisplay();
    void createControlPanel();
    void createFrequencyControls();
    void createDemodControls();
    void createRecordingControls();
    
    // Event Handlers
    static void frequencySliderCallback(lv_event_t* e);
    static void demodModeCallback(lv_event_t* e);
    static void scanButtonCallback(lv_event_t* e);
    static void recordButtonCallback(lv_event_t* e);
    static void bandSelectCallback(lv_event_t* e);
    static void gainControlCallback(lv_event_t* e);
    
    // Signal Processing
    void processSpectrumData(const std::vector<std::complex<float>>& samples);
    void updateSpectrumDisplay(const std::vector<float>& magnitudes);
    void updateWaterfallDisplay(const std::vector<float>& magnitudes);
    void processAudioData(const std::vector<std::complex<float>>& samples);
    
    // Frequency Management
    void setFrequency(uint32_t frequency);
    void setSampleRate(uint32_t sampleRate);
    void setGain(float gain);
    void setBandwidth(uint32_t bandwidth);
    
    // Scanning Functions
    void startFrequencyScan(uint32_t startFreq, uint32_t endFreq, uint32_t stepSize);
    void stopFrequencyScan();
    void processSignalStrength(float signalLevel);
    
    // Recording Functions
    void startRecording(const char* filename);
    void stopRecording();
    void saveIQData(const std::vector<std::complex<float>>& samples);
    
    // Demodulation
    void demodulateAM(const std::vector<std::complex<float>>& samples, std::vector<float>& audio);
    void demodulateFM(const std::vector<std::complex<float>>& samples, std::vector<float>& audio);
    void demodulateSSB(const std::vector<std::complex<float>>& samples, std::vector<float>& audio, bool upperSideband);
    
    // UI State Management
    void updateFrequencyDisplay();
    void updateSignalStrengthMeter(float strength);
    void updateSpectrumMarkers();
    void updateStatusBar();
    
    // Configuration Management
    void loadConfiguration();
    void saveConfiguration();
    void resetToDefaults();
    
    // RTL-SDR Service Interface
    RTLSDRService* m_sdrService;
    
    // UI Components
    lv_obj_t* m_mainContainer;
    lv_obj_t* m_spectrumChart;
    lv_obj_t* m_waterfallCanvas;
    lv_obj_t* m_controlPanel;
    lv_obj_t* m_frequencySlider;
    lv_obj_t* m_frequencyLabel;
    lv_obj_t* m_gainSlider;
    lv_obj_t* m_demodDropdown;
    lv_obj_t* m_bandDropdown;
    lv_obj_t* m_scanButton;
    lv_obj_t* m_recordButton;
    lv_obj_t* m_signalMeter;
    lv_obj_t* m_statusLabel;
    
    // Configuration
    AudioDemodConfig m_audioConfig;
    SpectrumConfig m_spectrumConfig;
    WaterfallConfig m_waterfallConfig;
    
    // Current State
    uint32_t m_currentFrequency = 100000000;  // 100 MHz
    uint32_t m_currentSampleRate = 2048000;   // 2.048 MHz
    float m_currentGain = 20.0f;              // dB
    bool m_isScanning = false;
    bool m_isRecording = false;
    bool m_isDemodulating = false;
    
    // Frequency Bands
    std::vector<FrequencyRange> m_frequencyBands;
    
    // Spectrum Data
    std::vector<float> m_spectrumData;
    std::vector<float> m_waterfallHistory;
    std::vector<std::complex<float>> m_iqBuffer;
    
    // Performance Monitoring
    uint32_t m_lastSpectrumUpdate = 0;
    uint32_t m_lastWaterfallUpdate = 0;
    uint32_t m_samplesProcessed = 0;
    uint32_t m_bufferOverruns = 0;
    
    // Constants
    static constexpr uint32_t SPECTRUM_UPDATE_INTERVAL = 33;  // ~30 FPS
    static constexpr uint32_t WATERFALL_UPDATE_INTERVAL = 66; // ~15 FPS
    static constexpr uint32_t AUDIO_SAMPLE_RATE = 48000;
    static constexpr uint32_t MAX_FREQUENCY = 1700000000;     // 1.7 GHz
    static constexpr uint32_t MIN_FREQUENCY = 24000000;       // 24 MHz
    
    static constexpr const char* TAG = "RTLSDRApp";
};

#endif // RTL_SDR_APP_H