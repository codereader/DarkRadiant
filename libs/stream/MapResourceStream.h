#pragma once

#include <memory>
#include <string>
#include <fstream>
#include "os/path.h"
#include "itextstream.h"
#include "ifilesystem.h"

namespace stream
{

/**
 * RAII object representing a file stream used for map loading.
 * It may either refer to a physical file or a VFS file.
 */
class MapResourceStream
{
public:
    using Ptr = std::shared_ptr<MapResourceStream>;

    virtual ~MapResourceStream() {}

    // Returns true if the stream has been successfully opened
    virtual bool isOpen() const = 0;

    // Returns the (seekable) input stream
    virtual std::istream& getStream() = 0;

    // Factory method which will return a stream reference for the given path
    // Will always return a non-empty reference
    static Ptr OpenFromPath(const std::string& path);
};

namespace detail
{

// Stream implementation targeting a physical file
class FileMapResourceStream :
    public MapResourceStream
{
private:
    std::ifstream _stream;

public:
    FileMapResourceStream(const std::string& path)
    {
        rMessage() << "Open file " << path << " from filesystem...";

        _stream.open(path);

        if (!_stream)
        {
            rError() << "failure" << std::endl;
            return;
        }

        rMessage() << "success." << std::endl;
    }

    bool isOpen() const override
    {
        return _stream.good();
    }

    std::istream& getStream() override
    {
        return _stream;
    }
};

/**
 * MapResourceStream implementation working with a VFS file.
 * Since VFS deflated file streams are not seekable, the whole 
 * file contents are pre-loaded into a seekable stringstream.
 */
class VfsMapResourceStream :
    public MapResourceStream
{
private:
    ArchiveTextFilePtr _vfsFile;

    std::stringstream _contentStream;

public:
    VfsMapResourceStream(const std::string& path)
    {
        rMessage() << "Trying to open file " << path << " from VFS...";

        _vfsFile = GlobalFileSystem().openTextFile(path);

        if (!_vfsFile)
        {
            rError() << "failure" << std::endl;
            return;
        }

        rMessage() << "success." << std::endl;

        std::istream vfsStream(&(_vfsFile->getInputStream()));

        // Load everything into one large string
        _contentStream << vfsStream.rdbuf();
    }

    bool isOpen() const override
    {
        return _vfsFile != nullptr;
    }

    std::istream& getStream() override
    {
        return _contentStream;
    }
};

}

inline MapResourceStream::Ptr MapResourceStream::OpenFromPath(const std::string& path)
{
    if (path_is_absolute(path.c_str()))
    {
        return std::make_shared<detail::FileMapResourceStream>(path);
    }
    else
    {
        // Not an absolute path, might as well be a VFS path, so try to load it from the PAKs
        return std::make_shared<detail::VfsMapResourceStream>(path);
    }
}

}
