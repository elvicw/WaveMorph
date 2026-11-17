#include "PluginProcessor.h"
#include "PluginEditor.h"

WaveMorphAudioProcessorEditor::WaveMorphAudioProcessorEditor(WaveMorphAudioProcessor& p)
    : AudioProcessorEditor(&p),
      audioProcessor(p),
      liveVisualizer(p.getWaveformBuffer()),
      filePreview(p.getTransformDSP())
{
    setLookAndFeel(&customLookAndFeel);

    audioProcessor.setFilePreviewPtr(&filePreview);
    liveVisualizer.setSampleRate(audioProcessor.getSampleRate());

    addAndMakeVisible(liveVisualizer);
    addChildComponent(filePreview);

    // Tab buttons
    addAndMakeVisible(liveTabBtn);
    addAndMakeVisible(fileTabBtn);

    auto updateTabs = [this]()
    {
        liveVisualizer.setVisible(showingLiveTab);
        filePreview.setVisible(!showingLiveTab);

        liveTabBtn.setColour(juce::TextButton::buttonColourId, showingLiveTab ? juce::Colour(0xff2a324b) : juce::Colour(0xff161922));
        liveTabBtn.setColour(juce::TextButton::textColourOffId, showingLiveTab ? juce::Colour(0xff00e5ff) : juce::Colour(0xff8c96b3));

        fileTabBtn.setColour(juce::TextButton::buttonColourId, !showingLiveTab ? juce::Colour(0xff2a324b) : juce::Colour(0xff161922));
        fileTabBtn.setColour(juce::TextButton::textColourOffId, !showingLiveTab ? juce::Colour(0xffffb300) : juce::Colour(0xff8c96b3));
    };

    liveTabBtn.onClick = [this, updateTabs]()
    {
        showingLiveTab = true;
        updateTabs();
    };

    fileTabBtn.onClick = [this, updateTabs]()
    {
        showingLiveTab = false;
        updateTabs();
        filePreview.triggerReprocess();
    };

    updateTabs();

    // Header combo boxes
    addAndMakeVisible(sourceBox);
    sourceBox.addItem("Internal DSP Preview", 1);
    sourceBox.addItem("External DAW Sidechain", 2);

    addAndMakeVisible(viewModeBox);
    viewModeBox.addItem("Overlay View", 1);
    viewModeBox.addItem("Split View", 2);
    viewModeBox.addItem("Delta View", 3);
    viewModeBox.onChange = [this]()
    {
        liveVisualizer.setViewMode((WaveformVisualizer::ViewMode)(viewModeBox.getSelectedItemIndex()));
    };

    addAndMakeVisible(timeWindowBox);
    timeWindowBox.addItem("50 ms", 1);
    timeWindowBox.addItem("100 ms", 2);
    timeWindowBox.addItem("250 ms", 3);
    timeWindowBox.addItem("500 ms", 4);
    timeWindowBox.addItem("1000 ms", 5);
    timeWindowBox.addItem("2000 ms", 6);
    timeWindowBox.onChange = [this]()
    {
        const float times[] = { 50.0f, 100.0f, 250.0f, 500.0f, 1000.0f, 2000.0f };
        int idx = timeWindowBox.getSelectedItemIndex();
        if (idx >= 0 && idx < 6)
            liveVisualizer.setTimeWindowMs(times[idx]);
    };

    addAndMakeVisible(freezeBtn);
    freezeBtn.setColour(juce::ToggleButton::textColourId, juce::Colour(0xffe085ff));
    freezeBtn.onClick = [this]()
    {
        liveVisualizer.setFrozen(freezeBtn.getToggleState());
    };

    addAndMakeVisible(bypassBtn);
    bypassBtn.setColour(juce::ToggleButton::textColourId, juce::Colour(0xffff5252));

    // Groups
    addAndMakeVisible(compGroup);
    compGroup.setColour(juce::GroupComponent::outlineColourId, juce::Colour(0xff293042));
    compGroup.setColour(juce::GroupComponent::textColourId, juce::Colour(0xff8ca0c7));

    addAndMakeVisible(eqGroup);
    eqGroup.setColour(juce::GroupComponent::outlineColourId, juce::Colour(0xff293042));
    eqGroup.setColour(juce::GroupComponent::textColourId, juce::Colour(0xff8ca0c7));

    addAndMakeVisible(clipGroup);
    clipGroup.setColour(juce::GroupComponent::outlineColourId, juce::Colour(0xff293042));
    clipGroup.setColour(juce::GroupComponent::textColourId, juce::Colour(0xff8ca0c7));

    // Compressor Controls
    setupRotaryKnob(compThresholdSlider, compThresholdLabel, " dB");
    setupRotaryKnob(compRatioSlider, compRatioLabel, ":1");
    setupRotaryKnob(compAttackSlider, compAttackLabel, " ms");
    setupRotaryKnob(compReleaseSlider, compReleaseLabel, " ms");
    setupRotaryKnob(compMakeupSlider, compMakeupLabel, " dB");
    setupRotaryKnob(compKneeSlider, compKneeLabel, " dB");
    addAndMakeVisible(compEnableToggle);

    // EQ Controls
    setupRotaryKnob(eqLowSlider, eqLowLabel, " dB");
    setupRotaryKnob(eqMidSlider, eqMidLabel, " dB");
    setupRotaryKnob(eqMidQSlider, eqMidQLabel, "");
    setupRotaryKnob(eqHighSlider, eqHighLabel, " dB");
    addAndMakeVisible(eqEnableToggle);

    // Clipper Controls
    setupRotaryKnob(driveSlider, driveLabel, " dB");
    setupRotaryKnob(clipCeilingSlider, clipCeilingLabel, " dB");
    addAndMakeVisible(clipModeBox);
    clipModeBox.addItem("Soft Saturation", 1);
    clipModeBox.addItem("Hard Ceiling", 2);
    addAndMakeVisible(clipEnableToggle);

    // Attachments
    auto& apvts = audioProcessor.getAPVTS();

    compThresholdAtt = std::make_unique<SliderAttachment>(apvts, "compThreshold", compThresholdSlider);
    compRatioAtt = std::make_unique<SliderAttachment>(apvts, "compRatio", compRatioSlider);
    compAttackAtt = std::make_unique<SliderAttachment>(apvts, "compAttack", compAttackSlider);
    compReleaseAtt = std::make_unique<SliderAttachment>(apvts, "compRelease", compReleaseSlider);
    compMakeupAtt = std::make_unique<SliderAttachment>(apvts, "compMakeup", compMakeupSlider);
    compKneeAtt = std::make_unique<SliderAttachment>(apvts, "compKnee", compKneeSlider);
    compEnableAtt = std::make_unique<ButtonAttachment>(apvts, "compEnabled", compEnableToggle);

    eqLowAtt = std::make_unique<SliderAttachment>(apvts, "eqLowGain", eqLowSlider);
    eqMidAtt = std::make_unique<SliderAttachment>(apvts, "eqMidGain", eqMidSlider);
    eqMidQAtt = std::make_unique<SliderAttachment>(apvts, "eqMidQ", eqMidQSlider);
    eqHighAtt = std::make_unique<SliderAttachment>(apvts, "eqHighGain", eqHighSlider);
    eqEnableAtt = std::make_unique<ButtonAttachment>(apvts, "eqEnabled", eqEnableToggle);

    driveAtt = std::make_unique<SliderAttachment>(apvts, "drive", driveSlider);
    clipCeilingAtt = std::make_unique<SliderAttachment>(apvts, "clipCeiling", clipCeilingSlider);
    clipModeAtt = std::make_unique<ComboBoxAttachment>(apvts, "clipMode", clipModeBox);
    clipEnableAtt = std::make_unique<ButtonAttachment>(apvts, "clipEnabled", clipEnableToggle);

    bypassAtt = std::make_unique<ButtonAttachment>(apvts, "dspBypass", bypassBtn);
    sourceAtt = std::make_unique<ComboBoxAttachment>(apvts, "inputSource", sourceBox);
    viewModeAtt = std::make_unique<ComboBoxAttachment>(apvts, "viewMode", viewModeBox);
    timeWindowAtt = std::make_unique<ComboBoxAttachment>(apvts, "timeWindow", timeWindowBox);
    freezeAtt = std::make_unique<ButtonAttachment>(apvts, "freeze", freezeBtn);

    // Reprocess file preview whenever sliders move
    auto onSliderChange = [this]()
    {
        if (!showingLiveTab)
            filePreview.triggerReprocess();
    };

    compThresholdSlider.onValueChange = onSliderChange;
    compRatioSlider.onValueChange = onSliderChange;
    compAttackSlider.onValueChange = onSliderChange;
    compReleaseSlider.onValueChange = onSliderChange;
    compMakeupSlider.onValueChange = onSliderChange;
    compKneeSlider.onValueChange = onSliderChange;

    eqLowSlider.onValueChange = onSliderChange;
    eqMidSlider.onValueChange = onSliderChange;
    eqMidQSlider.onValueChange = onSliderChange;
    eqHighSlider.onValueChange = onSliderChange;

    driveSlider.onValueChange = onSliderChange;
    clipCeilingSlider.onValueChange = onSliderChange;
    clipModeBox.onChange = onSliderChange;

    setResizable(true, true);
    setResizeLimits(860, 620, 1400, 1000);
    setSize(980, 700);

    startTimerHz(20);
}

WaveMorphAudioProcessorEditor::~WaveMorphAudioProcessorEditor()
{
    stopTimer();
    audioProcessor.setFilePreviewPtr(nullptr);
    setLookAndFeel(nullptr);
}

void WaveMorphAudioProcessorEditor::timerCallback()
{
    // Sync freeze state
    liveVisualizer.setFrozen(freezeBtn.getToggleState());
}

void WaveMorphAudioProcessorEditor::setupRotaryKnob(juce::Slider& slider, juce::Label& label, const juce::String& suffix)
{
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 55, 16);
    slider.setTextValueSuffix(suffix);
    addAndMakeVisible(slider);

    label.setFont(juce::Font(11.0f, juce::Font::plain));
    label.setJustificationType(juce::Justification::centred);
    label.attachToComponent(&slider, false);
}

void WaveMorphAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff12141c));

    // Header Title
    g.setFont(juce::Font(18.0f, juce::Font::bold));
    g.setColour(juce::Colour(0xff00e5ff));
    g.drawText("WAVEMORPH", 18.0f, 10.0f, 130.0f, 22.0f, juce::Justification::left);

    g.setFont(juce::Font(10.0f, juce::Font::plain));
    g.setColour(juce::Colour(0xff707d9e));
    g.drawText("WAVEFORM TRANSFORMATION PREVIEWER", 18.0f, 28.0f, 220.0f, 14.0f, juce::Justification::left);

    // Subtle header bottom divider
    g.setColour(juce::Colour(0xff222736));
    g.drawHorizontalLine(48, 0.0f, (float)getWidth());
}

void WaveMorphAudioProcessorEditor::resized()
{
    auto area = getLocalBounds();

    // Top Header Bar
    auto header = area.removeFromTop(48);
    header.removeFromLeft(240); // Leave room for title

    liveTabBtn.setBounds(header.removeFromLeft(120).reduced(4, 8));
    fileTabBtn.setBounds(header.removeFromLeft(180).reduced(4, 8));
    header.removeFromLeft(10);

    sourceBox.setBounds(header.removeFromLeft(150).reduced(2, 10));
    header.removeFromLeft(6);
    viewModeBox.setBounds(header.removeFromLeft(100).reduced(2, 10));
    header.removeFromLeft(6);
    timeWindowBox.setBounds(header.removeFromLeft(80).reduced(2, 10));
    header.removeFromLeft(8);

    freezeBtn.setBounds(header.removeFromLeft(70).reduced(2, 10));
    header.removeFromLeft(6);
    bypassBtn.setBounds(header.removeFromLeft(70).reduced(2, 10));

    // Bottom Control Rack (Height: 220 px)
    auto rackArea = area.removeFromBottom(220).reduced(12, 6);

    int compWidth = juce::roundToInt(rackArea.getWidth() * 0.45f);
    int eqWidth = juce::roundToInt(rackArea.getWidth() * 0.32f);
    int clipWidth = rackArea.getWidth() - compWidth - eqWidth - 16;

    auto compBounds = rackArea.removeFromLeft(compWidth);
    rackArea.removeFromLeft(8);
    auto eqBounds = rackArea.removeFromLeft(eqWidth);
    rackArea.removeFromLeft(8);
    auto clipBounds = rackArea;

    compGroup.setBounds(compBounds);
    eqGroup.setBounds(eqBounds);
    clipGroup.setBounds(clipBounds);

    // Section 1: Compressor
    auto compInner = compBounds.reduced(10, 24);
    compEnableToggle.setBounds(compInner.removeFromTop(20));

    int compCols = 3;
    int compRows = 2;
    float colW = compInner.getWidth() / float(compCols);
    float rowH = compInner.getHeight() / float(compRows);

    compThresholdSlider.setBounds(compInner.getX(), compInner.getY(), colW, rowH);
    compRatioSlider.setBounds(compInner.getX() + colW, compInner.getY(), colW, rowH);
    compAttackSlider.setBounds(compInner.getX() + 2 * colW, compInner.getY(), colW, rowH);

    compReleaseSlider.setBounds(compInner.getX(), compInner.getY() + rowH, colW, rowH);
    compMakeupSlider.setBounds(compInner.getX() + colW, compInner.getY() + rowH, colW, rowH);
    compKneeSlider.setBounds(compInner.getX() + 2 * colW, compInner.getY() + rowH, colW, rowH);

    // Section 2: Equalizer
    auto eqInner = eqBounds.reduced(10, 24);
    eqEnableToggle.setBounds(eqInner.removeFromTop(20));

    float eqColW = eqInner.getWidth() / 2.0f;
    float eqRowH = eqInner.getHeight() / 2.0f;

    eqLowSlider.setBounds(eqInner.getX(), eqInner.getY(), eqColW, eqRowH);
    eqMidSlider.setBounds(eqInner.getX() + eqColW, eqInner.getY(), eqColW, eqRowH);
    eqMidQSlider.setBounds(eqInner.getX(), eqInner.getY() + eqRowH, eqColW, eqRowH);
    eqHighSlider.setBounds(eqInner.getX() + eqColW, eqInner.getY() + eqRowH, eqColW, eqRowH);

    // Section 3: Clipper
    auto clipInner = clipBounds.reduced(10, 24);
    clipEnableToggle.setBounds(clipInner.removeFromTop(20));

    float clipRowH = clipInner.getHeight() / 2.0f;
    float clipColW = clipInner.getWidth() / 2.0f;

    driveSlider.setBounds(clipInner.getX(), clipInner.getY(), clipColW, clipRowH);
    clipCeilingSlider.setBounds(clipInner.getX() + clipColW, clipInner.getY(), clipColW, clipRowH);

    clipModeBox.setBounds(clipInner.getX() + 4, clipInner.getY() + clipRowH + 12, clipInner.getWidth() - 8, 24);

    // Center Stage Visualizer
    auto visualizerArea = area.reduced(12, 8);
    liveVisualizer.setBounds(visualizerArea);
    filePreview.setBounds(visualizerArea);
}
