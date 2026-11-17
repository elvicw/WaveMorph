#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "WaveformBuffer.h"

class WaveformVisualizer : public juce::Component, public juce::Timer
{
public:
    enum ViewMode
    {
        Overlay = 0,
        Split,
        Delta
    };

    WaveformVisualizer(WaveformBuffer& buffer);
    ~WaveformVisualizer() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

    void setViewMode(ViewMode mode);
    void setTimeWindowMs(float ms);
    void setTriggered(bool triggered);
    void setFrozen(bool frozen);

    void setSampleRate(double sr) { sampleRate = sr > 0 ? sr : 44100.0; }

private:
    WaveformBuffer& waveformBuffer;
    double sampleRate = 44100.0;
    float timeWindowMs = 250.0f;
    ViewMode viewMode = ViewMode::Overlay;
    bool isTriggered = true;
    bool isFrozen = false;

    // Display cache
    std::vector<float> beforeDisplay;
    std::vector<float> afterDisplay;
    std::vector<float> grDisplay;

    // Smoothed meter values for live HUD
    float peakBeforeDb = -100.0f;
    float peakAfterDb = -100.0f;
    float rmsBeforeDb = -100.0f;
    float rmsAfterDb = -100.0f;
    float currentGRDb = 0.0f;

    void drawGrid(juce::Graphics& g, juce::Rectangle<float> bounds);
    void drawWaveformOverlay(juce::Graphics& g, juce::Rectangle<float> bounds, int numPoints);
    void drawWaveformSplit(juce::Graphics& g, juce::Rectangle<float> bounds, int numPoints);
    void drawWaveformDelta(juce::Graphics& g, juce::Rectangle<float> bounds, int numPoints);
    void drawHUD(juce::Graphics& g, juce::Rectangle<float> bounds);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WaveformVisualizer)
};
