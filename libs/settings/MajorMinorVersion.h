#pragma once

#include <string>

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
        // Extract the version from the given string
        _majorVersion = 0;
        _minorVersion = 0;
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
};

}
