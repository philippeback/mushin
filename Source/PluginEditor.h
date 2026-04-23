#pragma once

#include "PluginProcessor.h"
#include <juce_gui_extra/juce_gui_extra.h>

class MushinAudioProcessorEditor : public juce::AudioProcessorEditor,
                                   private juce::AudioProcessorValueTreeState::Listener,
                                   private juce::Timer
{
public:
    MushinAudioProcessorEditor (MushinAudioProcessor&);
    ~MushinAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // Custom subclass to intercept URLs as a fallback bridge
    class MushinWebComponent : public juce::WebBrowserComponent {
    public:
        MushinWebComponent(const juce::WebBrowserComponent::Options& options, MushinAudioProcessor& p) 
            : juce::WebBrowserComponent(options), processor(p) {}

        bool pageAboutToLoad(const juce::String& newURL) override {
            if (newURL.startsWith("mushin://")) {
                handleCustomUrl(newURL);
                return false; // Prevent actual navigation
            }
            return true;
        }

    private:
        void handleCustomUrl(const juce::String& url);
        MushinAudioProcessor& processor;
    };

    void parameterChanged (const juce::String& parameterID, float newValue) override;
    void timerCallback() override;

    MushinAudioProcessor& audioProcessor;
    MushinWebComponent webComponent;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MushinAudioProcessorEditor)
};
