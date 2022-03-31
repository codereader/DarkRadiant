#pragma once

#include <string>
#include "version.h"
#include "imodule.h"

#include "MajorMinorVersion.h"

namespace settings
{

class SettingsManager
{
private:
    const IApplicationContext& _context;
    MajorMinorVersion _currentVersion;

public:
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
    {}

    // TODO
};

}
