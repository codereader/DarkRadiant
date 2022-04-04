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
    // Construct the major/minor version from the given version string,
    // as used in the RADIANT_VERSION pattern: "major.minor.micro[suffix]"
    // Throws std::invalid_argument in case of parsing failures
    MajorMinorVersion(const std::string& versionString)
    {
        // Major/minor version is mandatory, micro version and suffix are optional
        const std::regex VersionPattern("(\\d+)\\.(\\d+)(\\.\\d+[\\w\\d_]*)?");

        // Extract the version from the given string
        std::smatch match;
        if (!std::regex_match(versionString, match, VersionPattern))
        {
            throw std::invalid_argument("The input string " + versionString + " failed to parse");
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
