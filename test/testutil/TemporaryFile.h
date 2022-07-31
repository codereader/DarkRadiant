#pragma once

#include <string>
#include <fstream>
#include "os/file.h"
#include "../algorithm/FileUtils.h"

namespace test
{

// File object removing itself when going out of scope
class TemporaryFile
{
private:
    std::string _path;

public:
    // Set up the auto-destruct sequence for the file in the given path
    // Doesn't create the file yet, this is done in setContents() at the latest
    TemporaryFile(const std::string& path) :
        _path(path)
    {}

    // Construct and immediately create the file with the given contents
    TemporaryFile(const std::string& path, const std::string& contents) :
        TemporaryFile(path)
    {
        setContents(contents);
    }

    void setContents(const std::string& contents)
    {
        algorithm::replaceFileContents(_path, contents);
    }

    ~TemporaryFile()
    {
        if (fs::exists(_path))
        {
            fs::remove(_path);
        }
    }
};

}
