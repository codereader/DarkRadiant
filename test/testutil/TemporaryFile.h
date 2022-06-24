#pragma once

#include <string>
#include <fstream>
#include "os/file.h"

namespace test
{

// File object removing itself when going out of scope
class TemporaryFile
{
private:
    std::string _path;

public:
    TemporaryFile(const std::string& path, const std::string& contents = "") :
        _path(path)
    {
        setContents(contents);
    }

    void setContents(const std::string& contents)
    {
        std::ofstream stream(_path, std::ofstream::out);
        stream << contents;
        stream.flush();
        stream.close();
    }

    ~TemporaryFile()
    {
        fs::remove(_path);
    }
};

}
