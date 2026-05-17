// Source/PresetManager.h
#pragma once

#include <juce_audio_utils/juce_audio_utils.h>

class PresetManager
{
public:
    explicit PresetManager(juce::AudioProcessorValueTreeState& apvts);

    bool savePreset(const juce::String& name, juce::Result& result);
    bool loadPreset(const juce::String& name, juce::Result& result);
    bool deletePreset(const juce::String& name, juce::Result& result);

    juce::Array<juce::String> getPresetList() const;
    juce::File getUserPresetDir() const { return userPresetDir; }
    juce::File getPresetFile(const juce::String& name) const;

private:
    juce::AudioProcessorValueTreeState& apvts;
    juce::File userPresetDir;
};
