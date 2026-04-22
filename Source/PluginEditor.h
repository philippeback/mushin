#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class MushinAudioProcessorEditor : public juce::AudioProcessorEditor, public juce::Timer
{
public:
    MushinAudioProcessorEditor (MushinAudioProcessor&);
    ~MushinAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

private:
    MushinAudioProcessor& audioProcessor;

    juce::Slider gainSlider;
    juce::Label gainLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> gainAttachment;

    juce::AudioVisualiserComponent visualiser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MushinAudioProcessorEditor)
};
