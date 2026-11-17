#include "FileWaveformPreview.h"
#include <cmath>

FileWaveformPreview::FileWaveformPreview(TransformDSP& dsp)
    : liveDsp(dsp)
{
    formatManager.registerBasicFormats();

    origMinEnv.resize(DISPLAY_POINTS, 0.0f);
    origMaxEnv.resize(DISPLAY_POINTS, 0.0f);
    procMinEnv.resize(DISPLAY_POINTS, 0.0f);
    procMaxEnv.resize(DISPLAY_POINTS, 0.0f);

    addAndMakeVisible(loadFileBtn);
    loadFileBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff222736));
    loadFileBtn.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffc5cadb));
    loadFileBtn.onClick = [this]()
    {
        auto chooser = std::make_shared<juce::FileChooser>(
            "Select an Audio File", juce::File::getSpecialLocation(juce::File::userHomeDirectory),
            "*.wav;*.aif;*.aiff;*.mp3;*.flac;*.ogg");

        auto folderChooserFlags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;
        chooser->launchAsync(folderChooserFlags, [this, chooser](const juce::FileChooser& fc)
        {
            auto result = fc.getResult();
            if (result.existsAsFile())
                loadFile(result);
        });
    };

    addAndMakeVisible(recTrackBtn);
    recTrackBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff3b1926));
    recTrackBtn.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffff5252));
    recTrackBtn.onClick = [this]()
    {
        if (recordingActive)
            stopRecordingTrack();
        else
            startRecordingTrack();
    };

    addAndMakeVisible(playBtn);
    playBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff183325));
    playBtn.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff00e676));
    playBtn.onClick = [this]() { togglePlay(); };

    addAndMakeVisible(loopBtn);
    loopBtn.setToggleState(isLooping, juce::dontSendNotification);
    loopBtn.onClick = [this]() { isLooping = loopBtn.getToggleState(); };

    addAndMakeVisible(listenModeBtn);
    listenModeBtn.setToggleState(listenProcessed, juce::dontSendNotification);
    listenModeBtn.onClick = [this]()
    {
        listenProcessed = listenModeBtn.getToggleState();
        listenModeBtn.setButtonText(listenProcessed ? "Audition: Processed" : "Audition: Dry/Original");
    };

    startTimerHz(30);
}

FileWaveformPreview::~FileWaveformPreview()
{
    stopTimer();
}

void FileWaveformPreview::timerCallback()
{
    if (isPlaying)
        repaint();
}

void FileWaveformPreview::resized()
{
    auto area = getLocalBounds();
    auto topBar = area.removeFromTop(38);

    loadFileBtn.setBounds(topBar.removeFromLeft(130).reduced(2));
    topBar.removeFromLeft(8);
    recTrackBtn.setBounds(topBar.removeFromLeft(140).reduced(2));
    topBar.removeFromLeft(8);
    playBtn.setBounds(topBar.removeFromLeft(130).reduced(2));
    topBar.removeFromLeft(8);
    loopBtn.setBounds(topBar.removeFromLeft(70).reduced(2));
    topBar.removeFromLeft(8);
    listenModeBtn.setBounds(topBar.removeFromLeft(160).reduced(2));
}

bool FileWaveformPreview::isInterestedInFileDrag(const juce::StringArray& files)
{
    for (const auto& file : files)
    {
        if (file.endsWithIgnoreCase(".wav") || file.endsWithIgnoreCase(".aif") ||
            file.endsWithIgnoreCase(".aiff") || file.endsWithIgnoreCase(".mp3") ||
            file.endsWithIgnoreCase(".flac") || file.endsWithIgnoreCase(".ogg"))
            return true;
    }
    return false;
}

void FileWaveformPreview::filesDropped(const juce::StringArray& files, int /*x*/, int /*y*/)
{
    if (files.size() > 0)
        loadFile(juce::File(files[0]));
}

bool FileWaveformPreview::loadFile(const juce::File& file)
{
    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(file));
    if (reader == nullptr)
        return false;

    currentFileName = file.getFileName();
    fileSampleRate = reader->sampleRate;
    int length = (int)reader->lengthInSamples;

    // Limit to 60 seconds maximum for preview
    int maxSamples = juce::roundToInt(60.0 * fileSampleRate);
    if (length > maxSamples)
        length = maxSamples;

    originalBuffer.setSize(2, length);
    reader->read(&originalBuffer, 0, length, 0, true, true);

    if (reader->numChannels == 1)
        originalBuffer.copyFrom(1, 0, originalBuffer, 0, 0, length);

    reprocessAudio();
    playheadSample.store(0);
    repaint();
    return true;
}

void FileWaveformPreview::startRecordingTrack()
{
    int maxSamples = juce::roundToInt(MAX_RECORD_SECONDS * (fileSampleRate > 0 ? fileSampleRate : 44100.0));
    recordBuffer.setSize(2, maxSamples);
    recordBuffer.clear();
    recordWritePos = 0;
    recordingActive = true;
    recTrackBtn.setButtonText("STOP REC (15s Max)");
    recTrackBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff881122));
    repaint();
}

void FileWaveformPreview::stopRecordingTrack()
{
    recordingActive = false;
    recTrackBtn.setButtonText("REC TRACK (DAW)");
    recTrackBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff3b1926));

    if (recordWritePos > 1024)
    {
        originalBuffer.setSize(2, recordWritePos);
        originalBuffer.copyFrom(0, 0, recordBuffer, 0, 0, recordWritePos);
        originalBuffer.copyFrom(1, 0, recordBuffer, 1, 0, recordWritePos);
        currentFileName = "DAW Live Capture (" + juce::String(recordWritePos / fileSampleRate, 1) + "s)";
        reprocessAudio();
        playheadSample.store(0);
    }
    repaint();
}

void FileWaveformPreview::pushLiveRecordSamples(const float* l, const float* r, int numSamples)
{
    if (!recordingActive) return;

    int maxSamples = recordBuffer.getNumSamples();
    int samplesToWrite = std::min(numSamples, maxSamples - recordWritePos);

    if (samplesToWrite > 0)
    {
        recordBuffer.copyFrom(0, recordWritePos, l, samplesToWrite);
        if (r != nullptr)
            recordBuffer.copyFrom(1, recordWritePos, r, samplesToWrite);
        else
            recordBuffer.copyFrom(1, recordWritePos, l, samplesToWrite);

        recordWritePos += samplesToWrite;

        if (recordWritePos >= maxSamples)
            stopRecordingTrack();
    }
}

void FileWaveformPreview::triggerReprocess()
{
    if (originalBuffer.getNumSamples() > 0)
    {
        reprocessAudio();
        repaint();
    }
}

void FileWaveformPreview::reprocessAudio()
{
    int numSamples = originalBuffer.getNumSamples();
    if (numSamples <= 0) return;

    processedBuffer.setSize(2, numSamples);

    TransformDSP::processOfflineBuffer(originalBuffer, processedBuffer, fileSampleRate,
                                       liveDsp.getParameters(),
                                       peakInDb, peakOutDb,
                                       rmsInDb, rmsOutDb,
                                       crestInDb, crestOutDb,
                                       clippedCount);

    computeEnvelopes();
}

void FileWaveformPreview::computeEnvelopes()
{
    int numSamples = originalBuffer.getNumSamples();
    if (numSamples <= 0) return;

    float samplesPerPoint = float(numSamples) / float(DISPLAY_POINTS);

    for (int p = 0; p < DISPLAY_POINTS; ++p)
    {
        int startSample = juce::roundToInt(p * samplesPerPoint);
        int endSample = juce::roundToInt((p + 1) * samplesPerPoint);
        startSample = juce::jlimit(0, numSamples - 1, startSample);
        endSample = juce::jlimit(startSample + 1, numSamples, endSample);

        float minO = 1.0f, maxO = -1.0f;
        float minP = 1.0f, maxP = -1.0f;

        for (int s = startSample; s < endSample; ++s)
        {
            float o = 0.5f * (originalBuffer.getSample(0, s) + originalBuffer.getSample(1, s));
            float pr = 0.5f * (processedBuffer.getSample(0, s) + processedBuffer.getSample(1, s));

            if (o < minO) minO = o;
            if (o > maxO) maxO = o;
            if (pr < minP) minP = pr;
            if (pr > maxP) maxP = pr;
        }

        origMinEnv[p] = minO;
        origMaxEnv[p] = maxO;
        procMinEnv[p] = minP;
        procMaxEnv[p] = maxP;
    }
}

void FileWaveformPreview::togglePlay()
{
    if (originalBuffer.getNumSamples() == 0) return;

    isPlaying = !isPlaying;
    playBtn.setButtonText(isPlaying ? "PAUSE PREVIEW" : "PLAY PREVIEW");
    playBtn.setColour(juce::TextButton::buttonColourId, isPlaying ? juce::Colour(0xff4a3a14) : juce::Colour(0xff183325));
    repaint();
}

void FileWaveformPreview::setPlayheadNormalized(float pos)
{
    int total = originalBuffer.getNumSamples();
    if (total > 0)
    {
        int sample = juce::roundToInt(juce::jlimit(0.0f, 1.0f, pos) * float(total - 1));
        playheadSample.store(sample);
        repaint();
    }
}

void FileWaveformPreview::getNextAudioBlock(juce::AudioBuffer<float>& bufferToFill)
{
    if (!isPlaying || originalBuffer.getNumSamples() == 0) return;

    int totalSamples = originalBuffer.getNumSamples();
    int numSamples = bufferToFill.getNumSamples();
    int currentPos = playheadSample.load();

    const juce::AudioBuffer<float>& source = listenProcessed ? processedBuffer : originalBuffer;

    for (int ch = 0; ch < bufferToFill.getNumChannels(); ++ch)
    {
        int srcCh = ch < source.getNumChannels() ? ch : 0;
        float* dest = bufferToFill.getWritePointer(ch);

        for (int i = 0; i < numSamples; ++i)
        {
            int s = currentPos + i;
            if (s >= totalSamples)
            {
                if (isLooping)
                    s = s % totalSamples;
                else
                {
                    dest[i] += 0.0f;
                    continue;
                }
            }
            dest[i] += source.getSample(srcCh, s);
        }
    }

    currentPos += numSamples;
    if (currentPos >= totalSamples)
    {
        if (isLooping)
            currentPos %= totalSamples;
        else
        {
            currentPos = 0;
            isPlaying = false;
        }
    }
    playheadSample.store(currentPos);
}

void FileWaveformPreview::mouseDown(const juce::MouseEvent& e)
{
    auto waveArea = getLocalBounds();
    waveArea.removeFromTop(44);
    waveArea.removeFromBottom(70);

    if (waveArea.contains(e.getPosition()))
    {
        float normX = (float(e.x) - float(waveArea.getX())) / float(waveArea.getWidth());
        setPlayheadNormalized(normX);
    }
}

void FileWaveformPreview::mouseDrag(const juce::MouseEvent& e)
{
    mouseDown(e);
}

void FileWaveformPreview::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // Background
    g.fillAll(juce::Colour(0xff0f1118));

    // File name banner
    g.setFont(juce::Font(12.0f, juce::Font::bold));
    g.setColour(juce::Colour(0xff8c96b3));
    g.drawText(currentFileName, 12.0f, bounds.getY() + 42.0f, bounds.getWidth() - 24.0f, 18.0f, juce::Justification::left);

    // Waveform rendering area
    auto waveArea = bounds;
    waveArea.removeFromTop(64.0f);
    auto statsArea = waveArea.removeFromBottom(64.0f);

    // Draw waveform background box
    g.setColour(juce::Colour(0xff080a0e));
    g.fillRoundedRectangle(waveArea, 4.0f);
    g.setColour(juce::Colour(0xff222633));
    g.drawRoundedRectangle(waveArea, 4.0f, 1.0f);

    float centerY = waveArea.getCentreY();
    float halfH = (waveArea.getHeight() - 10.0f) * 0.5f;

    // Center zero axis
    g.setColour(juce::Colour(0x22ffffff));
    g.drawHorizontalLine(juce::roundToInt(centerY), waveArea.getX(), waveArea.getRight());

    // Draw Waveforms
    if (originalBuffer.getNumSamples() > 0)
    {
        float stepX = waveArea.getWidth() / float(DISPLAY_POINTS);

        // 1. Draw Original Waveform (Cyan)
        for (int p = 0; p < DISPLAY_POINTS; ++p)
        {
            float x = waveArea.getX() + p * stepX;
            float yTop = centerY - origMaxEnv[p] * halfH;
            float yBtm = centerY - origMinEnv[p] * halfH;

            g.setColour(juce::Colour(0x5500e5ff));
            g.drawVerticalLine(juce::roundToInt(x), yTop, yBtm);
        }

        // 2. Draw Transformed Waveform (Amber)
        for (int p = 0; p < DISPLAY_POINTS; ++p)
        {
            float x = waveArea.getX() + p * stepX;
            float yTop = centerY - procMaxEnv[p] * halfH;
            float yBtm = centerY - procMinEnv[p] * halfH;

            // Highlight squashed peaks in red/crimson
            if (origMaxEnv[p] > procMaxEnv[p] + 0.05f)
            {
                float squashTop = centerY - origMaxEnv[p] * halfH;
                float squashBtm = centerY - procMaxEnv[p] * halfH;
                g.setColour(juce::Colour(0x88ff3366));
                g.drawVerticalLine(juce::roundToInt(x), squashTop, squashBtm);
            }

            g.setColour(juce::Colour(0xccffb300));
            g.drawVerticalLine(juce::roundToInt(x), yTop, yBtm);
        }

        // 3. Playhead
        int total = originalBuffer.getNumSamples();
        if (total > 0)
        {
            float playPosNorm = float(playheadSample.load()) / float(total);
            float playheadX = waveArea.getX() + playPosNorm * waveArea.getWidth();

            g.setColour(juce::Colour(0xffffffff));
            g.drawVerticalLine(juce::roundToInt(playheadX), waveArea.getY(), waveArea.getBottom());

            // Playhead triangle marker
            juce::Path tri;
            tri.addTriangle(playheadX - 5.0f, waveArea.getY(), playheadX + 5.0f, waveArea.getY(), playheadX, waveArea.getY() + 8.0f);
            g.fillPath(tri);
        }
    }
    else
    {
        // Empty placeholder instructions
        g.setFont(juce::Font(14.0f));
        g.setColour(juce::Colour(0x66ffffff));
        g.drawText("Drag & drop any audio file here (.wav, .aif, .mp3)\nor click [REC TRACK] to record your DAW session in real-time!",
                   waveArea, juce::Justification::centred);
    }

    // Transformation Metrics HUD Card
    g.setColour(juce::Colour(0xff141722));
    g.fillRoundedRectangle(statsArea.reduced(2.0f, 4.0f), 6.0f);
    g.setColour(juce::Colour(0xff2b3042));
    g.drawRoundedRectangle(statsArea.reduced(2.0f, 4.0f), 6.0f, 1.0f);

    float statColW = statsArea.getWidth() / 4.0f;
    g.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 11.0f, juce::Font::plain));

    // Column 1: Peak
    auto col1 = statsArea.removeFromLeft(statColW).reduced(6.0f);
    g.setColour(juce::Colour(0xff8c96b3));
    g.drawText("PEAK TRANSFORMATION", col1.removeFromTop(16.0f), juce::Justification::left);
    juce::String peakStr = "In: " + juce::String(peakInDb, 1) + " dB -> Out: " + juce::String(peakOutDb, 1) + " dB";
    g.setColour(juce::Colour(0xffe1e6f5));
    g.drawText(peakStr, col1, juce::Justification::left);

    // Column 2: RMS Loudness
    auto col2 = statsArea.removeFromLeft(statColW).reduced(6.0f);
    g.setColour(juce::Colour(0xff8c96b3));
    g.drawText("RMS LOUDNESS", col2.removeFromTop(16.0f), juce::Justification::left);
    float rmsDelta = rmsOutDb - rmsInDb;
    juce::String rmsStr = "In: " + juce::String(rmsInDb, 1) + " -> Out: " + juce::String(rmsOutDb, 1) + " (" + (rmsDelta >= 0 ? "+" : "") + juce::String(rmsDelta, 1) + " dB)";
    g.setColour(rmsDelta >= 0 ? juce::Colour(0xffffb300) : juce::Colour(0xff00e5ff));
    g.drawText(rmsStr, col2, juce::Justification::left);

    // Column 3: Crest Factor (Dynamic Squash)
    auto col3 = statsArea.removeFromLeft(statColW).reduced(6.0f);
    g.setColour(juce::Colour(0xff8c96b3));
    g.drawText("DYNAMIC RANGE / CREST", col3.removeFromTop(16.0f), juce::Justification::left);
    float crestDelta = crestOutDb - crestInDb;
    juce::String crestStr = "In: " + juce::String(crestInDb, 1) + " dB -> Out: " + juce::String(crestOutDb, 1) + " dB (" + juce::String(crestDelta, 1) + " dB)";
    g.setColour(crestDelta < -1.0f ? juce::Colour(0xffff3366) : juce::Colour(0xffe1e6f5));
    g.drawText(crestStr, col3, juce::Justification::left);

    // Column 4: Clipped Samples
    auto col4 = statsArea.reduced(6.0f);
    g.setColour(juce::Colour(0xff8c96b3));
    g.drawText("CLIPPING HEADROOM", col4.removeFromTop(16.0f), juce::Justification::left);
    juce::String clipStr = (clippedCount > 0) ? ("Clipped: " + juce::String(clippedCount) + " samples") : "Clean (No Digital Clip)";
    g.setColour(clippedCount > 0 ? juce::Colour(0xffff5252) : juce::Colour(0xff00e676));
    g.drawText(clipStr, col4, juce::Justification::left);
}
