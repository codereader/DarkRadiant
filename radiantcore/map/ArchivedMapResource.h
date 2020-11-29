#pragma once

#include "i18n.h"
#include "ifilesystem.h"
#include "MapResource.h"
#include "stream/MapResourceStream.h"

namespace map
{

class ArchivedMapResource :
    public MapResource
{
private:
    std::string _archivePath;
    std::string _filePathWithinArchive;

    IArchive::Ptr _archive;

public:
    ArchivedMapResource(const std::string& archivePath, const std::string& filePathWithinArchive) :
        MapResource(filePathWithinArchive),
        _archivePath(archivePath),
        _filePathWithinArchive(filePathWithinArchive)
    {}

    virtual bool isReadOnly() override
    {
        return true;
    }

protected:
    virtual stream::MapResourceStream::Ptr openMapfileStream() override
    {
        ensureArchiveOpened();

        return openFileInArchive(_filePathWithinArchive);
    }

    virtual stream::MapResourceStream::Ptr openInfofileStream() override
    {
        ensureArchiveOpened();

        try
        {
            auto infoFilename = _filePathWithinArchive.substr(0, _filePathWithinArchive.rfind('.'));
            infoFilename += GetInfoFileExtension();

            return openFileInArchive(infoFilename);
        }
        catch (const OperationException& ex)
        {
            // Info file load file does not stop us, just issue a warning
            rWarning() << ex.what() << std::endl;
            return stream::MapResourceStream::Ptr();
        }
    }

private:
    stream::MapResourceStream::Ptr openFileInArchive(const std::string& filePathWithinArchive)
    {
        assert(_archive);

        auto archiveFile = _archive->openTextFile(filePathWithinArchive);

        if (!archiveFile)
        {
            throw OperationException(fmt::format(_("Could not open file in archive: {0}"), _archivePath));
        }

        return stream::MapResourceStream::OpenFromArchiveFile(archiveFile);
    }

    void ensureArchiveOpened()
    {
        if (_archive)
        {
            return;
        }

        _archive = GlobalFileSystem().openArchiveInAbsolutePath(_archivePath);

        if (!_archive)
        {
            throw OperationException(fmt::format(_("Could not open archive: {0}"), _archivePath));
        }
    }
};

}
