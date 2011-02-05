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

	// A map associating keywords with primitive parser instances
	typedef std::map<std::string, PrimitiveParserPtr> ParserMap;
	ParserMap _parsers;

public:
	void registerMapFormat(const std::string& extension, const MapFormatPtr& mapFormat);
	void unregisterMapFormat(const MapFormatPtr& mapFormat);

	std::set<MapFormatPtr> getMapFormatList(const std::string& extension);

	void registerPrimitiveParser(const PrimitiveParserPtr& parser);

	// Returns a parser for the given keyword or NULL if none associated
	PrimitiveParserPtr getPrimitiveParser(const std::string& keyword);

	// RegisterableModule implementation
	const std::string& getName() const;
	const StringSet& getDependencies() const;
	void initialiseModule(const ApplicationContext& ctx);
};

}
