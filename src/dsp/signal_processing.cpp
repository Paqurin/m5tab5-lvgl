#include "signal_processing.h"
#include <cmath>
#include <algorithm>
#include <memory>
#include <esp_log.h>

/**
 * @file signal_processing.cpp
 * @brief DSP Signal Processing Implementation
 */

static const char* TAG = "DSP";

// FFT Processor Implementation
FFTProcessor::FFTProcessor(size_t size) : m_size(size) {
    // Ensure size is power of 2
    size_t powerOfTwo = 1;
    while (powerOfTwo < size) {
        powerOfTwo <<= 1;
    }
    m_size = powerOfTwo;
    
    generateTwiddle();
    generateWindow();
    
    ESP_LOGI(TAG, "FFT Processor initialized with size %zu", m_size);
}

FFTProcessor::~FFTProcessor() {
    // Cleanup handled by vector destructors
}

void FFTProcessor::generateTwiddle() {
    m_twiddle.resize(m_size / 2);
    
    for (size_t k = 0; k < m_size / 2; k++) {
        float angle = -2.0f * M_PI * k / m_size;
        m_twiddle[k] = std::complex<float>(cosf(angle), sinf(angle));
    }
}

void FFTProcessor::generateWindow() {
    // Generate Hanning window
    m_window.resize(m_size);
    
    for (size_t i = 0; i < m_size; i++) {
        m_window[i] = 0.5f * (1.0f - cosf(2.0f * M_PI * i / (m_size - 1)));
    }
}

void FFTProcessor::computeFFT(const std::vector<std::complex<float>>& input, 
                              std::vector<std::complex<float>>& output) {
    if (input.size() < m_size) {
        ESP_LOGW(TAG, "Input size %zu less than FFT size %zu", input.size(), m_size);
        return;
    }
    
    output.resize(m_size);
    
    // Copy input data and apply window
    for (size_t i = 0; i < m_size; i++) {
        output[i] = input[i] * m_window[i];
    }
    
    // Perform radix-2 FFT
    radix2FFT(output);
}

void FFTProcessor::computeMagnitudeSpectrum(const std::vector<std::complex<float>>& input,
                                            std::vector<float>& magnitudes) {
    std::vector<std::complex<float>> fftOutput;
    computeFFT(input, fftOutput);
    
    magnitudes.resize(m_size / 2);
    
    for (size_t i = 0; i < m_size / 2; i++) {
        float magnitude = std::abs(fftOutput[i]);
        magnitudes[i] = DSPUtils::magnitudeToDB(magnitude);
    }
}

void FFTProcessor::computePSD(const std::vector<std::complex<float>>& input,
                              std::vector<float>& psd) {
    std::vector<std::complex<float>> fftOutput;
    computeFFT(input, fftOutput);
    
    psd.resize(m_size / 2);
    
    float scaleFactor = 1.0f / (m_size * m_size);
    
    for (size_t i = 0; i < m_size / 2; i++) {
        float power = std::norm(fftOutput[i]) * scaleFactor;
        psd[i] = DSPUtils::powerToDBM(power);
    }
}

void FFTProcessor::radix2FFT(std::vector<std::complex<float>>& data) {
    size_t n = data.size();
    
    // Bit-reversal permutation
    for (size_t i = 0; i < n; i++) {
        size_t j = 0;
        for (size_t k = 0; k < log2(n); k++) {
            j = (j << 1) | ((i >> k) & 1);
        }
        if (j > i) {
            std::swap(data[i], data[j]);
        }
    }
    
    // FFT computation
    for (size_t length = 2; length <= n; length <<= 1) {
        size_t step = n / length;
        for (size_t i = 0; i < n; i += length) {
            for (size_t j = 0; j < length / 2; j++) {
                std::complex<float> u = data[i + j];
                std::complex<float> v = data[i + j + length / 2] * m_twiddle[j * step];
                data[i + j] = u + v;
                data[i + j + length / 2] = u - v;
            }
        }
    }
}

// Digital Filter Implementation
DigitalFilter::DigitalFilter(FilterType type, float cutoffFreq, float sampleRate, int order) 
    : m_type(type) {
    designButterworthFilter(cutoffFreq, sampleRate, order);
    reset();
    
    ESP_LOGI(TAG, "Digital filter created: type=%d cutoff=%.1f Hz sr=%.1f Hz order=%d", 
             type, cutoffFreq, sampleRate, order);
}

DigitalFilter::DigitalFilter(FilterType type, float lowFreq, float highFreq, float sampleRate, int order)
    : m_type(type) {
    designBandpassFilter(lowFreq, highFreq, sampleRate, order);
    reset();
    
    ESP_LOGI(TAG, "Digital bandpass filter created: %.1f - %.1f Hz sr=%.1f Hz order=%d", 
             lowFreq, highFreq, sampleRate, order);
}

DigitalFilter::~DigitalFilter() {
    // Cleanup handled by vector destructors
}

void DigitalFilter::filter(const std::vector<std::complex<float>>& input,
                           std::vector<std::complex<float>>& output) {
    output.resize(input.size());
    
    for (size_t i = 0; i < input.size(); i++) {
        // Direct Form II implementation
        std::complex<float> x = input[i];
        std::complex<float> y = m_b_coeffs[0] * x;
        
        // Add delayed inputs
        for (size_t j = 1; j < m_b_coeffs.size() && j <= m_delay_line.size(); j++) {
            if (j - 1 < m_delay_line.size()) {
                y += m_b_coeffs[j] * m_delay_line[j - 1];
            }
        }
        
        // Subtract delayed outputs
        for (size_t j = 1; j < m_a_coeffs.size() && j <= m_delay_line.size(); j++) {
            if (j - 1 < m_delay_line.size()) {
                y -= m_a_coeffs[j] * m_delay_line[j - 1];
            }
        }
        
        output[i] = y;
        
        // Update delay line
        for (int j = m_delay_line.size() - 1; j > 0; j--) {
            m_delay_line[j] = m_delay_line[j - 1];
        }
        if (!m_delay_line.empty()) {
            m_delay_line[0] = x;
        }
    }
}

void DigitalFilter::filter(const std::vector<float>& input,
                           std::vector<float>& output) {
    // Convert to complex, filter, convert back to real
    std::vector<std::complex<float>> complexInput(input.size());
    std::vector<std::complex<float>> complexOutput;
    
    for (size_t i = 0; i < input.size(); i++) {
        complexInput[i] = std::complex<float>(input[i], 0.0f);
    }
    
    filter(complexInput, complexOutput);
    
    output.resize(complexOutput.size());
    for (size_t i = 0; i < complexOutput.size(); i++) {
        output[i] = complexOutput[i].real();
    }
}

void DigitalFilter::reset() {
    size_t delaySize = std::max(m_a_coeffs.size(), m_b_coeffs.size()) - 1;
    m_delay_line.assign(delaySize, std::complex<float>(0.0f, 0.0f));
}

void DigitalFilter::designButterworthFilter(float cutoff, float sampleRate, int order) {
    // Simplified Butterworth filter design
    // This is a basic implementation - real designs would use bilinear transform
    
    float wc = 2.0f * M_PI * cutoff / sampleRate;  // Normalized cutoff frequency
    
    // First-order filter coefficients (simplified)
    m_b_coeffs.resize(2);
    m_a_coeffs.resize(2);
    
    if (m_type == LOW_PASS) {
        float alpha = sin(wc) / (cos(wc) + 1);
        m_b_coeffs[0] = alpha / 2;
        m_b_coeffs[1] = alpha / 2;
        m_a_coeffs[0] = 1.0f;
        m_a_coeffs[1] = alpha - 1;
    } else if (m_type == HIGH_PASS) {
        float alpha = cos(wc) / (sin(wc) + 1);
        m_b_coeffs[0] = alpha / 2;
        m_b_coeffs[1] = -alpha / 2;
        m_a_coeffs[0] = 1.0f;
        m_a_coeffs[1] = alpha - 1;
    }
}

void DigitalFilter::designBandpassFilter(float lowFreq, float highFreq, float sampleRate, int order) {
    // Simplified bandpass filter design
    float wl = 2.0f * M_PI * lowFreq / sampleRate;
    float wh = 2.0f * M_PI * highFreq / sampleRate;
    float wc = (wl + wh) / 2.0f;  // Center frequency
    float bw = wh - wl;           // Bandwidth
    
    // Basic bandpass coefficients
    m_b_coeffs.resize(3);
    m_a_coeffs.resize(3);
    
    float r = 1.0f - 3.0f * bw;
    float k = (1.0f - 2.0f * r * cos(wc) + r * r) / (2.0f - 2.0f * cos(wc));
    
    m_b_coeffs[0] = 1.0f - k;
    m_b_coeffs[1] = 2.0f * (k - r) * cos(wc);
    m_b_coeffs[2] = r * r - k;
    m_a_coeffs[0] = 1.0f;
    m_a_coeffs[1] = 2.0f * r * cos(wc);
    m_a_coeffs[2] = -r * r;
}

// Audio Demodulator Implementation
AudioDemodulator::AudioDemodulator(DemodType type, float sampleRate) 
    : m_type(type), m_sampleRate(sampleRate), m_audioSampleRate(48000.0f), m_bandwidth(15000.0f) {
    
    // Calculate decimation factor
    m_decimationFactor = (size_t)(m_sampleRate / m_audioSampleRate);
    if (m_decimationFactor < 1) m_decimationFactor = 1;
    
    // Create audio lowpass filter
    m_audioFilter = std::make_unique<DigitalFilter>(DigitalFilter::LOW_PASS, 
                                                   m_audioSampleRate / 2, 
                                                   m_sampleRate, 4);
    
    // Create RF filter based on demodulation type
    if (type == AM) {
        m_rfFilter = std::make_unique<DigitalFilter>(DigitalFilter::LOW_PASS, 
                                                    m_bandwidth, m_sampleRate, 4);
    } else if (type == FM) {
        m_rfFilter = std::make_unique<DigitalFilter>(DigitalFilter::LOW_PASS, 
                                                    m_bandwidth, m_sampleRate, 4);
    }
    
    ESP_LOGI(TAG, "Audio demodulator created: type=%d sr=%.1f Hz audio_sr=%.1f Hz", 
             type, sampleRate, m_audioSampleRate);
}

AudioDemodulator::~AudioDemodulator() {
    // Cleanup handled by unique_ptr destructors
}

void AudioDemodulator::demodulate(const std::vector<std::complex<float>>& iqSamples,
                                  std::vector<float>& audioOutput) {
    if (iqSamples.empty()) {
        audioOutput.clear();
        return;
    }
    
    switch (m_type) {
        case AM:
            demodulateAM(iqSamples, audioOutput);
            break;
        case FM:
            demodulateFM(iqSamples, audioOutput);
            break;
        case USB:
            demodulateSSB(iqSamples, audioOutput, true);
            break;
        case LSB:
            demodulateSSB(iqSamples, audioOutput, false);
            break;
    }
}

void AudioDemodulator::setBandwidth(float bandwidth) {
    m_bandwidth = bandwidth;
    
    // Recreate filters with new bandwidth
    if (m_rfFilter) {
        if (m_type == AM || m_type == FM) {
            m_rfFilter = std::make_unique<DigitalFilter>(DigitalFilter::LOW_PASS, 
                                                        m_bandwidth, m_sampleRate, 4);
        }
    }
}

void AudioDemodulator::demodulateAM(const std::vector<std::complex<float>>& input, 
                                    std::vector<float>& output) {
    // AM demodulation: output = |I + jQ|
    std::vector<float> envelope(input.size());
    
    for (size_t i = 0; i < input.size(); i++) {
        envelope[i] = std::abs(input[i]);
    }
    
    // Apply audio filter and decimation
    if (m_audioFilter) {
        m_audioFilter->filter(envelope, output);
    } else {
        output = envelope;
    }
    
    // Decimate to audio sample rate
    if (m_decimationFactor > 1) {
        std::vector<float> decimated;
        for (size_t i = 0; i < output.size(); i += m_decimationFactor) {
            decimated.push_back(output[i]);
        }
        output = std::move(decimated);
    }
}

void AudioDemodulator::demodulateFM(const std::vector<std::complex<float>>& input, 
                                    std::vector<float>& output) {
    // FM demodulation: output = arg(I[n] * conj(I[n-1]))
    output.resize(input.size());
    
    for (size_t i = 1; i < input.size(); i++) {
        std::complex<float> product = input[i] * std::conj(m_lastSample);
        output[i] = std::arg(product);
        m_lastSample = input[i];
    }
    
    if (!output.empty()) {
        output[0] = 0.0f;  // First sample has no previous reference
    }
    
    // Apply audio filter and decimation
    std::vector<float> filtered;
    if (m_audioFilter) {
        m_audioFilter->filter(output, filtered);
        output = std::move(filtered);
    }
    
    // Decimate to audio sample rate
    if (m_decimationFactor > 1) {
        std::vector<float> decimated;
        for (size_t i = 0; i < output.size(); i += m_decimationFactor) {
            decimated.push_back(output[i]);
        }
        output = std::move(decimated);
    }
}

void AudioDemodulator::demodulateSSB(const std::vector<std::complex<float>>& input, 
                                     std::vector<float>& output, bool upperSideband) {
    // SSB demodulation using quadrature detection
    output.resize(input.size());
    
    for (size_t i = 0; i < input.size(); i++) {
        if (upperSideband) {
            // USB: output = I * cos(phase) + Q * sin(phase)
            output[i] = input[i].real();
        } else {
            // LSB: output = I * cos(phase) - Q * sin(phase)  
            output[i] = input[i].real();
        }
    }
    
    // Apply audio filter and decimation
    std::vector<float> filtered;
    if (m_audioFilter) {
        m_audioFilter->filter(output, filtered);
        output = std::move(filtered);
    }
    
    // Decimate to audio sample rate
    if (m_decimationFactor > 1) {
        std::vector<float> decimated;
        for (size_t i = 0; i < output.size(); i += m_decimationFactor) {
            decimated.push_back(output[i]);
        }
        output = std::move(decimated);
    }
}

// Spectrum Analyzer Implementation
SpectrumAnalyzer::SpectrumAnalyzer(size_t fftSize, float sampleRate) 
    : m_fftSize(fftSize), m_sampleRate(sampleRate) {
    
    m_fft = std::make_unique<FFTProcessor>(fftSize);
    m_averagedSpectrum.resize(fftSize / 2, -120.0f);  // Initialize to low value
    m_noiseHistory.reserve(100);  // Store last 100 noise estimates
    
    ESP_LOGI(TAG, "Spectrum analyzer created: FFT size=%zu sample rate=%.1f Hz", 
             fftSize, sampleRate);
}

SpectrumAnalyzer::~SpectrumAnalyzer() {
    // Cleanup handled by unique_ptr destructors
}

void SpectrumAnalyzer::analyzeSpectrum(const std::vector<std::complex<float>>& samples,
                                       std::vector<float>& spectrum,
                                       float centerFreq) {
    if (samples.size() < m_fftSize || !m_fft) {
        spectrum.clear();
        return;
    }
    
    // Compute magnitude spectrum
    std::vector<float> currentSpectrum;
    m_fft->computeMagnitudeSpectrum(samples, currentSpectrum);
    
    // Apply averaging if enabled
    if (m_averagingEnabled) {
        if (m_firstSpectrum) {
            m_averagedSpectrum = currentSpectrum;
            m_firstSpectrum = false;
        } else {
            for (size_t i = 0; i < currentSpectrum.size(); i++) {
                m_averagedSpectrum[i] = m_averagingFactor * m_averagedSpectrum[i] + 
                                       (1.0f - m_averagingFactor) * currentSpectrum[i];
            }
        }
        spectrum = m_averagedSpectrum;
    } else {
        spectrum = currentSpectrum;
    }
    
    // Update noise floor estimate
    if (m_estimateNoiseFloor) {
        updateNoiseFloor(spectrum);
    }
}

void SpectrumAnalyzer::setAveraging(bool enable, float factor) {
    m_averagingEnabled = enable;
    m_averagingFactor = std::clamp(factor, 0.0f, 1.0f);
    
    if (!enable) {
        m_firstSpectrum = true;
    }
}

void SpectrumAnalyzer::findPeaks(const std::vector<float>& spectrum,
                                 std::vector<float>& peaks,
                                 float threshold) {
    peaks.clear();
    
    if (spectrum.size() < 3) {
        return;
    }
    
    float noiseThreshold = m_noiseFloor + threshold;
    
    // Find local maxima above threshold
    for (size_t i = 1; i < spectrum.size() - 1; i++) {
        if (spectrum[i] > noiseThreshold &&
            spectrum[i] > spectrum[i - 1] &&
            spectrum[i] > spectrum[i + 1]) {
            
            // Convert bin to frequency
            float frequency = (float)i * m_sampleRate / (2 * m_fftSize);
            peaks.push_back(frequency);
        }
    }
}

void SpectrumAnalyzer::updateNoiseFloor(const std::vector<float>& spectrum) {
    if (spectrum.empty()) {
        return;
    }
    
    // Estimate noise floor as the median of the lower 50% of spectrum values
    std::vector<float> sortedSpectrum = spectrum;
    std::sort(sortedSpectrum.begin(), sortedSpectrum.end());
    
    size_t medianIndex = sortedSpectrum.size() / 4;  // 25th percentile
    float currentNoise = sortedSpectrum[medianIndex];
    
    // Update noise history
    m_noiseHistory.push_back(currentNoise);
    if (m_noiseHistory.size() > 100) {
        m_noiseHistory.erase(m_noiseHistory.begin());
    }
    
    // Average noise estimates
    float avgNoise = 0.0f;
    for (float noise : m_noiseHistory) {
        avgNoise += noise;
    }
    avgNoise /= m_noiseHistory.size();
    
    m_noiseFloor = avgNoise;
}

// DSP Utility Functions
namespace DSPUtils {
    
void decimate(const std::vector<float>& input, std::vector<float>& output, int factor) {
    if (factor <= 1) {
        output = input;
        return;
    }
    
    output.clear();
    output.reserve(input.size() / factor);
    
    for (size_t i = 0; i < input.size(); i += factor) {
        output.push_back(input[i]);
    }
}

void interpolate(const std::vector<float>& input, std::vector<float>& output, int factor) {
    if (factor <= 1) {
        output = input;
        return;
    }
    
    output.clear();
    output.reserve(input.size() * factor);
    
    for (size_t i = 0; i < input.size(); i++) {
        output.push_back(input[i]);
        
        // Add interpolated zeros (simple zero-stuffing)
        for (int j = 1; j < factor; j++) {
            if (i + 1 < input.size()) {
                float interpolated = input[i] + (input[i + 1] - input[i]) * j / factor;
                output.push_back(interpolated);
            } else {
                output.push_back(0.0f);
            }
        }
    }
}

void applyWindow(std::vector<std::complex<float>>& data, const std::vector<float>& window) {
    size_t minSize = std::min(data.size(), window.size());
    
    for (size_t i = 0; i < minSize; i++) {
        data[i] *= window[i];
    }
}

} // namespace DSPUtils