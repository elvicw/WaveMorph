cmake_minimum_required(VERSION 3.22)

project(WaveMorph VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add JUCE from libs/JUCE
add_subdirectory(libs/JUCE)

# Plugin formats
set(PLUGIN_FORMATS VST3 Standalone)
if(APPLE)
    list(APPEND PLUGIN_FORMATS AU)
endif()

juce_add_plugin(WaveMorph
    COMPANY_NAME "WaveMorphAudio"
    PLUGIN_NAME "WaveMorph"
    BUNDLE_ID "com.wavemorph.previewer"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD FALSE
    PLUGIN_MANUFACTURER_CODE WvMr
    PLUGIN_CODE WvTr
    FORMATS ${PLUGIN_FORMATS}
    PRODUCT_NAME "WaveMorph"
)

target_sources(WaveMorph
    PRIVATE
        Source/PluginProcessor.h
        Source/PluginProcessor.cpp
        Source/PluginEditor.h
        Source/PluginEditor.cpp
        Source/WaveformBuffer.h
        Source/WaveformVisualizer.h
        Source/WaveformVisualizer.cpp
        Source/FileWaveformPreview.h
        Source/FileWaveformPreview.cpp
        Source/TransformDSP.h
        Source/TransformDSP.cpp
        Source/CustomLookAndFeel.h
)

target_compile_definitions(WaveMorph
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        NOMINMAX
        _CRT_SECURE_NO_WARNINGS
)

target_link_libraries(WaveMorph
    PRIVATE
        juce::juce_audio_utils
        juce::juce_audio_processors
        juce::juce_audio_formats
        juce::juce_audio_basics
        juce::juce_dsp
        juce::juce_gui_extra
        juce::juce_gui_basics
        juce::juce_graphics
        juce::juce_events
        juce::juce_core
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
