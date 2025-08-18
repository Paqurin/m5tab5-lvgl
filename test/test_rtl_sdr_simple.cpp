#include <unity.h>
#include "../src/dsp/signal_processing.h"
#include <cmath>

#ifdef ARDUINO
#include <Arduino.h>
#endif

/**
 * @file test_rtl_sdr_simple.cpp
 * @brief Simplified RTL-SDR component tests focusing on DSP functions
 */

void setUp(void) {
    // Set up test environment
}

void tearDown(void) {
    // Clean up after tests
}

// Test basic DSP utility functions
void test_dsp_power_conversion() {
    // Test power to dBm conversion
    float power = 1.0f;  // 1 watt
    float dbm = DSPUtils::powerToDBM(power);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 30.0f, dbm);  // 1W = 30 dBm
    
    // Test smaller power
    power = 0.001f;  // 1 milliwatt
    dbm = DSPUtils::powerToDBM(power);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 0.0f, dbm);  // 1mW = 0 dBm
}

void test_dsp_magnitude_conversion() {
    // Test magnitude to dB conversion
    float magnitude = 10.0f;
    float db = DSPUtils::magnitudeToDB(magnitude);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 20.0f, db);  // 20*log10(10) = 20 dB
    
    // Test unity magnitude
    magnitude = 1.0f;
    db = DSPUtils::magnitudeToDB(magnitude);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 0.0f, db);  // 20*log10(1) = 0 dB
}

void test_dsp_complex_magnitude() {
    // Test complex magnitude calculation
    std::complex<float> sample(3.0f, 4.0f);  // 3+4j
    float mag = DSPUtils::complexMagnitude(sample);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 5.0f, mag);  // sqrt(3²+4²) = 5
    
    // Test unit circle
    sample = std::complex<float>(0.707f, 0.707f);  // ~1∠45°
    mag = DSPUtils::complexMagnitude(sample);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, mag);
}

void test_dsp_decimation() {
    // Test decimation function
    std::vector<float> input = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    std::vector<float> output;
    
    DSPUtils::decimate(input, output, 2);
    
    TEST_ASSERT_EQUAL(5, output.size());
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, output[0]);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 3.0f, output[1]);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 5.0f, output[2]);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 7.0f, output[3]);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 9.0f, output[4]);
}

void test_dsp_decimation_factor_1() {
    // Test decimation with factor 1 (no decimation)
    std::vector<float> input = {1, 2, 3, 4, 5};
    std::vector<float> output;
    
    DSPUtils::decimate(input, output, 1);
    
    TEST_ASSERT_EQUAL(input.size(), output.size());
    for (size_t i = 0; i < input.size(); i++) {
        TEST_ASSERT_FLOAT_WITHIN(0.01f, input[i], output[i]);
    }
}

void test_fft_basic_functionality() {
    const size_t fftSize = 8;  // Very small FFT for simple testing
    FFTProcessor fft(fftSize);
    
    // Create simple DC signal
    std::vector<std::complex<float>> input(fftSize, std::complex<float>(1.0f, 0.0f));
    std::vector<std::complex<float>> output;
    
    fft.computeFFT(input, output);
    
    // Check that we got output of correct size
    TEST_ASSERT_EQUAL(fftSize, output.size());
    
    // DC component should be non-zero
    float dcMagnitude = std::abs(output[0]);
    TEST_ASSERT_GREATER_THAN(0.0f, dcMagnitude);
}

void test_fft_magnitude_spectrum() {
    const size_t fftSize = 16;
    FFTProcessor fft(fftSize);
    
    // Create constant signal
    std::vector<std::complex<float>> input(fftSize, std::complex<float>(1.0f, 0.0f));
    std::vector<float> magnitudes;
    
    fft.computeMagnitudeSpectrum(input, magnitudes);
    
    // Should get half the FFT size for positive frequencies
    TEST_ASSERT_EQUAL(fftSize / 2, magnitudes.size());
    
    // All values should be reasonable (not NaN or infinite)
    for (float mag : magnitudes) {
        TEST_ASSERT_TRUE(isfinite(mag));
        TEST_ASSERT_GREATER_THAN(-200.0f, mag);  // Not too negative in dB
    }
}

void test_digital_filter_creation() {
    const float sampleRate = 1000.0f;
    const float cutoffFreq = 100.0f;
    
    // Test that filter can be created without crashing
    DigitalFilter filter(DigitalFilter::LOW_PASS, cutoffFreq, sampleRate, 2);
    
    // Create simple test signal
    std::vector<float> input(10, 1.0f);
    std::vector<float> output;
    
    filter.filter(input, output);
    
    TEST_ASSERT_EQUAL(input.size(), output.size());
    
    // Output should not be all zeros
    bool hasNonZero = false;
    for (float val : output) {
        if (val != 0.0f) {
            hasNonZero = true;
            break;
        }
    }
    TEST_ASSERT_TRUE(hasNonZero);
}

void test_spectrum_analyzer_basic() {
    const size_t fftSize = 32;
    const float sampleRate = 1000.0f;
    
    SpectrumAnalyzer analyzer(fftSize, sampleRate);
    
    // Create simple test signal
    std::vector<std::complex<float>> samples(fftSize);
    for (size_t i = 0; i < fftSize; i++) {
        samples[i] = std::complex<float>(1.0f, 0.0f);  // DC signal
    }
    
    std::vector<float> spectrum;
    analyzer.analyzeSpectrum(samples, spectrum, 0.0f);
    
    TEST_ASSERT_EQUAL(fftSize / 2, spectrum.size());
    
    // Values should be finite
    for (float val : spectrum) {
        TEST_ASSERT_TRUE(isfinite(val));
    }
}

void test_audio_demodulator_creation() {
    const float sampleRate = 44100.0f;
    
    // Test creating different demodulator types
    AudioDemodulator amDemod(AudioDemodulator::AM, sampleRate);
    AudioDemodulator fmDemod(AudioDemodulator::FM, sampleRate);
    AudioDemodulator usbDemod(AudioDemodulator::USB, sampleRate);
    
    // Create simple test signal
    std::vector<std::complex<float>> input(100);
    for (size_t i = 0; i < input.size(); i++) {
        input[i] = std::complex<float>(sinf(2.0f * M_PI * i / 10.0f), 0.0f);
    }
    
    // Test demodulation doesn't crash
    std::vector<float> audio;
    amDemod.demodulate(input, audio);
    TEST_ASSERT_GREATER_OR_EQUAL(0, audio.size());
    
    fmDemod.demodulate(input, audio);
    TEST_ASSERT_GREATER_OR_EQUAL(0, audio.size());
    
    usbDemod.demodulate(input, audio);
    TEST_ASSERT_GREATER_OR_EQUAL(0, audio.size());
}

void test_bandpass_filter() {
    const float sampleRate = 1000.0f;
    const float lowFreq = 100.0f;
    const float highFreq = 200.0f;
    
    DigitalFilter filter(DigitalFilter::BAND_PASS, lowFreq, highFreq, sampleRate, 2);
    
    std::vector<float> input(50, 1.0f);  // Step input
    std::vector<float> output;
    
    filter.filter(input, output);
    
    TEST_ASSERT_EQUAL(input.size(), output.size());
    
    // Filter should produce some output
    float maxOutput = 0.0f;
    for (float val : output) {
        maxOutput = std::max(maxOutput, std::abs(val));
    }
    TEST_ASSERT_GREATER_THAN(0.0f, maxOutput);
}

void test_interpolation() {
    std::vector<float> input = {1, 2, 3, 4};
    std::vector<float> output;
    
    DSPUtils::interpolate(input, output, 2);
    
    // Should be roughly double the size
    TEST_ASSERT_GREATER_OR_EQUAL(input.size() * 2 - 1, output.size());
    
    // First element should be preserved
    TEST_ASSERT_FLOAT_WITHIN(0.01f, input[0], output[0]);
}

void test_window_application() {
    std::vector<std::complex<float>> data = {
        {1.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 0.0f}
    };
    std::vector<float> window = {1.0f, 0.5f, 0.5f, 1.0f};
    
    DSPUtils::applyWindow(data, window);
    
    // Check that window was applied
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, data[0].real());
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.5f, data[1].real());
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.5f, data[2].real());
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, data[3].real());
}

int runSimpleTests(void) {
    UNITY_BEGIN();
    
    // DSP Utility Tests
    RUN_TEST(test_dsp_power_conversion);
    RUN_TEST(test_dsp_magnitude_conversion);
    RUN_TEST(test_dsp_complex_magnitude);
    RUN_TEST(test_dsp_decimation);
    RUN_TEST(test_dsp_decimation_factor_1);
    RUN_TEST(test_interpolation);
    RUN_TEST(test_window_application);
    
    // DSP Component Tests
    RUN_TEST(test_fft_basic_functionality);
    RUN_TEST(test_fft_magnitude_spectrum);
    RUN_TEST(test_digital_filter_creation);
    RUN_TEST(test_bandpass_filter);
    RUN_TEST(test_spectrum_analyzer_basic);
    RUN_TEST(test_audio_demodulator_creation);
    
    return UNITY_END();
}

#ifdef ARDUINO
void setup() {
    delay(2000);  // Wait for serial connection
    runSimpleTests();
}

void loop() {
    // Empty - tests run once in setup
}
#else
int main() {
    return runSimpleTests();
}
#endif