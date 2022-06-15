#include "ArchivedMapResource.h"

#include "i18n.h"
#include "ifilesystem.h"
#include "gamelib.h"

namespace map
{

ArchivedMapResource::ArchivedMapResource(const std::string& archivePath, const std::string& filePathWithinArchive) :
    MapResource(filePathWithinArchive),
    _archivePath(archivePath),
    _filePathWithinArchive(filePathWithinArchive)
{}

bool ArchivedMapResource::isReadOnly()
{
    return true;
}

void ArchivedMapResource::save(const MapFormatPtr& mapFormat)
{
    assert(false);
    rError() << "ArchivedMapResources cannot be saved." << std::endl;
}

stream::MapResourceStream::Ptr ArchivedMapResource::openMapfileStream()
{
    ensureArchiveOpened();

    return openFileInArchive(_filePathWithinArchive);
}

stream::MapResourceStream::Ptr ArchivedMapResource::openInfofileStream()
{
    ensureArchiveOpened();

    auto infoFilename = _filePathWithinArchive.substr(0, _filePathWithinArchive.rfind('.'));
    infoFilename += game::current::getInfoFileExtension();

    try
    {
        return openFileInArchive(infoFilename);
    }
    catch (const OperationException& ex)
    {
        // Info file load file does not stop us, just issue a warning
        rWarning() << "Could not locate info file " << infoFilename << ": " << ex.what() << std::endl;
        return stream::MapResourceStream::Ptr();
    }
}

stream::MapResourceStream::Ptr ArchivedMapResource::openFileInArchive(const std::string& filePathWithinArchive)
{
    assert(_archive);

    auto archiveFile = _archive->openTextFile(filePathWithinArchive);

    if (!archiveFile)
    {
        throw OperationException(fmt::format(_("Could not open file in archive: {0}"), _archivePath));
    }

    return stream::MapResourceStream::OpenFromArchiveFile(archiveFile);
}

void ArchivedMapResource::ensureArchiveOpened()
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

}
