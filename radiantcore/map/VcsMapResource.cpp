#include "VcsMapResource.h"

#include "i18n.h"
#include "iversioncontrol.h"
#include "VersionControlLib.h"

namespace map
{

VcsMapResource::VcsMapResource(const std::string& uri) :
    MapResource(uri),
    _uri(uri)
{}

bool VcsMapResource::isReadOnly()
{
    return true;
}

void VcsMapResource::save(const MapFormatPtr&)
{
    assert(false);
    rError() << "VcsMapResources cannot be saved." << std::endl;
}

stream::MapResourceStream::Ptr VcsMapResource::openMapfileStream()
{
    return stream::MapResourceStream::Ptr();
}

stream::MapResourceStream::Ptr VcsMapResource::openInfofileStream()
{
    //ensureArchiveOpened();

    try
    {
        //auto infoFilename = _filePathWithinArchive.substr(0, _filePathWithinArchive.rfind('.'));
        //infoFilename += GetInfoFileExtension();

        return stream::MapResourceStream::Ptr();
    }
    catch (const OperationException& ex)
    {
        // Info file load file does not stop us, just issue a warning
        rWarning() << ex.what() << std::endl;
        return stream::MapResourceStream::Ptr();
    }
}

stream::MapResourceStream::Ptr VcsMapResource::openFileFromVcs(const std::string& filePathWithinArchive)
{
    //auto archiveFile = _archive->openTextFile(filePathWithinArchive);
    //
    //if (!archiveFile)
    //{
    //    throw OperationException(fmt::format(_("Could not open file in archive: {0}"), _archivePath));
    //}

    //return stream::MapResourceStream::OpenFromArchiveFile(archiveFile);
    return stream::MapResourceStream::Ptr();
}

}
