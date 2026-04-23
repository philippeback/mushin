cmake_minimum_required(VERSION 3.20)
project(Mushin VERSION 1.0.0)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)
FetchContent_Declare(
    juce
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.4
    GIT_SHALLOW    ON
)
FetchContent_MakeAvailable(juce)

juce_add_plugin(Mushin
    COMPANY_NAME "PhilippeBack"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    PLUGIN_MANUFACTURER_CODE Phbk
    PLUGIN_CODE Msh2
    FORMATS VST3 Standalone
    PRODUCT_NAME "Mushin"
    NEEDS_WEBVIEW2 TRUE
)

juce_add_binary_data(MushinWebData
    SOURCES
        Source/Web/index.html
)

juce_generate_juce_header(Mushin)

target_sources(Mushin
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/dsp_waveshaper/Waveshaper.h
)

target_compile_definitions(Mushin
    PUBLIC
        JUCE_WEB_BROWSER=1
        JUCE_USE_WIN_WEBVIEW2=1
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(Mushin
    PRIVATE
        MushinWebData
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
