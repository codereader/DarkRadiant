#pragma once

#include <string>
#include <fstream>
#include <stdexcept>
#include <fmt/format.h>

#include "itextstream.h"

#include "os/fs.h"
#include "os/path.h"
#include "os/file.h"

namespace stream
{

/**
 * Stream object used to write data to the a given target directory and filename. 
 * To prevent corruption of a possibly existing target file, it will open a stream
 * to a temporary file for writing first. On calling close(), the temporary stream 
 * will be finalised and the temporary file will be moved over to the target file,
 * which in turn will be renamed to .bak first.
 */
class ExportStream
{
private:
    fs::path _tempFile;
    std::ofstream _tempStream;
    std::string _outputDirectory;
    std::string _filename;

public:
    // Output stream mode
    enum class Mode
    {
        Text, 
        Binary,
    };

    ExportStream(const std::string& outputDirectory, const std::string& filename, Mode mode = Mode::Text) :
        ExportStream(outputDirectory, filename, mode == Mode::Binary ? std::ios::out | std::ios::binary : std::ios::out)
    {}

    ExportStream(const std::string& outputDirectory, const std::string& filename, std::ios::openmode mode) :
        _outputDirectory(outputDirectory),
        _filename(filename)
    {
        if (!path_is_absolute(_outputDirectory.c_str()))
        {
            throw std::runtime_error(fmt::format(_("Path is not absolute: {0}"), _outputDirectory));
        }

        fs::path targetPath = _outputDirectory;

        if (!fs::exists(targetPath))
        {
            rMessage() << "Creating directory: " << targetPath << std::endl;
            fs::create_directories(targetPath);
        }

        // Open a temporary file (leading underscore)
        _tempFile = targetPath / ("_" + _filename);

        _tempStream = std::ofstream(_tempFile.string(), mode);

        if (!_tempStream.is_open())
        {
            throw std::runtime_error(
                fmt::format(_("Cannot open file for writing: {0}"), _tempFile.string()));
        }
    }

    // Returns the stream for writing the export data
    std::ofstream& getStream()
    {
        return _tempStream;
    }

    void close()
    {
        _tempStream.close();

        // The full OS path to the output file
        fs::path targetPath = _outputDirectory;
        targetPath /= _filename;

        if (fs::exists(targetPath) && !os::moveToBackupFile(targetPath))
        {
            throw std::runtime_error(
                fmt::format(_("Could not rename the existing file to .bak: {0}"), targetPath.string()));
        }

        try
        {
            fs::rename(_tempFile, targetPath);
        }
        catch (fs::filesystem_error& e)
        {
            rError() << "Could not rename the temporary file " << _tempFile.string() << std::endl
                << e.what() << std::endl;

            throw std::runtime_error(
                fmt::format(_("Could not rename the temporary file: {0}"), _tempFile.string()));
        }
    }
};

}
