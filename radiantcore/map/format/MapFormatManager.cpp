#include "MapFormatManager.h"

#include "itextstream.h"
#include "igame.h"
#include "module/StaticModule.h"

#include "debugging/debugging.h"
#include "string/case_conv.h"
#include "os/path.h"

namespace map
{

void MapFormatManager::registerMapFormat(const std::string& extension, const MapFormatPtr& mapFormat)
{
	_mapFormats.insert(std::make_pair(string::to_lower_copy(extension), mapFormat));
}

void MapFormatManager::unregisterMapFormat(const MapFormatPtr& mapFormat)
{
	for (auto i = _mapFormats.begin(); i != _mapFormats.end(); )
	{
		if (i->second == mapFormat)
		{
			_mapFormats.erase(i++);
		}
		else
		{
			++i;
		}
	}
}

MapFormatPtr MapFormatManager::getMapFormatByName(const std::string& mapFormatName)
{
	for (const auto& pair : _mapFormats)
	{
		if (pair.second->getMapFormatName() == mapFormatName)
		{
			return pair.second;
		}
	}

	return MapFormatPtr(); // nothing found
}

MapFormatPtr MapFormatManager::getMapFormatForGameType(const std::string& gameType,
													   const std::string& extension)
{
	std::string extLower = string::to_lower_copy(extension);

	for (const auto& pair : _mapFormats)
	{
		if (pair.first == extLower && pair.second->getGameType() == gameType)
		{
			return pair.second;
		}
	}

	return MapFormatPtr(); // nothing found
}

MapFormatPtr MapFormatManager::getMapFormatForFilename(const std::string& filename)
{
	if (!GlobalGameManager().currentGame())
	{
		return MapFormatPtr();
	}

	// Look up the module name which loads the given extension
	std::string gameType = GlobalGameManager().currentGame()->getKeyValue("type");

	return getMapFormatForGameType(gameType, os::getExtension(filename));
}

std::set<MapFormatPtr> MapFormatManager::getAllMapFormats()
{
	std::set<MapFormatPtr> set;

	for (const auto& fmt : _mapFormats)
	{
		set.insert(fmt.second);
	}

	return set;
}

std::set<MapFormatPtr> MapFormatManager::getMapFormatList(const std::string& extension)
{
	std::set<MapFormatPtr> list;
	std::string extLower = string::to_lower_copy(extension);

	for (auto it = _mapFormats.find(extLower);
		 it != _mapFormats.upper_bound(extLower) && it != _mapFormats.end();
		 ++it)
	{
		list.insert(it->second);
	}

	return list;
}

// RegisterableModule implementation
const std::string& MapFormatManager::getName() const
{
	static std::string _name(MODULE_MAPFORMATMANAGER);
	return _name;
}

const StringSet& MapFormatManager::getDependencies() const
{
	static StringSet _dependencies; // no deps
	return _dependencies;
}

void MapFormatManager::initialiseModule(const IApplicationContext& ctx)
{
}

// Creates the static module instance
module::StaticModuleRegistration<MapFormatManager> staticMapFormatManagerModule;

}
