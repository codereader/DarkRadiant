#pragma once

#include <string>
#include "version.h"
#include "imodule.h"

#include "MajorMinorVersion.h"
#include "os/dir.h"
#include "os/path.h"

namespace settings
{

class SettingsManager
{
private:
    const IApplicationContext& _context;
    MajorMinorVersion _currentVersion;

    // The path where this version writes its settings files to
    std::string _currentVersionSettingsFolder;

public:
    // Construct a settings manager for this version of DarkRadiant
    // Will create the settings path for this version if it's not existing yet.
    SettingsManager(const IApplicationContext& context) :
        SettingsManager(context, RADIANT_VERSION)
    {}

    SettingsManager(const SettingsManager& other) = delete;
    SettingsManager& operator=(const SettingsManager& other) = delete;

    // Construct a settings manager instance with a specific version
    // string in the format "Major.Minor.Micro[BuildSuffix]", or "2.14.0pre1".
    // Mainly used for unit test purposes, regular code should use the default constructor.
    SettingsManager(const IApplicationContext& context, const std::string& currentVersion) :
        _context(context),
        _currentVersion(currentVersion)
    {
        // Set up the path to the current version
        _currentVersionSettingsFolder = os::standardPathWithSlash(context.getSettingsPath() + _currentVersion.toString());

        // Make sure the output folder exists
        os::makeDirectory(_currentVersionSettingsFolder);
    }

    // Returns the output path (including trailing slash) where all settings files
    // for the current version can be saved to.
    // Matches the pattern IApplicationContext::getSettingsPath()/"major.minor"/
    const std::string& getCurrentVersionSettingsFolder() const
    {
        return _currentVersionSettingsFolder;
    }
};

}
