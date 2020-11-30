#pragma once

#include <map>
#include "ifilesystem.h"
#include "string/predicate.h"
#include "string/split.h"

namespace vfs
{

// Representation of an assets.lst file, containing visibility information for
// assets within a particular folder.
class AssetsList
{
    std::map<std::string, Visibility> _visibilities;

    // Convert visibility string to enum value
    static Visibility toVisibility(const std::string& input)
    {
        if (string::starts_with(input, "hid" /* 'hidden' or 'hide'*/))
        {
            return Visibility::HIDDEN;
        }
        else if (input == "normal")
        {
            return Visibility::NORMAL;
        }
        else
        {
            rWarning() << "AssetsList: failed to parse visibility '" << input
                << "'" << std::endl;
            return Visibility::NORMAL;
        }
    }

public:

    static constexpr const char* FILENAME = "assets.lst";

    // Construct with possible ArchiveTextFile pointer containing an assets.lst
    // file to parse.
    explicit AssetsList(const ArchiveTextFilePtr& inputFile)
    {
        if (inputFile)
        {
            // Read lines from the file
            std::istream stream(&inputFile->getInputStream());
            while (stream.good())
            {
                std::string line;
                std::getline(stream, line);

                // Attempt to parse the line as "asset=visibility"
                std::vector<std::string> tokens;
                string::split(tokens, line, "=");

                // Parsing was a success if we have two tokens
                if (tokens.size() == 2)
                {
                    std::string filename = tokens[0];
                    Visibility v = toVisibility(tokens[1]);
                    _visibilities[filename] = v;
                }
            }
        }
    }

    // Return visibility for a given file
    Visibility getVisibility(const std::string& fileName) const
    {
        auto i = _visibilities.find(fileName);
        if (i == _visibilities.end())
        {
            return Visibility::NORMAL;
        }
        else
        {
            return i->second;
        }
    }
};

}
