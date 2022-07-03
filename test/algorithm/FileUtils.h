#pragma once

#include <string>
#include "ifilesystem.h"

namespace test
{

namespace algorithm
{

// Loads the entire text from the given vfs file into a string
inline std::string loadTextFromVfsFile(const std::string& vfsPath)
{
    auto file = GlobalFileSystem().openTextFile(vfsPath);

    std::stringstream textStream;
    std::istream mapStream(&file->getInputStream());
    textStream << mapStream.rdbuf();
    textStream.flush();

    return textStream.str();
}

}

}
