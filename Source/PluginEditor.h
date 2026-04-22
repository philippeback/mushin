#pragma once

#include "PluginProcessor.h"
#include <juce_gui_extra/juce_gui_extra.h>

class MushinAudioProcessorEditor : public juce::AudioProcessorEditor,
                                   private juce::AudioProcessorValueTreeState::Listener,
                                   private juce::Timer
{
public:
    explicit MushinAudioProcessorEditor (MushinAudioProcessor&);
    ~MushinAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void parameterChanged (const juce::String& parameterID, float newValue) override;
    void timerCallback() override;

    MushinAudioProcessor& audioProcessor;
    
    juce::WebBrowserComponent webComponent;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MushinAudioProcessorEditor)
};
