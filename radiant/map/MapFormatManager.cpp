#include "MapFormatManager.h"

#include "itextstream.h"
#include "modulesystem/StaticModule.h"

#include <boost/algorithm/string/case_conv.hpp>

namespace map
{

void MapFormatManager::registerMapFormat(const std::string& extension, const MapFormatPtr& mapFormat)
{
	_mapFormats.insert(MapFormatModules::value_type(boost::algorithm::to_lower_copy(extension), mapFormat));
}

void MapFormatManager::unregisterMapFormat(const MapFormatPtr& mapFormat)
{
	for (MapFormatModules::iterator i = _mapFormats.begin(); i != _mapFormats.end(); )
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
	for (MapFormatModules::const_iterator i = _mapFormats.begin(); i != _mapFormats.end(); ++i)
	{
		if (i->second->getMapFormatName() == mapFormatName)
		{
			return i->second;
		}
	}

	return MapFormatPtr(); // nothing found
}

MapFormatPtr MapFormatManager::getMapFormatForGameType(const std::string& gameType, 
													   const std::string& extension)
{
	std::string extLower = boost::algorithm::to_lower_copy(extension);

	for (MapFormatModules::const_iterator i = _mapFormats.begin(); i != _mapFormats.end(); ++i)
	{
		if (i->first == extLower && i->second->getGameType() == gameType)
		{
			return i->second;
		}
	}

	return MapFormatPtr(); // nothing found
}

std::set<MapFormatPtr> MapFormatManager::getMapFormatList(const std::string& extension)
{
	std::set<MapFormatPtr> list;
	std::string extLower = boost::algorithm::to_lower_copy(extension);

	for (MapFormatModules::iterator it = _mapFormats.find(extLower);
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

void MapFormatManager::initialiseModule(const ApplicationContext& ctx)
{
	rMessage() << getName() << "::initialiseModule called." << std::endl;
}

// Creates the static module instance
module::StaticModule<MapFormatManager> staticMapFormatManagerModule;

}
