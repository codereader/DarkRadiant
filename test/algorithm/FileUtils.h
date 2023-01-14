#pragma once

#include <string>
#include <fstream>
#include "ifilesystem.h"
#include "os/fs.h"
#include "string/replace.h"

namespace test
{

class BackupCopy
{
private:
    fs::path _originalFile;
    fs::path _backupFile;
public:
    BackupCopy(const fs::path& originalFile) :
        _originalFile(originalFile)
    {
        _backupFile = _originalFile;
        _backupFile.replace_extension("bak");

        if (fs::exists(_backupFile))
        {
            fs::remove(_backupFile);
        }

        fs::copy(_originalFile, _backupFile);
    }

    ~BackupCopy()
    {
        fs::remove(_originalFile);
        fs::rename(_backupFile, _originalFile);
    }

    // Restore the original file, keep the backup
    void restoreNow()
    {
        fs::remove(_originalFile);
        fs::copy(_backupFile, _originalFile);
    }
};

namespace algorithm
{

// Writes the given contents to the given path, overwriting it
inline void replaceFileContents(const std::string& path, const std::string& contents)
{
    std::ofstream stream(path, std::ofstream::out);
    stream << contents;
    stream.flush();
    stream.close();
}

// Loads the entire text from the given vfs file into a string
inline std::string loadTextFromVfsFile(const std::string& vfsPath)
{
    auto file = GlobalFileSystem().openTextFile(vfsPath);

    if (!file)
    {
        return std::string();
    }

    std::stringstream textStream;
    std::istream mapStream(&file->getInputStream());
    textStream << mapStream.rdbuf();
    textStream.flush();

    return textStream.str();
}

// Returns the text contents of the given file
inline std::string loadFileToString(const fs::path& path)
{
    std::stringstream contentStream;
    std::ifstream input(path);

    contentStream << input.rdbuf();

    return contentStream.str();
}

// True if the file (full physical path) contains the given text (ignoring CRLF vs. LF line break differences)
inline bool fileContainsText(const fs::path& path, const std::string& textToFind)
{
    std::stringstream contentStream;
    std::ifstream input(path);

    contentStream << input.rdbuf();

    std::string contents = string::replace_all_copy(contentStream.str(), "\r\n", "\n");

    return contents.find(textToFind) != std::string::npos;
}

}

}
