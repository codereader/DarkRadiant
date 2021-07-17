#include "VcsMapResource.h"

#include "i18n.h"
#include "iversioncontrol.h"
#include "VersionControlLib.h"
#include "os/fs.h"

namespace map
{

VcsMapResource::VcsMapResource(const std::string& uri) :
    MapResource(uri),
    _uri(uri)
{
    assert(vcs::pathIsVcsUri(uri));

    auto prefix = vcs::getVcsPrefix(uri);
    _vcsModule = GlobalVersionControlManager().getModuleForPrefix(prefix);

    if (!_vcsModule)
    {
        rWarning() << "Unrecognised VCS URI prefix: " << prefix << std::endl;
    }

    _revision = vcs::getVcsRevision(uri);
    _filePath = vcs::getVcsFilePath(uri);
}

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
    return openFileFromVcs(_revision, _filePath);
}

stream::MapResourceStream::Ptr VcsMapResource::openInfofileStream()
{
    if (!_vcsModule) return stream::MapResourceStream::Ptr();

    try
    {
        auto infoFilename = os::replaceExtension(_filePath, GetInfoFileExtension());
        return openFileFromVcs(_revision, infoFilename);
    }
    catch (const OperationException& ex)
    {
        // Info file load file failure does not stop us, just issue a warning
        rWarning() << ex.what() << std::endl;
        return stream::MapResourceStream::Ptr();
    }
}

stream::MapResourceStream::Ptr VcsMapResource::openFileFromVcs(const std::string& revision, const std::string& filePath)
{
    if (!_vcsModule || revision.empty() || filePath.empty())
    {
        return stream::MapResourceStream::Ptr();
    }

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
