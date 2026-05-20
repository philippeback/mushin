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
    /** Returns the theme name that was saved previously.
        If no value exists yet, returns "Industrial". */
    static juce::String getSavedSkinName()
    {
        return getFile().getValue ("themeName", "Industrial");
    }

    /** Saves the supplied theme name to disk. */
    static void setSavedSkinName (const juce::String& name)
    {
        getFile().setValue ("themeName", name);
        bool ok = getFile().saveIfNeeded();

        // Debug output – harmless in release builds
        juce::Logger::writeToLog ("[Skin] Saved theme: " + name +
                                  (ok ? " (file written)" : " (no change)"));
    }

private:
    /** Returns a reference to the singleton PropertiesFile used for storage.
        The folder is created automatically if it does not exist. */
    static juce::PropertiesFile& getFile()
    {
        static std::unique_ptr<juce::PropertiesFile> props;
        
        if (props == nullptr)
        {
            auto dir = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
                         .getChildFile ("MushinPlugin");
            
            if (! dir.exists())
                dir.createDirectory();

            juce::PropertiesFile::Options opts;
            opts.applicationName = "MushinPlugin";
            opts.filenameSuffix = ".ini";
            opts.folderName = "MushinPlugin";
            opts.commonToAllUsers = false;
            opts.ignoreCaseOfKeyNames = true;
            opts.osxLibrarySubFolder = "Application Support";
            
            props = std::make_unique<juce::PropertiesFile> (dir.getChildFile ("settings.ini"), opts);
        }

        return *props;
    }
};

} // namespace mushin
