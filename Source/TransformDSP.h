#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <cmath>

class BiquadFilter
{
public:
    BiquadFilter() = default;

    void reset()
    {
        z1 = 0.0f;
        z2 = 0.0f;
    }

    void setLowShelf(float sampleRate, float frequency, float gainDb)
    {
        float A = std::pow(10.0f, gainDb / 40.0f);
        float w0 = 2.0f * juce::MathConstants<float>::pi * frequency / sampleRate;
        float cosw0 = std::cos(w0);
        float sinw0 = std::sin(w0);
        float S = 1.0f;
        float alpha = sinw0 / 2.0f * std::sqrt((A + 1.0f / A) * (1.0f / S - 1.0f) + 2.0f);
        float temp = 2.0f * std::sqrt(A) * alpha;

        float b0_raw =    A * ((A + 1.0f) - (A - 1.0f) * cosw0 + temp);
        float b1_raw = 2.0f * A * ((A - 1.0f) - (A + 1.0f) * cosw0);
        float b2_raw =    A * ((A + 1.0f) - (A - 1.0f) * cosw0 - temp);
        float a0_raw =         (A + 1.0f) + (A - 1.0f) * cosw0 + temp;
        float a1_raw = -2.0f * ((A - 1.0f) + (A + 1.0f) * cosw0);
        float a2_raw =         (A + 1.0f) + (A - 1.0f) * cosw0 - temp;

        b0 = b0_raw / a0_raw;
        b1 = b1_raw / a0_raw;
        b2 = b2_raw / a0_raw;
        a1 = a1_raw / a0_raw;
        a2 = a2_raw / a0_raw;
    }

    void setPeaking(float sampleRate, float frequency, float q, float gainDb)
    {
        float A = std::pow(10.0f, gainDb / 40.0f);
        float w0 = 2.0f * juce::MathConstants<float>::pi * frequency / sampleRate;
        float cosw0 = std::cos(w0);
        float sinw0 = std::sin(w0);
        float alpha = sinw0 / (2.0f * juce::jmax(0.1f, q));

        float b0_raw = 1.0f + alpha * A;
        float b1_raw = -2.0f * cosw0;
        float b2_raw = 1.0f - alpha * A;
        float a0_raw = 1.0f + alpha / A;
        float a1_raw = -2.0f * cosw0;
        float a2_raw = 1.0f - alpha / A;

        b0 = b0_raw / a0_raw;
        b1 = b1_raw / a0_raw;
        b2 = b2_raw / a0_raw;
        a1 = a1_raw / a0_raw;
        a2 = a2_raw / a0_raw;
    }

    void setHighShelf(float sampleRate, float frequency, float gainDb)
    {
        float A = std::pow(10.0f, gainDb / 40.0f);
        float w0 = 2.0f * juce::MathConstants<float>::pi * frequency / sampleRate;
        float cosw0 = std::cos(w0);
        float sinw0 = std::sin(w0);
        float S = 1.0f;
        float alpha = sinw0 / 2.0f * std::sqrt((A + 1.0f / A) * (1.0f / S - 1.0f) + 2.0f);
        float temp = 2.0f * std::sqrt(A) * alpha;

        float b0_raw =    A * ((A + 1.0f) + (A - 1.0f) * cosw0 + temp);
        float b1_raw = -2.0f * A * ((A - 1.0f) + (A + 1.0f) * cosw0);
        float b2_raw =    A * ((A + 1.0f) + (A - 1.0f) * cosw0 - temp);
        float a0_raw =         (A + 1.0f) - (A - 1.0f) * cosw0 + temp;
        float a1_raw =  2.0f * ((A - 1.0f) - (A + 1.0f) * cosw0);
        float a2_raw =         (A + 1.0f) - (A - 1.0f) * cosw0 - temp;

        b0 = b0_raw / a0_raw;
        b1 = b1_raw / a0_raw;
        b2 = b2_raw / a0_raw;
        a1 = a1_raw / a0_raw;
        a2 = a2_raw / a0_raw;
    }

    inline float processSample(float in)
    {
        // Direct Form II Transposed
        float out = b0 * in + z1;
        z1 = b1 * in - a1 * out + z2;
        z2 = b2 * in - a2 * out;
        return out;
    }

private:
    float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f;
    float a1 = 0.0f, a2 = 0.0f;
    float z1 = 0.0f, z2 = 0.0f;
};

struct DSPParameters
{
    // Compressor
    float compThresholdDb = -18.0f; // -60 to 0 dB
    float compRatio = 4.0f;         // 1 to 30
    float compAttackMs = 15.0f;     // 0.1 to 100 ms
    float compReleaseMs = 120.0f;   // 10 to 1000 ms
    float compMakeupDb = 0.0f;      // 0 to 24 dB
    float compKneeDb = 6.0f;        // 0 to 18 dB
    bool compEnabled = true;

    // EQ
    float eqLowGainDb = 0.0f;       // -18 to +18 dB (80 Hz shelf)
    float eqMidGainDb = 0.0f;       // -18 to +18 dB (1 kHz peak)
    float eqMidQ = 1.0f;            // 0.5 to 5.0
    float eqHighGainDb = 0.0f;      // -18 to +18 dB (8 kHz shelf)
    bool eqEnabled = true;

    // Clipper & Saturation
    float driveDb = 0.0f;           // 0 to +24 dB
    float clipCeilingDb = -0.5f;    // -12 to 0 dB
    int clipMode = 0;               // 0 = Soft (Analog/Tube), 1 = Hard (Digital)
    bool clipperEnabled = true;

    // Global
    bool bypass = false;
};

class TransformDSP
{
public:
    TransformDSP();
    ~TransformDSP() = default;

    void prepare(double sampleRate, int samplesPerBlock, int numChannels);
    void reset();

    void setParameters(const DSPParameters& params);
    const DSPParameters& getParameters() const { return currentParams; }

    // Real-time block processing
    // dryBuffer contains the original un-processed audio.
    // wetBuffer contains the processed audio output.
    // grBuffer (if not null) stores gain reduction values in linear scale [0.0 to 1.0].
    void processBlock(const juce::AudioBuffer<float>& inputBuffer,
                      juce::AudioBuffer<float>& outputBuffer,
                      float* outMaxGainReduction = nullptr);

    // Single sample processing for channel
    float processSample(int channel, float inSample, float& outGainReduction);

    // Offline buffer processor for audio file previewing
    static void processOfflineBuffer(const juce::AudioBuffer<float>& inBuffer,
                                     juce::AudioBuffer<float>& outBuffer,
                                     double sampleRate,
                                     const DSPParameters& params,
                                     float& peakInDb, float& peakOutDb,
                                     float& rmsInDb, float& rmsOutDb,
                                     float& crestInDb, float& crestOutDb,
                                     int& clippedSamplesCount);

    float getCurrentGainReductionDb() const { return currentGainReductionDb.load(); }

private:
    double currentSampleRate = 44100.0;
    int numChannels = 2;
    DSPParameters currentParams;

    // Compressor state
    std::vector<float> envelopeDetector;
    float attackCoeff = 0.0f;
    float releaseCoeff = 0.0f;
    std::atomic<float> currentGainReductionDb { 0.0f };

    // EQ filters (per channel: Low, Mid, High)
    std::vector<BiquadFilter> lowShelfFilters;
    std::vector<BiquadFilter> midPeakFilters;
    std::vector<BiquadFilter> highShelfFilters;

    void updateFilterCoefficients();
    void updateBallistics();
};
