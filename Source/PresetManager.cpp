// Source/PresetManager.cpp
#include "PresetManager.h"

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
    juce::XmlDocument xml(apvts.state.toXmlString());
    xml.setAttribute ("name",   name);
    xml.setAttribute ("version","1");

    if (!xml.writeTo(file))
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

    auto ver = xml->getStringAttribute ("version", "0");
    if (ver != "1")
    {
        result = juce::Result::fail("Unsupported preset version");
        return false;
    }

    apvts.state = juce::ValueTree::fromXml(*xml);
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

    // Factory presets (BinaryData) – placeholder: add names manually if known.
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
