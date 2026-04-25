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
        MushinWebComponent(const juce::WebBrowserComponent::Options& options, MushinAudioProcessor& p, MushinAudioProcessorEditor& ed) 
            : juce::WebBrowserComponent(options), processor(p), editor(ed) 
        {
            juce::Logger::writeToLog("MushinWebComponent subclass constructor - base call finished.");
        }

        bool pageAboutToLoad(const juce::String& newURL) override {
            if (newURL.startsWith("mushin://")) {
                handleCustomUrl(newURL);
                return false; 
            }
            return true;
        }

        void pageFinishedLoading(const juce::String& url) override;

    private:
        void handleCustomUrl(const juce::String& url);
        MushinAudioProcessor& processor;
        MushinAudioProcessorEditor& editor;
    };

    void syncAllParameters();
    void parameterChanged (const juce::String& parameterID, float newValue) override;
    void timerCallback() override;

    MushinAudioProcessor& audioProcessor;
    std::unique_ptr<MushinWebComponent> webComponent;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MushinAudioProcessorEditor)
};
