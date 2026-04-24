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
                    }), 
                    p, *this)
{
    addAndMakeVisible (webComponent);
    webComponent.goToURL (juce::WebBrowserComponent::getResourceProviderRoot());

    audioProcessor.treeState.addParameterListener ("gain", this);
    audioProcessor.treeState.addParameterListener ("drive", this);
    audioProcessor.treeState.addParameterListener ("exhaustion", this);
    audioProcessor.treeState.addParameterListener ("threshold", this);
    audioProcessor.treeState.addParameterListener ("cutoff", this);
    audioProcessor.treeState.addParameterListener ("resonance", this);
    audioProcessor.treeState.addParameterListener ("mix", this);

    setResizable(true, true);
    setSize (800, 600);
    startTimerHz(30);
}

MushinAudioProcessorEditor::~MushinAudioProcessorEditor()
{
    stopTimer();
    audioProcessor.treeState.removeParameterListener ("gain", this);
    audioProcessor.treeState.removeParameterListener ("drive", this);
    audioProcessor.treeState.removeParameterListener ("exhaustion", this);
    audioProcessor.treeState.removeParameterListener ("threshold", this);
    audioProcessor.treeState.removeParameterListener ("cutoff", this);
    audioProcessor.treeState.removeParameterListener ("resonance", this);
    audioProcessor.treeState.removeParameterListener ("mix", this);
}

void MushinAudioProcessorEditor::syncAllParameters() {
    auto ids = {"gain", "drive", "exhaustion", "threshold", "cutoff", "resonance", "mix"};
    for (auto id : ids) {
        if (auto* param = audioProcessor.treeState.getParameter(id)) {
            // parameterChanged expects the PLAIN value, but param->getValue() is normalized.
            // We use convertFrom0to1 to get the actual value (Hz, dB, etc.)
            parameterChanged(id, param->convertFrom0to1(param->getValue()));
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
