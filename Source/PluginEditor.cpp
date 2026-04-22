#include "PluginEditor.h"
#include "PluginProcessor.h"
#include <JuceHeader.h>

MushinAudioProcessorEditor::MushinAudioProcessorEditor (MushinAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p),
      webComponent (juce::WebBrowserComponent::Options{}
                    .withBackend (juce::WebBrowserComponent::Options::Backend::webview2)
                    .withWinWebView2Options (juce::WebBrowserComponent::Options::WinWebView2{}
                                             .withUserDataFolder (juce::File::getSpecialLocation (juce::File::tempDirectory)
                                                                  .getChildFile ("Mushin_WebView2_Cache")))
                    .withNativeIntegrationEnabled (true)
                    .withResourceProvider ([] (const juce::String& url) -> std::optional<juce::WebBrowserComponent::Resource>
                    {
                        auto path = (url == "/" || url.isEmpty()) ? "index_html" : url.substring(1).replace(".", "_").replace("-", "_");

                        int size = 0;
                        if (auto data = BinaryData::getNamedResource (path.toRawUTF8(), size))
                        {
                            return juce::WebBrowserComponent::Resource {
                                std::vector<std::byte> (reinterpret_cast<const std::byte*> (data), 
                                                       reinterpret_cast<const std::byte*> (data) + size),
                                url.endsWith(".js") ? "application/javascript" : 
                                (url.endsWith(".css") ? "text/css" : "text/html")
                            };
                        }
                        
                        return std::nullopt;
                    })
                    .withNativeFunction ("setParameterValue", [this] (const juce::Array<juce::var>& args, juce::WebBrowserComponent::NativeFunctionCompletion completion)
                    {
                        if (args.size() >= 2)
                        {
                            auto paramID = args[0].toString();
                            auto value = (float) args[1];

                            if (auto* param = audioProcessor.treeState.getParameter(paramID))
                            {
                                param->setValueNotifyingHost(value);
                            }
                        }
                        completion (juce::var (true));
                    }))
{
    addAndMakeVisible (webComponent);
    webComponent.goToURL (juce::WebBrowserComponent::getResourceProviderRoot());

    // Listen for parameter changes to update the UI
    audioProcessor.treeState.addParameterListener ("gain", this);
    audioProcessor.treeState.addParameterListener ("drive", this);
    audioProcessor.treeState.addParameterListener ("exhaustion", this);
    audioProcessor.treeState.addParameterListener ("threshold", this);

    // Make the UI resizable
    setResizable(true, true);
    setResizeLimits(400, 300, 2000, 1500);
    getConstrainer()->setFixedAspectRatio(1.5); // Optional: keep a nice ratio

    setSize (800, 600);
}

MushinAudioProcessorEditor::~MushinAudioProcessorEditor()
{
    audioProcessor.treeState.removeParameterListener ("gain", this);
    audioProcessor.treeState.removeParameterListener ("drive", this);
    audioProcessor.treeState.removeParameterListener ("exhaustion", this);
    audioProcessor.treeState.removeParameterListener ("threshold", this);
}

void MushinAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black);
}

void MushinAudioProcessorEditor::resized()
{
    webComponent.setBounds (getLocalBounds());
}

void MushinAudioProcessorEditor::parameterChanged (const juce::String& parameterID, float newValue)
{
    juce::String js = "if (window.setParameterValue) window.setParameterValue('" + parameterID + "', " + juce::String(newValue) + ");";
    
    juce::MessageManager::callAsync([this, js] {
        webComponent.evaluateJavascript(js);
    });
}
