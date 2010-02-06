#include "MapFormatManager.h"

#include "itextstream.h"
#include "modulesystem/StaticModule.h"

namespace map
{

void MapFormatManager::registerPrimitiveParser(const PrimitiveParserPtr& parser)
{
	std::pair<ParserMap::iterator, bool> result = _parsers.insert(
		ParserMap::value_type(parser->getKeyword(), parser)
	);
	
	if (!result.second)
	{
		globalErrorStream() << "Could not register primitive parser for keyword " 
			<< parser->getKeyword() << ". The keyword is already associated." << std::endl;
	}
}

PrimitiveParserPtr MapFormatManager::getPrimitiveParser(const std::string& keyword)
{
	ParserMap::const_iterator found = _parsers.find(keyword);

	return (found != _parsers.end()) ? found->second : PrimitiveParserPtr();
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
	globalOutputStream() << getName() << "::initialiseModule called." << std::endl;
}

// Creates the static module instance
module::StaticModule<MapFormatManager> staticMapFormatManagerModule;

}
