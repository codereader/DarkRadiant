#pragma once

#include "i18n.h"
#include <string>
#include <fstream>
#include "os/fs.h"
#include "fmt/format.h"

namespace stream
{

// An output stream wrapper which opens a temporary file next to the actual target,
// moving the temporary file over the target file on demand.
// If something goes wrong, a std::runtime_error is thrown.
class TemporaryOutputStream
{
private:
    fs::path _targetFile;
    fs::path _temporaryPath;

    std::ofstream _stream;

public:
    TemporaryOutputStream(const fs::path& targetFile) :
        _targetFile(targetFile),
        _temporaryPath(getTemporaryPath(_targetFile)),
        _stream(_temporaryPath)
    {
        if (!_stream.is_open())
        {
            throw std::runtime_error(fmt::format(_("Cannot open file for writing: {0}"), _temporaryPath.string()));
        }
    }

    std::ostream& getStream()
    {
        return _stream;
    }

    void closeAndReplaceTargetFile()
    {
        _stream.close();

        // Move the temporary stream over the actual file, removing the target first
        if (fs::exists(_targetFile))
        {
            try
            {
                fs::remove(_targetFile);
            }
            catch (fs::filesystem_error& e)
            {
                rError() << "Could not remove the file " << _targetFile.string() << std::endl
                    << e.what() << std::endl;

                throw std::runtime_error(fmt::format(_("Could not remove the file {0}"), _targetFile.string()));
            }
        }

        try
        {
            fs::rename(_temporaryPath, _targetFile);
        }
        catch (fs::filesystem_error& e)
        {
            rError() << "Could not rename the temporary file " << _temporaryPath.string() << std::endl
                << e.what() << std::endl;

            throw std::runtime_error(
                fmt::format(_("Could not rename the temporary file {0}"), _temporaryPath.string()));
        }
    }

private:
    static fs::path getTemporaryPath(const fs::path& targetFile)
    {
        fs::path tempFile = targetFile;

        tempFile.remove_filename();
        tempFile /= "_" + targetFile.filename().string();

        return tempFile;
    }
};

}
