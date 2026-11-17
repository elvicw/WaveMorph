#include "TransformDSP.h"
#include <cmath>
#include <algorithm>

TransformDSP::TransformDSP()
{
    envelopeDetector.resize(2, 0.0f);
    lowShelfFilters.resize(2);
    midPeakFilters.resize(2);
    highShelfFilters.resize(2);
    updateBallistics();
}

void TransformDSP::prepare(double sampleRate, int /*samplesPerBlock*/, int channels)
{
    currentSampleRate = sampleRate > 0.0 ? sampleRate : 44100.0;
    numChannels = std::max(1, channels);

    envelopeDetector.assign(numChannels, 0.0f);
    lowShelfFilters.resize(numChannels);
    midPeakFilters.resize(numChannels);
    highShelfFilters.resize(numChannels);

    reset();
    updateBallistics();
    updateFilterCoefficients();
}

void TransformDSP::reset()
{
    for (auto& env : envelopeDetector)
        env = 0.0f;

    for (auto& f : lowShelfFilters)
        f.reset();
    for (auto& f : midPeakFilters)
        f.reset();
    for (auto& f : highShelfFilters)
        f.reset();

    currentGainReductionDb.store(0.0f);
}

void TransformDSP::setParameters(const DSPParameters& params)
{
    bool needFilterUpdate = (params.eqLowGainDb != currentParams.eqLowGainDb ||
                             params.eqMidGainDb != currentParams.eqMidGainDb ||
                             params.eqMidQ != currentParams.eqMidQ ||
                             params.eqHighGainDb != currentParams.eqHighGainDb);

    bool needBallisticsUpdate = (params.compAttackMs != currentParams.compAttackMs ||
                                 params.compReleaseMs != currentParams.compReleaseMs);

    currentParams = params;

    if (needFilterUpdate)
        updateFilterCoefficients();

    if (needBallisticsUpdate)
        updateBallistics();
}

void TransformDSP::updateBallistics()
{
    float attackSec = std::max(0.0001f, currentParams.compAttackMs * 0.001f);
    float releaseSec = std::max(0.001f, currentParams.compReleaseMs * 0.001f);

    attackCoeff = std::exp(-1.0f / (float(currentSampleRate) * attackSec));
    releaseCoeff = std::exp(-1.0f / (float(currentSampleRate) * releaseSec));
}

void TransformDSP::updateFilterCoefficients()
{
    float sr = float(currentSampleRate);
    for (int ch = 0; ch < numChannels; ++ch)
    {
        lowShelfFilters[ch].setLowShelf(sr, 80.0f, currentParams.eqLowGainDb);
        midPeakFilters[ch].setPeaking(sr, 1000.0f, currentParams.eqMidQ, currentParams.eqMidGainDb);
        highShelfFilters[ch].setHighShelf(sr, 8000.0f, currentParams.eqHighGainDb);
    }
}

float TransformDSP::processSample(int channel, float inSample, float& outGainReduction)
{
    if (currentParams.bypass)
    {
        outGainReduction = 0.0f;
        return inSample;
    }

    float sample = inSample;

    // 1. Equalizer Stage
    if (currentParams.eqEnabled && channel < (int)lowShelfFilters.size())
    {
        sample = lowShelfFilters[channel].processSample(sample);
        sample = midPeakFilters[channel].processSample(sample);
        sample = highShelfFilters[channel].processSample(sample);
    }

    // 2. Compressor Stage
    outGainReduction = 0.0f;
    if (currentParams.compEnabled && channel < (int)envelopeDetector.size())
    {
        float absSample = std::fabs(sample);
        float& env = envelopeDetector[channel];

        // Ballistics envelope detection
        if (absSample > env)
            env = attackCoeff * env + (1.0f - attackCoeff) * absSample;
        else
            env = releaseCoeff * env + (1.0f - releaseCoeff) * absSample;

        // Level in dB
        float levelDb = 20.0f * std::log10(std::max(env, 1e-5f));
        float threshold = currentParams.compThresholdDb;
        float ratio = std::max(1.0f, currentParams.compRatio);
        float knee = std::max(0.01f, currentParams.compKneeDb);
        float halfKnee = knee * 0.5f;

        float targetGainDb = 0.0f;

        if (levelDb <= threshold - halfKnee)
        {
            // Below knee: no compression
            targetGainDb = 0.0f;
        }
        else if (levelDb >= threshold + halfKnee)
        {
            // Above knee: full ratio compression
            targetGainDb = (threshold + (levelDb - threshold) / ratio) - levelDb;
        }
        else
        {
            // Inside knee quadratic curve
            float delta = levelDb - threshold + halfKnee;
            float kneeFactor = (1.0f / ratio - 1.0f) * (delta * delta) / (2.0f * knee);
            targetGainDb = kneeFactor;
        }

        // targetGainDb is <= 0 (gain reduction)
        float grDb = -targetGainDb; // positive dB reduction
        outGainReduction = grDb;

        float makeupLinear = std::pow(10.0f, currentParams.compMakeupDb / 20.0f);
        float compGainLinear = std::pow(10.0f, targetGainDb / 20.0f) * makeupLinear;
        sample *= compGainLinear;
    }

    // 3. Drive & Clipper / Saturation Stage
    if (currentParams.clipperEnabled)
    {
        float driveLinear = std::pow(10.0f, currentParams.driveDb / 20.0f);
        sample *= driveLinear;

        float ceilingLinear = std::pow(10.0f, currentParams.clipCeilingDb / 20.0f);

        if (currentParams.clipMode == 0) // Soft Saturation
        {
            // Smooth soft clipping using tanh
            float normalized = sample / std::max(1e-4f, ceilingLinear);
            sample = ceilingLinear * std::tanh(normalized);
        }
        else // Hard Digital Ceiling
        {
            sample = juce::jlimit(-ceilingLinear, ceilingLinear, sample);
        }
    }

    return sample;
}

void TransformDSP::processBlock(const juce::AudioBuffer<float>& inputBuffer,
                                juce::AudioBuffer<float>& outputBuffer,
                                float* outMaxGainReduction)
{
    int numSamples = inputBuffer.getNumSamples();
    int channels = std::min(inputBuffer.getNumChannels(), outputBuffer.getNumChannels());

    float maxGR = 0.0f;

    for (int ch = 0; ch < channels; ++ch)
    {
        const float* in = inputBuffer.getReadPointer(ch);
        float* out = outputBuffer.getWritePointer(ch);

        for (int i = 0; i < numSamples; ++i)
        {
            float grDb = 0.0f;
            out[i] = processSample(ch, in[i], grDb);
            if (grDb > maxGR)
                maxGR = grDb;
        }
    }

    currentGainReductionDb.store(maxGR);
    if (outMaxGainReduction != nullptr)
        *outMaxGainReduction = maxGR;
}

void TransformDSP::processOfflineBuffer(const juce::AudioBuffer<float>& inBuffer,
                                        juce::AudioBuffer<float>& outBuffer,
                                        double sampleRate,
                                        const DSPParameters& params,
                                        float& peakInDb, float& peakOutDb,
                                        float& rmsInDb, float& rmsOutDb,
                                        float& crestInDb, float& crestOutDb,
                                        int& clippedSamplesCount)
{
    int numSamples = inBuffer.getNumSamples();
    int channels = std::min(inBuffer.getNumChannels(), outBuffer.getNumChannels());

    TransformDSP offlineDsp;
    offlineDsp.prepare(sampleRate, 512, channels);
    offlineDsp.setParameters(params);

    float maxIn = 1e-6f;
    float maxOut = 1e-6f;
    double sumSqIn = 0.0;
    double sumSqOut = 0.0;
    int clippedCount = 0;

    float ceiling = std::pow(10.0f, params.clipCeilingDb / 20.0f);

    for (int ch = 0; ch < channels; ++ch)
    {
        const float* in = inBuffer.getReadPointer(ch);
        float* out = outBuffer.getWritePointer(ch);

        for (int i = 0; i < numSamples; ++i)
        {
            float x = in[i];
            float absX = std::fabs(x);
            if (absX > maxIn) maxIn = absX;
            sumSqIn += double(x * x);

            float dummyGR = 0.0f;
            float y = offlineDsp.processSample(ch, x, dummyGR);
            out[i] = y;

            float absY = std::fabs(y);
            if (absY > maxOut) maxOut = absY;
            sumSqOut += double(y * y);

            if (absY >= ceiling * 0.999f)
                clippedCount++;
        }
    }

    double totalSamples = double(numSamples * channels);
    if (totalSamples < 1.0) totalSamples = 1.0;

    float rmsIn = float(std::sqrt(sumSqIn / totalSamples));
    float rmsOut = float(std::sqrt(sumSqOut / totalSamples));

    peakInDb = 20.0f * std::log10(std::max(1e-5f, maxIn));
    peakOutDb = 20.0f * std::log10(std::max(1e-5f, maxOut));

    rmsInDb = 20.0f * std::log10(std::max(1e-5f, rmsIn));
    rmsOutDb = 20.0f * std::log10(std::max(1e-5f, rmsOut));

    crestInDb = peakInDb - rmsInDb;
    crestOutDb = peakOutDb - rmsOutDb;

    clippedSamplesCount = clippedCount;
}
