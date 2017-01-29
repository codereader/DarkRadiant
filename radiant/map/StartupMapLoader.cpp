#include "StartupMapLoader.h"

#include "imodule.h"
#include "igl.h"
#include "irender.h"
#include "iregistry.h"
#include "iradiant.h"
#include "Map.h"
#include "ui/mru/MRU.h"
#include "modulesystem/ModuleRegistry.h"

#include "os/path.h"
#include "os/file.h"
#include <boost/filesystem.hpp>

namespace map 
{

StartupMapLoader::StartupMapLoader()
{
	GlobalRadiant().signal_radiantStarted().connect(
		sigc::mem_fun(*this, &StartupMapLoader::onRadiantStartup)
	);
	GlobalRadiant().signal_radiantShutdown().connect(
		sigc::mem_fun(*this, &StartupMapLoader::onRadiantShutdown)
	);
}

void StartupMapLoader::onRadiantStartup()
{
	std::string mapToLoad = "";

    const ApplicationContext::ArgumentList& args(
        module::ModuleRegistry::Instance().getApplicationContext().getCmdLineArgs()
    );

    for (const std::string& candidate : args)
    {
		if (os::getExtension(candidate) != "map") continue;

		// We have a map file, check if it exists (and where)
		boost::filesystem::path mapsPath = GlobalRegistry().get(RKEY_MAP_PATH);

		boost::filesystem::path fullMapPath = mapsPath / candidate;

		// First, look in the regular maps path
		if (boost::filesystem::exists(fullMapPath))
		{
			mapToLoad = fullMapPath.string();
			break;
		}

		// Second, check for mod-relative paths
		fullMapPath = mapsPath.remove_leaf().remove_leaf() / candidate;

		if (boost::filesystem::exists(fullMapPath))
		{
			mapToLoad = fullMapPath.string();
			break;
		}
	}

	if (!mapToLoad.empty())
	{
		loadMapSafe(mapToLoad);
	}
	else
	{
		std::string lastMap = GlobalMRU().getLastMapName();

		if (GlobalMRU().loadLastMap() && !lastMap.empty() && os::fileOrDirExists(lastMap))
		{
			loadMapSafe(lastMap);
		}
		else
		{
			GlobalMap().createNew();
		}
	}
}

void StartupMapLoader::loadMapSafe(const std::string& mapToLoad)
{
	// Check if we have a valid openGL context, otherwise postpone the load
	if (GlobalOpenGL().wxContextValid())
	{
		GlobalMap().load(mapToLoad);
		return;
	}

	// No valid context, subscribe to the extensionsInitialised signal
	GlobalRenderSystem().signal_extensionsInitialised().connect([mapToLoad]()
	{
		GlobalMap().load(mapToLoad);
	});
}

void StartupMapLoader::onRadiantShutdown()
{
	GlobalMRU().saveRecentFiles();
}

} // namespace map
