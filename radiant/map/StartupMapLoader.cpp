#include "StartupMapLoader.h"

#include "imodule.h"
#include "iregistry.h"
#include "Map.h"
#include "ui/mru/MRU.h"

#include "os/path.h"
#include "os/file.h"
#include <boost/filesystem.hpp>

namespace map {

void StartupMapLoader::onRadiantStartup() 
{
	std::string mapToLoad = "";

    const ApplicationContext::ArgumentList& args(
        module::getRegistry().getApplicationContext().getCmdLineArgs()
    );

    for (ApplicationContext::ArgumentList::const_iterator i = args.begin();
         i != args.end(); 
         ++i) 
    {
		// Investigate the i-th argument
		std::string candidate = *i;

		if (os::getExtension(candidate) != "map") continue;

		// We have a map file, check if it exists (and where)
		boost::filesystem::path mapsPath = GlobalRegistry().get(RKEY_MAP_PATH);
		
		boost::filesystem::path fullMapPath = mapsPath / candidate;

		// First, look in the regular maps path
		if (boost::filesystem::exists(fullMapPath)) {
			mapToLoad = fullMapPath.file_string();
			break;
		}
		
		// Second, check for mod-relative paths
		fullMapPath = mapsPath.remove_leaf().remove_leaf() / candidate;

		if (boost::filesystem::exists(fullMapPath)) {
			mapToLoad = fullMapPath.file_string();
			break;
		}
	}

	if (!mapToLoad.empty()) {
		GlobalMap().load(mapToLoad);
	}
	else {
		std::string lastMap = GlobalMRU().getLastMapName();
		if (GlobalMRU().loadLastMap() && !lastMap.empty() && file_exists(lastMap.c_str())) {
			GlobalMap().load(lastMap);
		}
		else {
			GlobalMap().createNew();
		}
	}
}

void StartupMapLoader::onRadiantShutdown() {
	GlobalMRU().saveRecentFiles();
}

} // namespace map
