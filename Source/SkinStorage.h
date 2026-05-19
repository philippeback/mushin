#pragma once

#include <juce_core/juce_core.h>

/**
    Simple helper that stores the selected UI skin in a persistent
    properties file separate from any preset data.

    The file is written to the user‑specific application‑data directory,
    e.g.:
        macOS   : ~/Library/Application Support/MushinPlugin/settings.ini
        Windows: %APPDATA%\MushinPlugin\settings.ini

    Usage:

        // Load saved skin index (default = 0 → “Industrial”)
        int savedIndex = SkinStorage::getSavedSkinIndex();

        // When the user selects a new skin:
        SkinStorage::setSavedSkinIndex (newIndex);
*/
class SkinStorage
{
public:
    /** Returns the singleton PropertiesFile used for storing UI settings. */
    static juce::PropertiesFile& getFile()
    {
        static juce::PropertiesFile props (
            juce::PropertiesFile::Options()
                .withFilename (juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
                               .getChildFile ("MushinPlugin")
                               .getChildFile ("settings.ini"))
                .withCommonPreferencesFile (false)   // we only need our own file
                .withIgnoreCaseOfKeyNames (true));

        return props;
    }

    /** Retrieves the saved skin index.
        Returns 0 if no value has been stored yet, which corresponds to the default
        “Industrial” skin. */
    static int getSavedSkinIndex()
    {
        return getFile().getIntValue ("skin", 0);
    }

    /** Saves the supplied skin index to disk. */
    static void setSavedSkinIndex (int index)
    {
        getFile().setValue ("skin", index);
        getFile().saveIfNeeded();   // writes the file if it has changed
    }
};
