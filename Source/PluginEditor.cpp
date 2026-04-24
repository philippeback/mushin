#include "PluginEditor.h"
#include "PluginProcessor.h"
#include <JuceHeader.h>

void MushinAudioProcessorEditor::MushinWebComponent::handleCustomUrl(const juce::String& url) {
    auto cmd = url.substring(9); 
    if (cmd.startsWith("setParameterValue?")) {
        auto params = cmd.substring(18);
        auto paramID = params.upToFirstOccurrenceOf("&", false, false).fromFirstOccurrenceOf("id=", false, false);
        auto valStr = params.fromFirstOccurrenceOf("val=", false, false);
        float value = valStr.getFloatValue();

        processor.lastParamId = paramID;
        processor.lastUiValue.store(value);
        if (auto* param = processor.treeState.getParameter(paramID))
            param->setValueNotifyingHost(value);
    }
}

void MushinAudioProcessorEditor::MushinWebComponent::pageFinishedLoading(const juce::String& url) {
    juce::ignoreUnused(url);
    editor.syncAllParameters();
    // Second sync after a small delay to ensure JS is ready
    juce::Timer::callAfterDelay(500, [this] { editor.syncAllParameters(); });
}

MushinAudioProcessorEditor::MushinAudioProcessorEditor (MushinAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p),
      webComponent (juce::WebBrowserComponent::Options{}
                    .withBackend (juce::WebBrowserComponent::Options::Backend::webview2)
                    .withWinWebView2Options (juce::WebBrowserComponent::Options::WinWebView2{}
                        .withUserDataFolder (juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
                                              .getChildFile ("Mushin_VST3_Final")))
                    .withNativeIntegrationEnabled (true)
                    .withResourceProvider ([this] (const juce::String& url) -> std::optional<juce::WebBrowserComponent::Resource>
                    {
                        auto path = url;
                        if (path.startsWith("/")) path = path.substring(1);
                        if (path.contains("?")) path = path.upToFirstOccurrenceOf("?", false, false);
                        auto resourceName = (path.isEmpty() || path == "index.html") ? "index_html" : path.replace(".", "_").replace("-", "_");
                        int size = 0;
                        if (auto data = BinaryData::getNamedResource (resourceName.toRawUTF8(), size)) {
                            return juce::WebBrowserComponent::Resource {
                                std::vector<std::byte> (reinterpret_cast<const std::byte*> (data), reinterpret_cast<const std::byte*> (data) + size),
                                url.contains(".js") ? "application/javascript" : (url.contains(".css") ? "text/css" : "text/html")
                            };
                        }
                        return std::nullopt;
                    })
                    .withNativeFunction ("setParameterValue", [this] (const juce::Array<juce::var>& args, juce::WebBrowserComponent::NativeFunctionCompletion completion)
                    {
                        if (args.size() >= 2) {
                            auto id = args[0].toString();
                            auto val = (float) args[1];
                            audioProcessor.lastParamId = id;
                            audioProcessor.lastUiValue.store(val);
                            if (auto* pr = audioProcessor.treeState.getParameter(id)) pr->setValueNotifyingHost(val);
                        }
                        completion (juce::var (true));
                    })
                    .withNativeFunction ("log", [] (const juce::Array<juce::var>& args, juce::WebBrowserComponent::NativeFunctionCompletion completion)
                    {
                        if (args.size() >= 1) juce::Logger::writeToLog ("JS Log: " + args[0].toString());
                        completion (juce::var (true));
                    }), 
                    p, *this)
{
    addAndMakeVisible (webComponent);
    webComponent.goToURL (juce::WebBrowserComponent::getResourceProviderRoot());

    // Add listeners for all parameters
    for (auto* param : audioProcessor.getParameters()) {
        if (auto* pRanged = dynamic_cast<juce::AudioProcessorParameterWithID*>(param)) {
            audioProcessor.treeState.addParameterListener (pRanged->paramID, this);
        }
    }

    setResizable(true, true);
    setSize (1200, 800);
    startTimerHz(30);
}

MushinAudioProcessorEditor::~MushinAudioProcessorEditor()
{
    stopTimer();
    for (auto* param : audioProcessor.getParameters()) {
        if (auto* pRanged = dynamic_cast<juce::AudioProcessorParameterWithID*>(param)) {
            audioProcessor.treeState.removeParameterListener (pRanged->paramID, this);
        }
    }
}

void MushinAudioProcessorEditor::syncAllParameters() {
    for (auto* param : audioProcessor.getParameters()) {
        if (auto* pRanged = dynamic_cast<juce::AudioProcessorParameterWithID*>(param)) {
            // parameterChanged expects the PLAIN/REAL value (e.g. Hz), but param->getValue() is normalized 0-1.
            // We MUST convert it back to the real range before calling parameterChanged.
            if (auto* apvtsParam = audioProcessor.treeState.getParameter(pRanged->paramID))
                parameterChanged(pRanged->paramID, apvtsParam->convertFrom0to1(apvtsParam->getValue()));
            else
                parameterChanged(pRanged->paramID, pRanged->getValue());
        }
    }
}

void MushinAudioProcessorEditor::paint (juce::Graphics& g) { g.fillAll (juce::Colours::black); }
void MushinAudioProcessorEditor::resized() { webComponent.setBounds (getLocalBounds()); }

void MushinAudioProcessorEditor::parameterChanged (const juce::String& parameterID, float newValue)
{
    auto* param = audioProcessor.treeState.getParameter (parameterID);
    if (!param) return;
    auto normalizedValue = param->convertTo0to1 (newValue);
    juce::String js = "if (window.setParameterValue) window.setParameterValue('" + parameterID + "', " + juce::String (normalizedValue) + ");";
    juce::MessageManager::callAsync ([this, js] { webComponent.evaluateJavascript (js); });
}

void MushinAudioProcessorEditor::timerCallback()
{
    int numReady = audioProcessor.abstractFifo.getNumReady();
    if (numReady <= 0) return;
    constexpr int numSamplesToVisualise = 256;
    juce::Array<juce::var> waveformData;
    int samplesToRead = std::min(numReady, numSamplesToVisualise);
    int start1, block1, start2, block2;
    audioProcessor.abstractFifo.prepareToRead (samplesToRead, start1, block1, start2, block2);
    for (int i = 0; i < block1; ++i) waveformData.add(audioProcessor.audioFifo[(size_t)(start1 + i)]);
    for (int i = 0; i < block2; ++i) waveformData.add(audioProcessor.audioFifo[(size_t)(start2 + i)]);
    audioProcessor.abstractFifo.finishedRead (block1 + block2);
    if (waveformData.size() > 0) webComponent.emitEventIfBrowserIsVisible("waveform", waveformData);
}
