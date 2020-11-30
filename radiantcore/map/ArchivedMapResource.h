#pragma once

#include "iarchive.h"
#include "MapResource.h"
#include "stream/MapResourceStream.h"

namespace map
{

/**
 * MapResource specialising on loading map files from archives
 * just as PK4 files, which may be located outside the VFS search
 * paths.
 * 
 * ArchivedMapResources are read-only and don't implement the save()
 * methods.
 */
class ArchivedMapResource :
    public MapResource
{
private:
    std::string _archivePath;
    std::string _filePathWithinArchive;

    IArchive::Ptr _archive;

public:
    ArchivedMapResource(const std::string& archivePath, const std::string& filePathWithinArchive);

    virtual bool isReadOnly() override;
    virtual void save(const MapFormatPtr& mapFormat = MapFormatPtr()) override;

protected:
    virtual stream::MapResourceStream::Ptr openMapfileStream() override;
    virtual stream::MapResourceStream::Ptr openInfofileStream() override;

private:
    stream::MapResourceStream::Ptr openFileInArchive(const std::string& filePathWithinArchive);

    void ensureArchiveOpened();
};

}
