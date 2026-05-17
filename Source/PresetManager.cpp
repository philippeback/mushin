// Source/PresetManager.cpp
#include "PresetManager.h"
#include <memory>

PresetManager::PresetManager(juce::AudioProcessorValueTreeState& state)
    : apvts(state),
      userPresetDir (juce::File::getSpecialLocation(
                         juce::File::userApplicationDataDirectory)
                         .getChildFile("Mushin")
                         .getChildFile("Presets"))
{
    if (!userPresetDir.exists())
        userPresetDir.createDirectory();
}

bool PresetManager::savePreset(const juce::String& name, juce::Result& result)
{
    auto file = getPresetFile(name);

    // Parse the APVTS state into an XmlElement
    std::unique_ptr<juce::XmlElement> vtRoot (juce::XmlDocument::parse(apvts.state.toXmlString()));
    if (!vtRoot)
    {
        result = juce::Result::fail("Failed to serialize state");
        return false;
    }

    // Wrap it in a <Preset> element with name and version attributes
    juce::XmlElement preset ("Preset");
    preset.setAttribute ("name",   name);
    preset.setAttribute ("version","1");
    preset.addChild(vtRoot.release(), -1, nullptr);

    if (!preset.writeTo(file))
    {
        result = juce::Result::fail("Failed to write preset file");
        return false;
    }

    result = juce::Result::ok();
    return true;
}

bool PresetManager::loadPreset(const juce::String& name, juce::Result& result)
{
    auto file = getPresetFile(name);
    if (!file.existsAsFile())
    {
        result = juce::Result::fail("Preset not found");
        return false;
    }

    std::unique_ptr<juce::XmlElement> xml (juce::XmlDocument::parse(file));
    if (!xml)
    {
        result = juce::Result::fail("Failed to parse preset XML");
        return false;
    }

    // The file should contain a <Preset> element
    if (xml->getTagName() != "Preset")
    {
        result = juce::Result::fail("Invalid preset format");
        return false;
    }

    auto ver = xml->getStringAttribute ("version", "0");
    if (ver != "1")
    {
        result = juce::Result::fail("Unsupported preset version");
        return false;
    }

    // The first child of <Preset> should be the ValueTree XML
    auto* vtRoot = xml->getFirstChildElement();
    if (!vtRoot)
    {
        result = juce::Result::fail("Missing ValueTree in preset");
        return false;
    }

    apvts.state = juce::ValueTree::fromXml(*vtRoot);
    // Notify host of parameter changes
    apvts.stateChanged();

    result = juce::Result::ok();
    return true;
}

bool PresetManager::deletePreset(const juce::String& name, juce::Result& result)
{
    auto file = getPresetFile(name);
    if (!file.existsAsFile())
    {
        result = juce::Result::fail("Preset not found");
        return false;
    }

    if (!file.deleteFile())
    {
        result = juce::Result::fail("Failed to delete preset");
        return false;
    }
    result = juce::Result::ok();
    return true;
}

juce::Array<juce::String> PresetManager::getPresetList() const
{
    juce::Array<juce::String> list;

    // Factory presets – add known names here (if any)
    // Example:
    // list.add ("FactoryPreset1");
    // list.add ("FactoryPreset2");

    // User presets
    auto files = userPresetDir.findChildFiles(juce::File::findFiles, false, "*.xml");
    for (auto& f : files)
        list.add(f.getFileNameWithoutExtension());

    return list;
}

juce::File PresetManager::getPresetFile(const juce::String& name) const
{
    return userPresetDir.getChildFile(name + ".xml");
}
