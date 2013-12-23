#pragma once

#include "imapformat.h"
#include <map>

namespace map
{

class MapFormatManager :
	public IMapFormatManager
{
private:
	// A mapping between extensions and format modules
	// Multiple modules can register themselves for a single extension
	typedef std::multimap<std::string, MapFormatPtr> MapFormatModules;
	MapFormatModules _mapFormats;

public:
	void registerMapFormat(const std::string& extension, const MapFormatPtr& mapFormat);
	void unregisterMapFormat(const MapFormatPtr& mapFormat);

	MapFormatPtr getMapFormatByName(const std::string& mapFormatName);
	MapFormatPtr getMapFormatForGameType(const std::string& gameType, const std::string& extension);

	std::set<MapFormatPtr> getMapFormatList(const std::string& extension);

	// RegisterableModule implementation
	const std::string& getName() const;
	const StringSet& getDependencies() const;
	void initialiseModule(const ApplicationContext& ctx);
};

}
