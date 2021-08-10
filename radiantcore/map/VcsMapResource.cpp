#include "VcsMapResource.h"

#include "i18n.h"
#include "iversioncontrol.h"
#include "VersionControlLib.h"
#include "stream/VcsMapResourceStream.h"
#include "os/fs.h"
#include "gamelib.h"
#include <fmt/format.h>

namespace map
{

VcsMapResource::VcsMapResource(const std::string& mapFileUri) :
    MapResource(vcs::getVcsFilePath(mapFileUri)),
    _mapFileUri(mapFileUri)
{
    assert(vcs::pathIsVcsUri(_mapFileUri));

    auto prefix = vcs::getVcsPrefix(_mapFileUri);
    _vcsModule = GlobalVersionControlManager().getModuleForPrefix(prefix);

    if (!_vcsModule)
    {
        rWarning() << "Unrecognised VCS URI prefix: " << prefix << std::endl;
    }

    auto infoFilePath = vcs::getVcsFilePath(_mapFileUri);
    infoFilePath = os::replaceExtension(infoFilePath, game::current::getInfoFileExtension());

    _infoFileUri = vcs::constructVcsFileUri(prefix, vcs::getVcsRevision(_mapFileUri), infoFilePath);
}

bool VcsMapResource::load()
{
    auto result = MapResource::load();

    if (result)
    {
        // Set the name string to contain the revision of the map
        auto mapName = fmt::format("{0}@{1}", os::getFilename(vcs::getVcsFilePath(_mapFileUri)), 
            vcs::getVcsRevision(_mapFileUri).substr(0, 7));
        getRootNode()->setName(mapName);
    }

    return result;
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
    return openFileFromVcs(_mapFileUri);
}

stream::MapResourceStream::Ptr VcsMapResource::openInfofileStream()
{
    return openFileFromVcs(_infoFileUri);
}

stream::MapResourceStream::Ptr VcsMapResource::openFileFromVcs(const std::string& uri)
{
    if (!_vcsModule || !vcs::pathIsVcsUri(uri))
    {
        return stream::MapResourceStream::Ptr();
    }

    return stream::VcsMapResourceStream::OpenFromVcs(*_vcsModule, uri);
}

}
