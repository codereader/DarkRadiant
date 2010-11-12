#ifndef _MapFormatManager_h__
#define _MapFormatManager_h__

#include "imapformat.h"
#include <map>

namespace map
{

class MapFormatManager :
	public IMapFormatManager
{
private:
	// A map associating keywords with primitive parser instances
	typedef std::map<std::string, PrimitiveParserPtr> ParserMap;
	ParserMap _parsers;

public:
	void registerPrimitiveParser(const PrimitiveParserPtr& parser);

	// Returns a parser for the given keyword or NULL if none associated
	PrimitiveParserPtr getPrimitiveParser(const std::string& keyword);

	// RegisterableModule implementation
	const std::string& getName() const;
	const StringSet& getDependencies() const;
	void initialiseModule(const ApplicationContext& ctx);
};

}

#endif // _MapFormatManager_h__
