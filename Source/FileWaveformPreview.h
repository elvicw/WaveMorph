#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include "TransformDSP.h"

class FileWaveformPreview : public juce::Component,
                            public juce::FileDragAndDropTarget,
                            public juce::Timer
{
public:
    FileWaveformPreview(TransformDSP& dsp);
    ~FileWaveformPreview() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

    // File drag and drop
    bool isInterestedInFileDrag(const juce::StringArray& files) override;
    void filesDropped(const juce::StringArray& files, int x, int y) override;

    // Load audio file from path
    bool loadFile(const juce::File& file);

    // DAW track recording into preview buffer
    void startRecordingTrack();
    void stopRecordingTrack();
    bool isRecording() const { return recordingActive; }
    void pushLiveRecordSamples(const float* l, const float* r, int numSamples);

    // Recompute processed waveform when parameters change
    void triggerReprocess();

    // Playback for auditioning
    void togglePlay();
    void setPlayheadNormalized(float pos);
    void getNextAudioBlock(juce::AudioBuffer<float>& bufferToFill);

    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;

private:
    TransformDSP& liveDsp;
    juce::AudioFormatManager formatManager;

    // Original and Processed buffers
    juce::AudioBuffer<float> originalBuffer;
    juce::AudioBuffer<float> processedBuffer;
    double fileSampleRate = 44100.0;
    juce::String currentFileName = "Drag & Drop Audio File (.wav, .aif, .mp3) or Click [REC TRACK]";

    // Recording buffer for capturing from DAW
    bool recordingActive = false;
    juce::AudioBuffer<float> recordBuffer;
    int recordWritePos = 0;
    static constexpr int MAX_RECORD_SECONDS = 15;

    // Min/Max envelopes for display (1024 points)
    static constexpr int DISPLAY_POINTS = 1024;
    std::vector<float> origMinEnv;
    std::vector<float> origMaxEnv;
    std::vector<float> procMinEnv;
    std::vector<float> procMaxEnv;

    // Metrics
    float peakInDb = 0.0f;
    float peakOutDb = 0.0f;
    float rmsInDb = 0.0f;
    float rmsOutDb = 0.0f;
    float crestInDb = 0.0f;
    float crestOutDb = 0.0f;
    int clippedCount = 0;

    // Playback state
    bool isPlaying = false;
    std::atomic<int> playheadSample { 0 };
    bool isLooping = true;
    bool listenProcessed = true; // true = hear wet, false = hear dry

    // UI Buttons
    juce::TextButton loadFileBtn { "Open Audio File" };
    juce::TextButton recTrackBtn { "REC TRACK (DAW)" };
    juce::TextButton playBtn { "PLAY PREVIEW" };
    juce::ToggleButton loopBtn { "Loop" };
    juce::ToggleButton listenModeBtn { "Hear Processed" };

    void reprocessAudio();
    void computeEnvelopes();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FileWaveformPreview)
};
