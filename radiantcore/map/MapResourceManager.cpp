#include "MapResourceManager.h"

#include "ifilesystem.h"
#include "ifiletypes.h"
#include "itextstream.h"
#include "os/path.h"
#include "module/StaticModule.h"
#include "MapResource.h"
#include "ArchivedMapResource.h"
#include "VersionControlLib.h"
#include "VcsMapResource.h"

namespace map
{

IMapResourcePtr MapResourceManager::createFromPath(const std::string& path)
{
    if (vcs::pathIsVcsUri(path))
    {
        return std::make_shared<VcsMapResource>(path);
    }

	return std::make_shared<MapResource>(path);
}

IMapResourcePtr MapResourceManager::createFromArchiveFile(const std::string& archivePath,
    const std::string& filePathWithinArchive)
{
    return std::make_shared<ArchivedMapResource>(archivePath, filePathWithinArchive);
}

MapResourceManager::ExportEvent& MapResourceManager::signal_onResourceExporting()
{
	return _resourceExporting;
}

MapResourceManager::ExportEvent& MapResourceManager::signal_onResourceExported()
{
	return _resourceExported;
}

const std::string& MapResourceManager::getName() const
{
	static std::string _name(MODULE_MAPRESOURCEMANAGER);
	return _name;
}

const StringSet& MapResourceManager::getDependencies() const
{
	static StringSet _dependencies;

	if (_dependencies.empty())
	{
		_dependencies.insert(MODULE_VIRTUALFILESYSTEM);
		_dependencies.insert(MODULE_FILETYPES);
		_dependencies.insert("Doom3MapLoader");
	}

	return _dependencies;
}

void MapResourceManager::initialiseModule(const IApplicationContext& ctx)
{
	rMessage() << getName() << "::initialiseModule called." << std::endl;
}

// Define the MapResourceManager registerable module
module::StaticModuleRegistration<MapResourceManager> mapResourceManagerModule;

} // namespace

