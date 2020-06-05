#include "StartupMapLoader.h"

#include "imodule.h"
#include "imap.h"
#include "igl.h"
#include "irender.h"
#include "iregistry.h"
#include "igame.h"
#include "iradiant.h"
#include "ui/mru/MRU.h"
#include "module/StaticModule.h"

#include "os/path.h"
#include "os/file.h"

namespace map 
{

void StartupMapLoader::onRadiantStartup()
{
	std::string mapToLoad = "";

    const ApplicationContext::ArgumentList& args(
        module::GlobalModuleRegistry().getApplicationContext().getCmdLineArgs()
    );

    for (const std::string& candidate : args)
    {
		if (os::getExtension(candidate) != "map" && 
			os::getExtension(candidate) != "mapx") continue;

		// We have a map file, check if it exists (and where)

		// First, look if we have an absolute map path
		if (os::fileOrDirExists(candidate))
		{
			mapToLoad = candidate;
			break;
		}

		fs::path mapsPath = GlobalGameManager().getMapPath();

		fs::path fullMapPath = mapsPath / candidate;

		// Next, look in the regular maps path
		if (os::fileOrDirExists(fullMapPath.string()))
		{
			mapToLoad = fullMapPath.string();
			break;
		}
	}

	if (!mapToLoad.empty())
	{
		loadMapSafe(mapToLoad);
	}
	else if (registry::getValue<bool>(RKEY_LOAD_LAST_MAP))
	{
		std::string lastMap = GlobalMRU().getLastMapName();

		if (!lastMap.empty() && os::fileOrDirExists(lastMap))
		{
			loadMapSafe(lastMap);
		}
	}
}

void StartupMapLoader::loadMapSafe(const std::string& mapToLoad)
{
	// Check if we have a valid openGL context, otherwise postpone the load
	if (GlobalOpenGL().wxContextValid())
	{
		GlobalCommandSystem().executeCommand("OpenMap", mapToLoad);
		return;
	}

	// No valid context, subscribe to the extensionsInitialised signal
	GlobalRenderSystem().signal_extensionsInitialised().connect([mapToLoad]()
	{
		GlobalCommandSystem().executeCommand("OpenMap", mapToLoad);
	});
}

void StartupMapLoader::onRadiantShutdown()
{
	GlobalMRU().saveRecentFiles();
}

const std::string& StartupMapLoader::getName() const
{
	static std::string _name("StartupMapLoader");
	return _name;
}

const StringSet& StartupMapLoader::getDependencies() const
{
	static StringSet _dependencies;

	if (_dependencies.empty())
	{
		_dependencies.insert(MODULE_RADIANT_APP);
	}

	return _dependencies;
}

void StartupMapLoader::initialiseModule(const ApplicationContext& ctx)
{
	rMessage() << getName() << "::initialiseModule called." << std::endl;

	GlobalRadiant().signal_radiantStarted().connect(
		sigc::mem_fun(*this, &StartupMapLoader::onRadiantStartup)
	);
	GlobalRadiant().signal_radiantShutdown().connect(
		sigc::mem_fun(*this, &StartupMapLoader::onRadiantShutdown)
	);
}

module::StaticModule<StartupMapLoader> startupMapLoader;

} // namespace map
