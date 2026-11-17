#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "FileWaveformPreview.h"

WaveMorphAudioProcessor::WaveMorphAudioProcessor()
    : AudioProcessor(BusesProperties()
                     .withInput("Input", juce::AudioChannelSet::stereo(), true)
                     .withOutput("Output", juce::AudioChannelSet::stereo(), true)
                     .withInput("Sidechain", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "Parameters", createParameterLayout())
{
}

WaveMorphAudioProcessor::~WaveMorphAudioProcessor()
{
}

juce::AudioProcessorValueTreeState::ParameterLayout WaveMorphAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // Compressor
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("compThreshold", 1), "Comp Threshold",
        juce::NormalisableRange<float>(-60.0f, 0.0f, 0.1f), -18.0f, "dB"));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("compRatio", 1), "Comp Ratio",
        juce::NormalisableRange<float>(1.0f, 30.0f, 0.1f, 0.5f), 4.0f, ":1"));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("compAttack", 1), "Comp Attack",
        juce::NormalisableRange<float>(0.1f, 100.0f, 0.1f, 0.4f), 15.0f, "ms"));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("compRelease", 1), "Comp Release",
        juce::NormalisableRange<float>(10.0f, 1000.0f, 1.0f, 0.4f), 120.0f, "ms"));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("compMakeup", 1), "Comp Makeup",
        juce::NormalisableRange<float>(0.0f, 24.0f, 0.1f), 0.0f, "dB"));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("compKnee", 1), "Comp Knee",
        juce::NormalisableRange<float>(0.0f, 18.0f, 0.5f), 6.0f, "dB"));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID("compEnabled", 1), "Comp Enable", true));

    // EQ
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("eqLowGain", 1), "EQ Low (80Hz)",
        juce::NormalisableRange<float>(-18.0f, 18.0f, 0.1f), 0.0f, "dB"));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("eqMidGain", 1), "EQ Mid (1kHz)",
        juce::NormalisableRange<float>(-18.0f, 18.0f, 0.1f), 0.0f, "dB"));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("eqMidQ", 1), "EQ Mid Q",
        juce::NormalisableRange<float>(0.5f, 5.0f, 0.1f), 1.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("eqHighGain", 1), "EQ High (8kHz)",
        juce::NormalisableRange<float>(-18.0f, 18.0f, 0.1f), 0.0f, "dB"));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID("eqEnabled", 1), "EQ Enable", true));

    // Clipper & Saturation
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("drive", 1), "Drive / Saturation",
        juce::NormalisableRange<float>(0.0f, 24.0f, 0.1f), 0.0f, "dB"));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("clipCeiling", 1), "Clip Ceiling",
        juce::NormalisableRange<float>(-12.0f, 0.0f, 0.1f), -0.5f, "dB"));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID("clipMode", 1), "Clip Mode",
        juce::StringArray { "Soft (Tape/Warm)", "Hard (Digital)" }, 0));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID("clipEnabled", 1), "Clipper Enable", true));

    // Global
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID("dspBypass", 1), "Bypass", false));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID("inputSource", 1), "Input Source",
        juce::StringArray { "Internal DSP Preview", "External DAW Sidechain" }, 0));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID("viewMode", 1), "View Mode",
        juce::StringArray { "Overlay", "Split", "Delta" }, 0));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID("timeWindow", 1), "Time Window",
        juce::StringArray { "50 ms", "100 ms", "250 ms", "500 ms", "1000 ms", "2000 ms" }, 2));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID("freeze", 1), "Freeze Waveform", false));

    return { params.begin(), params.end() };
}

const juce::String WaveMorphAudioProcessor::getName() const
{
    return "WaveMorph";
}

bool WaveMorphAudioProcessor::acceptsMidi() const { return false; }
bool WaveMorphAudioProcessor::producesMidi() const { return false; }
bool WaveMorphAudioProcessor::isMidiEffect() const { return false; }
double WaveMorphAudioProcessor::getTailLengthSeconds() const { return 0.0; }
int WaveMorphAudioProcessor::getNumPrograms() { return 1; }
int WaveMorphAudioProcessor::getCurrentProgram() { return 0; }
void WaveMorphAudioProcessor::setCurrentProgram(int) {}
const juce::String WaveMorphAudioProcessor::getProgramName(int) { return {}; }
void WaveMorphAudioProcessor::changeProgramName(int, const juce::String&) {}

void WaveMorphAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    transformDSP.prepare(sampleRate, samplesPerBlock, getTotalNumOutputChannels());
    waveformBuffer.reset();

    dryCopyBuffer.setSize(2, samplesPerBlock);
    wetBuffer.setSize(2, samplesPerBlock);
}

void WaveMorphAudioProcessor::releaseResources()
{
}

bool WaveMorphAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto& mainIn = layouts.getMainInputChannelSet();
    const auto& mainOut = layouts.getMainOutputChannelSet();

    if (mainIn != juce::AudioChannelSet::mono() && mainIn != juce::AudioChannelSet::stereo())
        return false;

    if (mainIn != mainOut)
        return false;

    const auto& sidechain = layouts.getChannelSet(true, 1);
    if (!sidechain.isDisabled() && sidechain != juce::AudioChannelSet::mono() && sidechain != juce::AudioChannelSet::stereo())
        return false;

    return true;
}

void WaveMorphAudioProcessor::updateDSPParametersFromAPVTS()
{
    DSPParameters p;
    p.compThresholdDb = apvts.getRawParameterValue("compThreshold")->load();
    p.compRatio = apvts.getRawParameterValue("compRatio")->load();
    p.compAttackMs = apvts.getRawParameterValue("compAttack")->load();
    p.compReleaseMs = apvts.getRawParameterValue("compRelease")->load();
    p.compMakeupDb = apvts.getRawParameterValue("compMakeup")->load();
    p.compKneeDb = apvts.getRawParameterValue("compKnee")->load();
    p.compEnabled = apvts.getRawParameterValue("compEnabled")->load() > 0.5f;

    p.eqLowGainDb = apvts.getRawParameterValue("eqLowGain")->load();
    p.eqMidGainDb = apvts.getRawParameterValue("eqMidGain")->load();
    p.eqMidQ = apvts.getRawParameterValue("eqMidQ")->load();
    p.eqHighGainDb = apvts.getRawParameterValue("eqHighGain")->load();
    p.eqEnabled = apvts.getRawParameterValue("eqEnabled")->load() > 0.5f;

    p.driveDb = apvts.getRawParameterValue("drive")->load();
    p.clipCeilingDb = apvts.getRawParameterValue("clipCeiling")->load();
    p.clipMode = (int)apvts.getRawParameterValue("clipMode")->load();
    p.clipperEnabled = apvts.getRawParameterValue("clipEnabled")->load() > 0.5f;

    p.bypass = apvts.getRawParameterValue("dspBypass")->load() > 0.5f;

    transformDSP.setParameters(p);
}

void WaveMorphAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& /*midiMessages*/)
{
    juce::ScopedNoDenormals noDenormals;

    int totalNumInputChannels = getTotalNumInputChannels();
    int totalNumOutputChannels = getTotalNumOutputChannels();
    int numSamples = buffer.getNumSamples();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, numSamples);

    updateDSPParametersFromAPVTS();

    if (dryCopyBuffer.getNumSamples() < numSamples)
    {
        dryCopyBuffer.setSize(2, numSamples, false, false, true);
        wetBuffer.setSize(2, numSamples, false, false, true);
    }

    auto mainIn = getBusBuffer(buffer, true, 0);
    auto mainOut = getBusBuffer(buffer, false, 0);

    int sourceChoice = (int)apvts.getRawParameterValue("inputSource")->load();
    float maxGR = 0.0f;

    if (sourceChoice == 0) // Internal DSP Preview Mode
    {
        // Copy original input
        for (int ch = 0; ch < 2; ++ch)
        {
            int srcCh = std::min(ch, mainIn.getNumChannels() - 1);
            if (srcCh >= 0)
                dryCopyBuffer.copyFrom(ch, 0, mainIn, srcCh, 0, numSamples);
            else
                dryCopyBuffer.clear(ch, 0, numSamples);
        }

        // Process through DSP
        transformDSP.processBlock(dryCopyBuffer, wetBuffer, &maxGR);

        // Copy wet output to main output bus
        for (int ch = 0; ch < mainOut.getNumChannels(); ++ch)
        {
            int srcCh = std::min(ch, wetBuffer.getNumChannels() - 1);
            mainOut.copyFrom(ch, 0, wetBuffer, srcCh, 0, numSamples);
        }

        // Push to live waveform visualizer
        const float* beforeL = dryCopyBuffer.getReadPointer(0);
        const float* beforeR = dryCopyBuffer.getReadPointer(1);
        const float* afterL = wetBuffer.getReadPointer(0);
        const float* afterR = wetBuffer.getReadPointer(1);

        waveformBuffer.pushSamples(beforeL, beforeR, afterL, afterR, maxGR, numSamples);

        // Record into File Previewer if recording
        if (filePreviewPtr != nullptr)
        {
            filePreviewPtr->pushLiveRecordSamples(beforeL, beforeR, numSamples);
            filePreviewPtr->getNextAudioBlock(mainOut);
        }
    }
    else // External DAW Sidechain Mode
    {
        auto sidechainIn = getBusBuffer(buffer, true, 1);
        bool hasSidechain = (getBusCount(true) > 1 && sidechainIn.getNumChannels() > 0);

        const float* beforeL = nullptr;
        const float* beforeR = nullptr;

        if (hasSidechain)
        {
            beforeL = sidechainIn.getReadPointer(0);
            beforeR = (sidechainIn.getNumChannels() > 1) ? sidechainIn.getReadPointer(1) : beforeL;
        }
        else
        {
            beforeL = mainIn.getReadPointer(0);
            beforeR = (mainIn.getNumChannels() > 1) ? mainIn.getReadPointer(1) : beforeL;
        }

        const float* afterL = mainIn.getReadPointer(0);
        const float* afterR = (mainIn.getNumChannels() > 1) ? mainIn.getReadPointer(1) : afterL;

        waveformBuffer.pushSamples(beforeL, beforeR, afterL, afterR, 0.0f, numSamples);

        if (filePreviewPtr != nullptr)
        {
            filePreviewPtr->pushLiveRecordSamples(afterL, afterR, numSamples);
            filePreviewPtr->getNextAudioBlock(mainOut);
        }
    }
}

bool WaveMorphAudioProcessor::hasEditor() const { return true; }

juce::AudioProcessorEditor* WaveMorphAudioProcessor::createEditor()
{
    return new WaveMorphAudioProcessorEditor(*this);
}

void WaveMorphAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void WaveMorphAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState.get() != nullptr && xmlState->hasTagName(apvts.state.getType()))
        apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new WaveMorphAudioProcessor();
}
