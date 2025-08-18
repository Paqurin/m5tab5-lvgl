#ifndef SIGNAL_PROCESSING_H
#define SIGNAL_PROCESSING_H

#include <vector>
#include <complex>
#include <cmath>
#include <memory>

/**
 * @file signal_processing.h
 * @brief Real-time Signal Processing Library for RTL-SDR
 * 
 * Optimized DSP algorithms for embedded systems including
 * FFT, filtering, demodulation, and spectrum analysis.
 */

class FFTProcessor {
public:
    FFTProcessor(size_t size);
    ~FFTProcessor();

    /**
     * @brief Compute FFT of complex input
     * @param input Input samples
     * @param output Output frequency domain
     */
    void computeFFT(const std::vector<std::complex<float>>& input, 
                    std::vector<std::complex<float>>& output);

    /**
     * @brief Compute magnitude spectrum
     * @param input Input samples
     * @param magnitudes Output magnitudes in dB
     */
    void computeMagnitudeSpectrum(const std::vector<std::complex<float>>& input,
                                  std::vector<float>& magnitudes);

    /**
     * @brief Compute power spectral density
     * @param input Input samples
     * @param psd Output power spectral density
     */
    void computePSD(const std::vector<std::complex<float>>& input,
                    std::vector<float>& psd);

private:
    size_t m_size;
    std::vector<std::complex<float>> m_twiddle;
    std::vector<float> m_window;  // Hanning window
    
    void generateTwiddle();
    void generateWindow();
    void radix2FFT(std::vector<std::complex<float>>& data);
};

class DigitalFilter {
public:
    enum FilterType {
        LOW_PASS,
        HIGH_PASS,
        BAND_PASS,
        BAND_STOP
    };

    DigitalFilter(FilterType type, float cutoffFreq, float sampleRate, int order = 4);
    DigitalFilter(FilterType type, float lowFreq, float highFreq, float sampleRate, int order = 4);
    ~DigitalFilter();

    /**
     * @brief Filter complex samples
     * @param input Input samples
     * @param output Filtered output
     */
    void filter(const std::vector<std::complex<float>>& input,
                std::vector<std::complex<float>>& output);

    /**
     * @brief Filter real samples
     * @param input Input samples
     * @param output Filtered output
     */
    void filter(const std::vector<float>& input,
                std::vector<float>& output);

    /**
     * @brief Reset filter state
     */
    void reset();

private:
    FilterType m_type;
    std::vector<float> m_b_coeffs;  // Numerator coefficients
    std::vector<float> m_a_coeffs;  // Denominator coefficients
    std::vector<std::complex<float>> m_delay_line;
    
    void designButterworthFilter(float cutoff, float sampleRate, int order);
    void designBandpassFilter(float lowFreq, float highFreq, float sampleRate, int order);
};

class AudioDemodulator {
public:
    enum DemodType {
        AM,
        FM,
        USB,
        LSB
    };

    AudioDemodulator(DemodType type, float sampleRate);
    ~AudioDemodulator();

    /**
     * @brief Demodulate I/Q samples to audio
     * @param iqSamples Input I/Q samples
     * @param audioOutput Output audio samples
     */
    void demodulate(const std::vector<std::complex<float>>& iqSamples,
                    std::vector<float>& audioOutput);

    /**
     * @brief Set audio sample rate
     * @param sampleRate Sample rate in Hz
     */
    void setAudioSampleRate(float sampleRate) { m_audioSampleRate = sampleRate; }

    /**
     * @brief Set demodulation bandwidth
     * @param bandwidth Bandwidth in Hz
     */
    void setBandwidth(float bandwidth);

private:
    DemodType m_type;
    float m_sampleRate;
    float m_audioSampleRate;
    float m_bandwidth;
    
    // Demodulation state
    float m_lastPhase = 0.0f;
    std::complex<float> m_lastSample = {0.0f, 0.0f};
    
    // Filters
    std::unique_ptr<DigitalFilter> m_audioFilter;
    std::unique_ptr<DigitalFilter> m_rfFilter;
    
    // Decimation/interpolation
    std::vector<float> m_decimationBuffer;
    size_t m_decimationFactor = 1;
    
    void demodulateAM(const std::vector<std::complex<float>>& input, std::vector<float>& output);
    void demodulateFM(const std::vector<std::complex<float>>& input, std::vector<float>& output);
    void demodulateSSB(const std::vector<std::complex<float>>& input, std::vector<float>& output, bool upperSideband);
};

class SpectrumAnalyzer {
public:
    SpectrumAnalyzer(size_t fftSize = 1024, float sampleRate = 2048000.0f);
    ~SpectrumAnalyzer();

    /**
     * @brief Analyze spectrum of input signal
     * @param samples Input I/Q samples
     * @param spectrum Output spectrum (dBm)
     * @param centerFreq Center frequency in Hz
     */
    void analyzeSpectrum(const std::vector<std::complex<float>>& samples,
                         std::vector<float>& spectrum,
                         float centerFreq);

    /**
     * @brief Enable/disable averaging
     * @param enable Enable averaging
     * @param factor Averaging factor (0.0-1.0)
     */
    void setAveraging(bool enable, float factor = 0.8f);

    /**
     * @brief Set noise floor estimation
     * @param enable Enable noise floor estimation
     */
    void setNoiseFloorEstimation(bool enable) { m_estimateNoiseFloor = enable; }

    /**
     * @brief Get current noise floor
     * @return Noise floor in dBm
     */
    float getNoiseFloor() const { return m_noiseFloor; }

    /**
     * @brief Find peaks in spectrum
     * @param spectrum Input spectrum
     * @param peaks Output peak frequencies
     * @param threshold Minimum peak threshold (dB above noise floor)
     */
    void findPeaks(const std::vector<float>& spectrum,
                   std::vector<float>& peaks,
                   float threshold = 10.0f);

private:
    size_t m_fftSize;
    float m_sampleRate;
    
    std::unique_ptr<FFTProcessor> m_fft;
    
    // Averaging
    bool m_averagingEnabled = false;
    float m_averagingFactor = 0.8f;
    std::vector<float> m_averagedSpectrum;
    bool m_firstSpectrum = true;
    
    // Noise floor estimation
    bool m_estimateNoiseFloor = true;
    float m_noiseFloor = -100.0f;
    std::vector<float> m_noiseHistory;
    
    void updateNoiseFloor(const std::vector<float>& spectrum);
};

// Utility functions
namespace DSPUtils {
    /**
     * @brief Convert power to dBm
     * @param power Power value
     * @return Power in dBm
     */
    inline float powerToDBM(float power) {
        return 10.0f * log10f(power) + 30.0f;  // Assuming 50 ohm impedance
    }
    
    /**
     * @brief Convert magnitude to dB
     * @param magnitude Magnitude value
     * @return Magnitude in dB
     */
    inline float magnitudeToDB(float magnitude) {
        return 20.0f * log10f(magnitude);
    }
    
    /**
     * @brief Complex magnitude
     * @param sample Complex sample
     * @return Magnitude
     */
    inline float complexMagnitude(const std::complex<float>& sample) {
        return sqrtf(sample.real() * sample.real() + sample.imag() * sample.imag());
    }
    
    /**
     * @brief Decimate signal by integer factor
     * @param input Input signal
     * @param output Decimated output
     * @param factor Decimation factor
     */
    void decimate(const std::vector<float>& input, std::vector<float>& output, int factor);
    
    /**
     * @brief Interpolate signal by integer factor
     * @param input Input signal
     * @param output Interpolated output
     * @param factor Interpolation factor
     */
    void interpolate(const std::vector<float>& input, std::vector<float>& output, int factor);
    
    /**
     * @brief Apply window function
     * @param data Data to window
     * @param window Window coefficients
     */
    void applyWindow(std::vector<std::complex<float>>& data, const std::vector<float>& window);
}

#endif // SIGNAL_PROCESSING_H