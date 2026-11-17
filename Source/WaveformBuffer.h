#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <atomic>
#include <vector>
#include <cmath>
#include <algorithm>

class WaveformBuffer
{
public:
    static constexpr int BUFFER_SIZE = 524288; // ~11 seconds at 48kHz, power of 2 for fast bitmask

    WaveformBuffer()
        : bufferMask(BUFFER_SIZE - 1)
    {
        beforeBuffer.assign(BUFFER_SIZE, 0.0f);
        afterBuffer.assign(BUFFER_SIZE, 0.0f);
        grBuffer.assign(BUFFER_SIZE, 0.0f);
        frozenBefore.assign(16384, 0.0f);
        frozenAfter.assign(16384, 0.0f);
        frozenGR.assign(16384, 0.0f);
    }

    void reset()
    {
        writeIndex.store(0, std::memory_order_relaxed);
        std::fill(beforeBuffer.begin(), beforeBuffer.end(), 0.0f);
        std::fill(afterBuffer.begin(), afterBuffer.end(), 0.0f);
        std::fill(grBuffer.begin(), grBuffer.end(), 0.0f);
    }

    // Audio thread pushes stereo or mono samples
    inline void pushSamples(const float* beforeL, const float* beforeR,
                            const float* afterL, const float* afterR,
                            float grDb, int numSamples)
    {
        int w = writeIndex.load(std::memory_order_relaxed);

        for (int i = 0; i < numSamples; ++i)
        {
            float b = beforeL[i];
            if (beforeR != nullptr)
                b = 0.5f * (b + beforeR[i]);

            float a = afterL[i];
            if (afterR != nullptr)
                a = 0.5f * (a + afterR[i]);

            int idx = (w + i) & bufferMask;
            beforeBuffer[idx] = b;
            afterBuffer[idx] = a;
            grBuffer[idx] = grDb;
        }

        writeIndex.store((w + numSamples) & bufferMask, std::memory_order_release);
    }

    void setFrozen(bool freeze)
    {
        if (freeze && !isFrozen.load(std::memory_order_relaxed))
        {
            // Snapshot current latest 16384 samples
            int count = (int)frozenBefore.size();
            copyLatestSamples(frozenBefore.data(), frozenAfter.data(), frozenGR.data(), count, false);
            isFrozen.store(true, std::memory_order_release);
        }
        else if (!freeze)
        {
            isFrozen.store(false, std::memory_order_release);
        }
    }

    bool getIsFrozen() const
    {
        return isFrozen.load(std::memory_order_relaxed);
    }

    // GUI thread copies latest samples into display array
    void copyLatestSamples(float* destBefore, float* destAfter, float* destGR,
                           int numSamplesToRead, bool triggered = false, float triggerLevel = 0.05f)
    {
        if (isFrozen.load(std::memory_order_relaxed))
        {
            int toCopy = std::min(numSamplesToRead, (int)frozenBefore.size());
            std::copy(frozenBefore.data(), frozenBefore.data() + toCopy, destBefore);
            std::copy(frozenAfter.data(), frozenAfter.data() + toCopy, destAfter);
            if (destGR)
                std::copy(frozenGR.data(), frozenGR.data() + toCopy, destGR);
            return;
        }

        int w = writeIndex.load(std::memory_order_acquire);
        int readEnd = w;

        if (triggered && numSamplesToRead < BUFFER_SIZE / 2)
        {
            // Search back for positive-going zero crossing or rising edge
            int searchWindow = std::min(numSamplesToRead, 4096);
            int triggerIdx = -1;

            for (int s = 0; s < searchWindow; ++s)
            {
                int curr = (w - s - 1) & bufferMask;
                int prev = (w - s - 2) & bufferMask;
                if (beforeBuffer[prev] < triggerLevel && beforeBuffer[curr] >= triggerLevel)
                {
                    triggerIdx = curr;
                    break;
                }
            }

            if (triggerIdx != -1)
            {
                // Align so trigger point is at roughly 10% into display window
                int offset = numSamplesToRead / 10;
                readEnd = (triggerIdx + numSamplesToRead - offset) & bufferMask;
            }
        }

        int startIdx = (readEnd - numSamplesToRead) & bufferMask;
        for (int i = 0; i < numSamplesToRead; ++i)
        {
            int idx = (startIdx + i) & bufferMask;
            destBefore[i] = beforeBuffer[idx];
            destAfter[i] = afterBuffer[idx];
            if (destGR)
                destGR[i] = grBuffer[idx];
        }
    }

private:
    const int bufferMask;
    std::vector<float> beforeBuffer;
    std::vector<float> afterBuffer;
    std::vector<float> grBuffer;
    std::atomic<int> writeIndex { 0 };

    std::atomic<bool> isFrozen { false };
    std::vector<float> frozenBefore;
    std::vector<float> frozenAfter;
    std::vector<float> frozenGR;
};
