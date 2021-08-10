#pragma once

#include "iversioncontrol.h"
#include "itextstream.h"
#include "MapResourceStream.h"

namespace stream
{

// Loading the given archive file into a buffer 
// to serve the contents as MapResourceStream
class VcsMapResourceStream :
    public MapResourceStream
{
private:
    std::stringstream _contentStream;
    bool _opened;

public:
    using Ptr = std::shared_ptr<VcsMapResourceStream>;

    VcsMapResourceStream(const ArchiveTextFilePtr& vcsArchive)
    {
        rMessage() << "Opened text file in VCS: " << vcsArchive->getName() << std::endl;

        // We can't be sure if the returned stream is seekable,
        // so load everything into a seekable string stream
        std::istream vfsStream(&(vcsArchive->getInputStream()));

        _contentStream << vfsStream.rdbuf();
    }

    // Returns true if the stream has been successfully opened
    virtual bool isOpen() const override
    {
        return true;
    }

    // Returns the (seekable) input stream
    virtual std::istream& getStream() override
    {
        return _contentStream;
    }

    // Factory method which will return a stream reference of the given VCS file
    static Ptr OpenFromVcs(vcs::IVersionControlModule& module, const std::string& uri)
    {
        auto file = module.openTextFile(uri);

        if (!file)
        {
            rMessage() << "Could not open file " << uri << " from VCS module " << module.getUriPrefix() << std::endl;
            return Ptr();
        }

        return std::make_shared<VcsMapResourceStream>(file);
    }
};

}
