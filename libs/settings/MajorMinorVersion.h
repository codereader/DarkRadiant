#pragma once

#include <string>
#include <regex>
#include "string/convert.h"

namespace settings
{

// Represents an application version tag with Major and Minor numbers
class MajorMinorVersion
{
private:
    int _majorVersion;
    int _minorVersion;

public:
    MajorMinorVersion(const std::string& versionString)
    {
        const std::regex VersionPattern("(\\d+)\\.(\\d+)\\.[\\w\\d]+");

        // Extract the version from the given string
        std::smatch match;
        if (!std::regex_match(versionString, match, VersionPattern))
        {
            throw std::runtime_error("The input string " + versionString + " failed to parse");
        }

        _majorVersion = string::convert<int>(match[1].str());
        _minorVersion = string::convert<int>(match[2].str());
    }

    int getMajorVersion() const
    {
        return _majorVersion;
    }

    int getMinorVersion() const
    {
        return _minorVersion;
    }

    // Compare this version to the other one, returning true if this is instance is smaller
    bool operator<(const MajorMinorVersion& other) const
    {
        if (_majorVersion < other._majorVersion)
        {
            return true;
        }

        // If major version matches, minor version decides
        if (_majorVersion == other._majorVersion)
        {
            return _minorVersion < other._minorVersion;
        }

        // Major version is larger
        return false;
    }

    std::string toString() const
    {
        return string::to_string(_majorVersion) + "." + string::to_string(_minorVersion);
    }
};

}
