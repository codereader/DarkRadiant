#pragma once

#include "iarchive.h"
#include <git2.h>
#include "stream/BufferInputStream.h"

namespace vcs
{

namespace git
{

// Adapter class, implementing the ArchiveTextFile interface to a Git BLOB
class GitArchiveTextFile :
    public ArchiveTextFile
{
private:
    git_blob* _blob;
    std::size_t _blobSize;
    const void* _blobContents;

    std::string _path;

    stream::BufferInputStream _stream;

public:
    using Ptr = std::shared_ptr<GitArchiveTextFile>;

    GitArchiveTextFile(git_blob* blob, const std::string& path) :
        _blob(blob),
        _blobSize(git_blob_rawsize(_blob)),
        _blobContents(git_blob_rawcontent(_blob)),
        _path(path),
        _stream(reinterpret_cast<const char*>(_blobContents), _blobSize)
    {}

    ~GitArchiveTextFile()
    {
        git_blob_free(_blob);
    }

    std::string getModName() const override
    {
        return "Git History";
    }

    const std::string& getName() const override
    {
        return _path;
    }

    TextInputStream& getInputStream() override
    {
        return _stream;
    }
};

}

}
