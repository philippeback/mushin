#pragma once

#include <juce_core/juce_core.h>

/*
    Simple persistent storage for the UI skin (theme) index.
    The value is saved to a tiny INI file inside the user's application‑data folder:

        ~/Library/Application Support/MushinPlugin/settings.ini   (macOS)
        %APPDATA%\MushinPlugin\settings.ini                     (Windows)

    Usage:
        int idx = SkinStorage::getSavedSkinIndex();          // read on startup
        SkinStorage::setSavedSkinIndex (newIdx);             // write when user changes skin
*/

namespace mushin {

class SkinStorage
{
public:
    /** Returns the index that was saved previously.
        If no value exists yet, returns 0 (the first theme). */
    static int getSavedSkinIndex()
    {
        return static_cast<int> (getFile().getIntValue ("skin", 0));
    }

    /** Saves the supplied index to disk. */
    static void setSavedSkinIndex (int index)
    {
        getFile().setValue ("skin", index);
        bool ok = getFile().saveIfNeeded();

        // Debug output – harmless in release builds
        juce::Logger::writeToLog ("[Skin] Saved index " + juce::String (index) +
                                  (ok ? " (file written)" : " (no change)"));
    }

private:
    /** Returns a reference to the singleton PropertiesFile used for storage.
        The folder is created automatically if it does not exist. */
    static juce::PropertiesFile& getFile()
    {
        static juce::PropertiesFile props (
            juce::PropertiesFile::Options()
                .withFilename (juce::File::getSpecialLocation (
                                   juce::File::userApplicationDataDirectory)
                               .getChildFile ("MushinPlugin")
                               .createDirectory()               // ensure folder exists
                               .getChildFile ("settings.ini"))
                .withCommonPreferencesFile (false)
                .withIgnoreCaseOfKeyNames (true));

        return props;
    }
};

} // namespace mushin
