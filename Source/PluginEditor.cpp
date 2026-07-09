// Source/PluginEditor.cpp
#include "PluginEditor.h"
#include "PluginProcessor.h"
#include "PresetManager.h"
#include "SkinStorage.h"
#include <JuceHeader.h>
#include <memory>


void MushinAudioProcessorEditor::MushinWebComponent::handleCustomUrl(
    const juce::String &url) {
  juce::Logger::writeToLog("handleCustomUrl: " + url);
  auto cmd = url.substring(9);
  if (cmd.startsWith("setParameterValue?")) {
    auto params = cmd.substring(18);
    auto paramID = params.upToFirstOccurrenceOf("&", false, false)
                       .fromFirstOccurrenceOf("id=", false, false);
    auto valStr = params.fromFirstOccurrenceOf("val=", false, false);
    float value = valStr.getFloatValue();

    processor.lastParamId = paramID;
    processor.lastUiValue.store(value);
    if (auto *param = processor.treeState.getParameter(paramID))
      param->setValueNotifyingHost(value);
  } else {
    juce::Component::SafePointer<MushinWebComponent> safeThis(this);
    if (cmd.startsWith("savePreset?")) {
      auto name = juce::URL::removeEscapeChars(
          cmd.fromFirstOccurrenceOf("name=", false, false));
      juce::Logger::writeToLog("Bridge: savePreset '" + name + "'");
      juce::Result r = juce::Result::ok();
      if (editor.presetMgr && editor.presetMgr->savePreset(name, r))
        juce::MessageManager::callAsync(
            [safeThis] {
              if (safeThis != nullptr)
                safeThis->evaluateJavascript("onPresetSaved();");
            });
      else
        juce::MessageManager::callAsync([safeThis, r] {
          if (safeThis != nullptr)
            safeThis->evaluateJavascript("onPresetError('" + r.getErrorMessage() + "');");
        });
    } else if (cmd.startsWith("loadPreset?")) {
      auto name = juce::URL::removeEscapeChars(
          cmd.fromFirstOccurrenceOf("name=", false, false));
      juce::Logger::writeToLog("Bridge: loadPreset '" + name + "'");
      juce::Result r = juce::Result::ok();
      if (editor.presetMgr && editor.presetMgr->loadPreset(name, r))
        juce::MessageManager::callAsync(
            [safeThis] {
              if (safeThis != nullptr)
                safeThis->evaluateJavascript("onPresetLoaded();");
            });
      else
        juce::MessageManager::callAsync([safeThis, r] {
          if (safeThis != nullptr)
            safeThis->evaluateJavascript("onPresetError('" + r.getErrorMessage() + "');");
        });
    } else if (cmd.startsWith("deletePreset?")) {
      auto name = juce::URL::removeEscapeChars(
          cmd.fromFirstOccurrenceOf("name=", false, false));
      juce::Logger::writeToLog("Bridge: deletePreset '" + name + "'");
      juce::Result r = juce::Result::ok();
      if (editor.presetMgr && editor.presetMgr->deletePreset(name, r))
        juce::MessageManager::callAsync(
            [safeThis] {
              if (safeThis != nullptr)
                safeThis->evaluateJavascript("onPresetDeleted();");
            });
      else
        juce::MessageManager::callAsync([safeThis, r] {
          if (safeThis != nullptr)
            safeThis->evaluateJavascript("onPresetError('" + r.getErrorMessage() + "');");
        });
    } else if (cmd == "requestPresetList") {
      juce::Logger::writeToLog("Bridge: requestPresetList");
      if (editor.presetMgr) {
        auto list = editor.presetMgr->getPresetList();
        juce::Array<juce::var> jsArray;
        for (auto &n : list)
          jsArray.add(n);
        juce::String json = juce::JSON::toString(jsArray);
        juce::MessageManager::callAsync([safeThis, json] {
          if (safeThis != nullptr)
            safeThis->evaluateJavascript("onPresetListReceived(" + json + ");");
        });
      }
    } else if (cmd.startsWith("setTheme?")) {
      auto name = juce::URL::removeEscapeChars(
          cmd.fromFirstOccurrenceOf("name=", false, false));
      juce::Logger::writeToLog("Bridge: setTheme '" + name + "'");
      editor.currentTheme = name;
      mushin::SkinStorage::setSavedSkinName(name); // Persist selection globally
      juce::MessageManager::callAsync([safeThis] {
        if (safeThis != nullptr)
          safeThis->evaluateJavascript("document.querySelector('link[href^=\"skin.css\"]')."
                             "href = 'skin.css?t=' + new Date().getTime();");
      });
    }
  }
}

void MushinAudioProcessorEditor::MushinWebComponent::pageFinishedLoading(
    const juce::String &url) {
  juce::Logger::writeToLog("Web Page Finished Loading: " + url);
  editor.syncAllParameters();
  // Second sync after a small delay to ensure JS is ready
  juce::Component::SafePointer<MushinAudioProcessorEditor> safeEditor(&editor);
  juce::Timer::callAfterDelay(500, [safeEditor] {
    if (safeEditor != nullptr)
      safeEditor->syncAllParameters();
  });
}

MushinAudioProcessorEditor::MushinAudioProcessorEditor(MushinAudioProcessor &p)
    : AudioProcessorEditor(&p), audioProcessor(p) {
  juce::Logger::writeToLog("--- Mushin Editor Constructor Started ---");
  
  currentTheme = mushin::SkinStorage::getSavedSkinName();
  juce::Logger::writeToLog("Loaded theme from storage: " + currentTheme);

  setResizable(true, true);
  setSize(1200, 820);

  // Instantiate preset manager
  presetMgr = std::make_unique<PresetManager>(audioProcessor.treeState);

  juce::Component::SafePointer<MushinAudioProcessorEditor> safeEditor(this);
  juce::Timer::callAfterDelay(250, [safeEditor, &p] {
    if (safeEditor == nullptr) {
      juce::Logger::writeToLog("Deferred Init: Editor destroyed before timer fired.");
      return;
    }

    juce::Logger::writeToLog(
        "Deferred Init: Configuring WebBrowserComponent for editor: " + juce::String::toHexString((juce::uint64)safeEditor.getComponent()));

    auto dllFile =
        juce::File::getSpecialLocation(juce::File::currentExecutableFile)
            .getSiblingFile("WebView2Loader.dll");

    juce::Logger::writeToLog(
        "Deferred Init: Resolved dllFile path is: " + dllFile.getFullPathName());

    auto cacheDir =
        juce::File::getSpecialLocation(juce::File::windowsLocalAppData)
            .getChildFile("PhilippeBack")
            .getChildFile("Mushin")
            .getChildFile("WV2_v1");

    if (!cacheDir.exists())
      cacheDir.createDirectory();

    auto options =
        juce::WebBrowserComponent::Options{}
            .withBackend(juce::WebBrowserComponent::Options::Backend::webview2)
            .withWinWebView2Options(
                juce::WebBrowserComponent::Options::WinWebView2{}
                    .withDLLLocation(dllFile)
                    .withUserDataFolder(cacheDir)
                    .withBackgroundColour(juce::Colours::darkgrey))
            .withNativeIntegrationEnabled(true)
            .withNativeFunction(
                "juce_invoke",
                [safeEditor](const juce::Array<juce::var> &args,
                       juce::WebBrowserComponent::NativeFunctionCompletion
                           completion) {
                  juce::Logger::writeToLog("Native: juce_invoke called");
                  if (safeEditor != nullptr && safeEditor->webComponent != nullptr && args.size() >= 1) {
                    auto name = args[0].toString();
                    juce::Logger::writeToLog("Native: dispatching callback: " +
                                             name);
                    juce::var callbackArgs =
                        (args.size() > 1) ? args[1] : juce::var();
                    safeEditor->webComponent->dispatchCallback(name, callbackArgs);
                  } else {
                    juce::Logger::writeToLog(
                        "Native: ERROR - safeEditor/webComponent is null or args empty");
                  }
                  completion({});
                })
            .withResourceProvider([safeEditor](const juce::String &url)
                                       -> std::optional<
                                           juce::WebBrowserComponent::Resource> {
              if (safeEditor == nullptr)
                return std::nullopt;

              auto path = url;
              if (path.contains("?"))
                path = path.upToFirstOccurrenceOf("?", false, false);
              if (path.contains("/"))
                path = path.fromLastOccurrenceOf("/", false, false);

              // Handle dynamic skinning
              if (path == "skin.css") {
                juce::String css = ":root {\n";
                if (safeEditor->currentTheme == "Synthwave") {
                  css +=
                      "--primary: #ff00aa;\n--secondary: "
                      "#00ffff;\n--bg-hardware: #14091a;\n--text-main: "
                      "#ebd6f5;\n--panel-border: #461c5c;\n--marking: "
                      "rgba(255, 0, 255, 0.15);\n--display-bg: "
                      "#200f29;\n--sc-yellow: #ffff00;\n--sc-input: #00ffcc;\n";
                } else if (safeEditor->currentTheme == "Acid") {
                  css +=
                      "--primary: #bfff00;\n--secondary: "
                      "#39ff14;\n--bg-hardware: #091209;\n--text-main: "
                      "#e2ebd2;\n--panel-border: #284d28;\n--marking: "
                      "rgba(191, 255, 0, 0.15);\n--display-bg: "
                      "#122412;\n--sc-yellow: #dfff4f;\n--sc-input: #00ff3c;\n";
                } else if (safeEditor->currentTheme == "Firepits") {
                  css +=
                      "--primary: #ff3c00;\n--secondary: "
                      "#ff6600;\n--bg-hardware: #120906;\n--text-main: "
                      "#f5e6de;\n--panel-border: #4d2010;\n--marking: "
                      "rgba(255, 69, 0, 0.15);\n--display-bg: "
                      "#22120b;\n--sc-yellow: #ffcc00;\n--sc-input: #ff3333;\n";
                } else if (safeEditor->currentTheme == "Ocean Deep") {
                  css +=
                      "--primary: #00f3ff;\n--secondary: "
                      "#00bfff;\n--bg-hardware: #060b1a;\n--text-main: "
                      "#e3f0f5;\n--panel-border: #1d305c;\n--marking: "
                      "rgba(0, 243, 255, 0.15);\n--display-bg: "
                      "#0f1830;\n--sc-yellow: #ccff33;\n--sc-input: #008080;\n";
                } else if (safeEditor->currentTheme == "Ice World") {
                  css +=
                      "--primary: #66e0ff;\n--secondary: "
                      "#a5c1eb;\n--bg-hardware: #0f151c;\n--text-main: "
                      "#ffffff;\n--panel-border: #384c61;\n--marking: "
                      "rgba(102, 224, 255, 0.15);\n--display-bg: "
                      "#1a2430;\n--sc-yellow: #ffff66;\n--sc-input: #e0ffff;\n";
                } else if (safeEditor->currentTheme == "Dark Hellish") {
                  css +=
                      "--primary: #ff2200;\n--secondary: "
                      "#ff4d4d;\n--bg-hardware: #0e0606;\n--text-main: "
                      "#e6cfcf;\n--panel-border: #301414;\n--marking: "
                      "rgba(255, 77, 77, 0.15);\n--display-bg: "
                      "#180a0a;\n--sc-yellow: #ffcc00;\n--sc-input: #990000;\n";
                } else {
                  // Default "Industrial" Theme
                  css +=
                      "--primary: #00bfff;\n--secondary: "
                      "#ff9f00;\n--bg-hardware: #111111;\n--text-main: "
                      "#f0f0f0;\n--panel-border: #333333;\n--marking: "
                      "rgba(255, 255, 255, 0.12);\n--display-bg: "
                      "#1c1c1c;\n--sc-yellow: #ffcc00;\n--sc-input: #00ff66;\n";
                }
                css += "}";

                auto data = css.toRawUTF8();
                auto size = (int)strlen(data);
                return juce::WebBrowserComponent::Resource{
                    std::vector<std::byte>(
                        reinterpret_cast<const std::byte *>(data),
                        reinterpret_cast<const std::byte *>(data) + size),
                    "text/css"};
              }

              auto resourceName =
                  (path.isEmpty() || path == "index.html")
                      ? "index_html"
                      : path.replace(".", "_").replace("-", "_");
              int size = 0;
              if (auto data = BinaryData::getNamedResource(
                      resourceName.toRawUTF8(), size)) {
                return juce::WebBrowserComponent::Resource{
                    std::vector<std::byte>(
                        reinterpret_cast<const std::byte *>(data),
                        reinterpret_cast<const std::byte *>(data) + size),
                    url.contains(".js")
                        ? "application/javascript"
                        : (url.contains(".css") ? "text/css" : "text/html")};
              }
              return std::nullopt;
            });

    juce::Logger::writeToLog(
        "Deferred Init: Instantiating MushinWebComponent...");
    safeEditor->webComponent.reset(new MushinWebComponent(options, p, *safeEditor));

    if (safeEditor->webComponent) {
      juce::Logger::writeToLog(
          "Deferred Init: MushinWebComponent created. Adding to editor.");
      safeEditor->addAndMakeVisible(*safeEditor->webComponent);
      safeEditor->webComponent->setBounds(safeEditor->getLocalBounds());

      // Register preset callbacks
      safeEditor->webComponent->registerCallback(
          "savePreset", [safeEditor](const juce::var &args) {
            if (safeEditor == nullptr || !safeEditor->presetMgr || !safeEditor->webComponent)
              return;
            auto name = args[0].toString();
            juce::Result r = juce::Result::ok();
            if (safeEditor->presetMgr->savePreset(name, r))
              juce::MessageManager::callAsync([safeEditor] {
                if (safeEditor != nullptr && safeEditor->webComponent)
                  safeEditor->webComponent->evaluateJavascript("onPresetSaved();");
              });
            else
              juce::MessageManager::callAsync([safeEditor, r] {
                if (safeEditor != nullptr && safeEditor->webComponent)
                  safeEditor->webComponent->evaluateJavascript("onPresetError('" +
                                                   r.getErrorMessage() + "');");
              });
          });

      safeEditor->webComponent->registerCallback(
          "loadPreset", [safeEditor](const juce::var &args) {
            if (safeEditor == nullptr || !safeEditor->presetMgr || !safeEditor->webComponent)
              return;
            auto name = args[0].toString();
            juce::Result r = juce::Result::ok();
            if (safeEditor->presetMgr->loadPreset(name, r))
              juce::MessageManager::callAsync([safeEditor] {
                if (safeEditor != nullptr && safeEditor->webComponent)
                  safeEditor->webComponent->evaluateJavascript("onPresetLoaded();");
              });
            else
              juce::MessageManager::callAsync([safeEditor, r] {
                if (safeEditor != nullptr && safeEditor->webComponent)
                  safeEditor->webComponent->evaluateJavascript("onPresetError('" +
                                                   r.getErrorMessage() + "');");
              });
          });

      safeEditor->webComponent->registerCallback(
          "deletePreset", [safeEditor](const juce::var &args) {
            if (safeEditor == nullptr || !safeEditor->presetMgr || !safeEditor->webComponent)
              return;
            auto name = args[0].toString();
            juce::Result r = juce::Result::ok();
            if (safeEditor->presetMgr->deletePreset(name, r))
              juce::MessageManager::callAsync([safeEditor] {
                if (safeEditor != nullptr && safeEditor->webComponent)
                  safeEditor->webComponent->evaluateJavascript("onPresetDeleted();");
              });
            else
              juce::MessageManager::callAsync([safeEditor, r] {
                if (safeEditor != nullptr && safeEditor->webComponent)
                  safeEditor->webComponent->evaluateJavascript("onPresetError('" +
                                                   r.getErrorMessage() + "');");
              });
          });

      safeEditor->webComponent->registerCallback(
          "requestPresetList", [safeEditor](const juce::var &) {
            if (safeEditor == nullptr || !safeEditor->presetMgr || !safeEditor->webComponent)
              return;
            auto list = safeEditor->presetMgr->getPresetList();
            juce::Array<juce::var> jsArray;
            for (auto &n : list)
              jsArray.add(n);
            juce::String json = juce::JSON::toString(jsArray);
            juce::MessageManager::callAsync([safeEditor, json] {
              if (safeEditor != nullptr && safeEditor->webComponent)
                safeEditor->webComponent->evaluateJavascript("onPresetListReceived(" +
                                                 json + ");");
            });
          });

      safeEditor->webComponent->registerCallback("setTheme", [safeEditor](const juce::var &args) {
        if (safeEditor == nullptr || !safeEditor->webComponent)
          return;
        auto name = args[0].toString();
        juce::Logger::writeToLog("Native: setTheme '" + name + "'");
        safeEditor->currentTheme = name;
        juce::MessageManager::callAsync([safeEditor] {
          if (safeEditor != nullptr && safeEditor->webComponent)
            safeEditor->webComponent->evaluateJavascript(
                "document.querySelector('link[href^=\"skin.css\"]').href = "
                "'skin.css?t=' + new Date().getTime();");
        });
      });

      safeEditor->webComponent->goToURL(
          juce::WebBrowserComponent::getResourceProviderRoot());
    } else {
      juce::Logger::writeToLog(
          "Deferred Init: FAILED to create MushinWebComponent.");
    }
  });

  // Add listeners for all parameters
  for (auto *param : audioProcessor.getParameters()) {
    if (auto *pRanged =
            dynamic_cast<juce::AudioProcessorParameterWithID *>(param)) {
      audioProcessor.treeState.addParameterListener(pRanged->paramID, this);
    }
  }

  startTimerHz(30);
  juce::Logger::writeToLog("--- Mushin Editor Constructor Finished ---");
}

MushinAudioProcessorEditor::~MushinAudioProcessorEditor() {
  juce::Logger::writeToLog("--- Mushin Editor Destroyed ---");
  stopTimer();
  for (auto *param : audioProcessor.getParameters()) {
    if (auto *pRanged =
            dynamic_cast<juce::AudioProcessorParameterWithID *>(param)) {
      audioProcessor.treeState.removeParameterListener(pRanged->paramID, this);
    }
  }
  webComponent.reset();
}

void MushinAudioProcessorEditor::syncAllParameters() {
  for (auto *param : audioProcessor.getParameters()) {
    if (auto *pRanged =
            dynamic_cast<juce::AudioProcessorParameterWithID *>(param)) {
      if (auto *apvtsParam =
              audioProcessor.treeState.getParameter(pRanged->paramID))
        parameterChanged(pRanged->paramID,
                         apvtsParam->convertFrom0to1(apvtsParam->getValue()));
      else
        parameterChanged(pRanged->paramID, pRanged->getValue());
    }
  }

  juce::Component::SafePointer<MushinAudioProcessorEditor> safeEditor(this);
  // Sync theme to UI
  juce::MessageManager::callAsync([safeEditor] {
    if (safeEditor != nullptr && safeEditor->webComponent)
      safeEditor->webComponent->evaluateJavascript("if (window.applyThemeFromNative) window.applyThemeFromNative('" + safeEditor->currentTheme + "');");
  });

  // Sync sample rate to UI
  auto currentSR = audioProcessor.getSampleRate();
  juce::MessageManager::callAsync([safeEditor, currentSR] {
    if (safeEditor != nullptr && safeEditor->webComponent)
      safeEditor->webComponent->evaluateJavascript("if (window.setSampleRateFromNative) window.setSampleRateFromNative(" + juce::String(currentSR) + ");");
  });
}

void MushinAudioProcessorEditor::paint(juce::Graphics &g) {
  g.fillAll(juce::Colours::darkgrey);
}
void MushinAudioProcessorEditor::resized() {
  if (webComponent)
    webComponent->setBounds(getLocalBounds());
}

void MushinAudioProcessorEditor::parameterChanged(
    const juce::String &parameterID, float newValue) {
  auto *param = audioProcessor.treeState.getParameter(parameterID);
  if (!param)
    return;
  auto normalizedValue = param->convertTo0to1(newValue);
  juce::String js = "if (window.setParameterValue) window.setParameterValue('" +
                    parameterID + "', " + juce::String(normalizedValue) + ");";
  juce::Component::SafePointer<MushinAudioProcessorEditor> safeEditor(this);
  juce::MessageManager::callAsync([safeEditor, js] {
    if (safeEditor != nullptr && safeEditor->webComponent)
      safeEditor->webComponent->evaluateJavascript(js);
  });
}

void MushinAudioProcessorEditor::timerCallback() {
  // Waveform visualization
  int numReady = audioProcessor.abstractFifo.getNumReady();
  if (numReady > 0) {
    constexpr int numSamplesToVisualise = 256;
    juce::Array<juce::var> waveformData;
    int samplesToRead = std::min(numReady, numSamplesToVisualise);
    int start1, block1, start2, block2;
    audioProcessor.abstractFifo.prepareToRead(samplesToRead, start1, block1,
                                              start2, block2);
    for (int i = 0; i < block1; ++i)
      waveformData.add(audioProcessor.audioFifo[(size_t)(start1 + i)]);
    for (int i = 0; i < block2; ++i)
      waveformData.add(audioProcessor.audioFifo[(size_t)(start2 + i)]);
    audioProcessor.abstractFifo.finishedRead(block1 + block2);
    if (waveformData.size() > 0 && webComponent)
      webComponent->emitEventIfBrowserIsVisible("waveform", waveformData);
  }

  // Sidechain and Limiter meter visualization
  if (webComponent) {
    webComponent->emitEventIfBrowserIsVisible(
        "scMeter", (double)audioProcessor.scMeterLevel.load());
    webComponent->emitEventIfBrowserIsVisible(
        "scPeak", (double)audioProcessor.scInputPeak.load());
    webComponent->emitEventIfBrowserIsVisible(
        "tgStep", (double)audioProcessor.getTranceGateCurrentStep());
    webComponent->emitEventIfBrowserIsVisible(
        "limiterGR", (double)audioProcessor.limiterGainReduction.load());
  }
}
