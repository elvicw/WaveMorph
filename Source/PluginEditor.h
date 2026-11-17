#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "WaveformVisualizer.h"
#include "FileWaveformPreview.h"
#include "CustomLookAndFeel.h"

class WaveMorphAudioProcessorEditor : public juce::AudioProcessorEditor,
                                      public juce::Timer
{
public:
    WaveMorphAudioProcessorEditor(WaveMorphAudioProcessor&);
    ~WaveMorphAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

private:
    WaveMorphAudioProcessor& audioProcessor;
    CustomLookAndFeel customLookAndFeel;

    // Visualizers
    WaveformVisualizer liveVisualizer;
    FileWaveformPreview filePreview;

    // Header Controls
    juce::TextButton liveTabBtn { "LIVE STREAM" };
    juce::TextButton fileTabBtn { "FILE & RECORD PREVIEW" };
    bool showingLiveTab = true;

    juce::ComboBox sourceBox;
    juce::ComboBox viewModeBox;
    juce::ComboBox timeWindowBox;
    juce::ToggleButton freezeBtn { "FREEZE" };
    juce::ToggleButton bypassBtn { "BYPASS" };

    // Section 1: Compressor
    juce::GroupComponent compGroup { "compGroup", "DYNAMICS COMPRESSOR" };
    juce::Slider compThresholdSlider;
    juce::Slider compRatioSlider;
    juce::Slider compAttackSlider;
    juce::Slider compReleaseSlider;
    juce::Slider compMakeupSlider;
    juce::Slider compKneeSlider;
    juce::ToggleButton compEnableToggle { "Comp On" };

    juce::Label compThresholdLabel { {}, "Thresh" };
    juce::Label compRatioLabel { {}, "Ratio" };
    juce::Label compAttackLabel { {}, "Attack" };
    juce::Label compReleaseLabel { {}, "Release" };
    juce::Label compMakeupLabel { {}, "Makeup" };
    juce::Label compKneeLabel { {}, "Knee" };

    // Section 2: Equalizer
    juce::GroupComponent eqGroup { "eqGroup", "TONAL EQUALIZER" };
    juce::Slider eqLowSlider;
    juce::Slider eqMidSlider;
    juce::Slider eqMidQSlider;
    juce::Slider eqHighSlider;
    juce::ToggleButton eqEnableToggle { "EQ On" };

    juce::Label eqLowLabel { {}, "Low 80Hz" };
    juce::Label eqMidLabel { {}, "Mid 1kHz" };
    juce::Label eqMidQLabel { {}, "Mid Q" };
    juce::Label eqHighLabel { {}, "High 8kHz" };

    // Section 3: Clipper & Saturation
    juce::GroupComponent clipGroup { "clipGroup", "SATURATION & CLIPPER" };
    juce::Slider driveSlider;
    juce::Slider clipCeilingSlider;
    juce::ComboBox clipModeBox;
    juce::ToggleButton clipEnableToggle { "Clip On" };

    juce::Label driveLabel { {}, "Drive" };
    juce::Label clipCeilingLabel { {}, "Ceiling" };

    // APVTS Attachments
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    std::unique_ptr<SliderAttachment> compThresholdAtt;
    std::unique_ptr<SliderAttachment> compRatioAtt;
    std::unique_ptr<SliderAttachment> compAttackAtt;
    std::unique_ptr<SliderAttachment> compReleaseAtt;
    std::unique_ptr<SliderAttachment> compMakeupAtt;
    std::unique_ptr<SliderAttachment> compKneeAtt;
    std::unique_ptr<ButtonAttachment> compEnableAtt;

    std::unique_ptr<SliderAttachment> eqLowAtt;
    std::unique_ptr<SliderAttachment> eqMidAtt;
    std::unique_ptr<SliderAttachment> eqMidQAtt;
    std::unique_ptr<SliderAttachment> eqHighAtt;
    std::unique_ptr<ButtonAttachment> eqEnableAtt;

    std::unique_ptr<SliderAttachment> driveAtt;
    std::unique_ptr<SliderAttachment> clipCeilingAtt;
    std::unique_ptr<ComboBoxAttachment> clipModeAtt;
    std::unique_ptr<ButtonAttachment> clipEnableAtt;

    std::unique_ptr<ButtonAttachment> bypassAtt;
    std::unique_ptr<ComboBoxAttachment> sourceAtt;
    std::unique_ptr<ComboBoxAttachment> viewModeAtt;
    std::unique_ptr<ComboBoxAttachment> timeWindowAtt;
    std::unique_ptr<ButtonAttachment> freezeAtt;

    void setupRotaryKnob(juce::Slider& slider, juce::Label& label, const juce::String& suffix = "");

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WaveMorphAudioProcessorEditor)
};
