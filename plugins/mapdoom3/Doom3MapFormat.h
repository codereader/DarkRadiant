#ifndef DOOM3MAPFORMAT_H_
#define DOOM3MAPFORMAT_H_

#include "imap.h"
#include "PrimitiveParser.h"

namespace map {

	namespace {
		// The supported Doom3 map version 
		const int MAPVERSION = 2;

		// The version of the map info file
		const int MAP_INFO_VERSION = 2;
	}

class Doom3MapFormat : 
	public MapFormat,
	public PrimitiveParser
{
public:
	// RegisterableModule implementation
	virtual const std::string& getName() const;
	virtual const StringSet& getDependencies() const;
	virtual void initialiseModule(const ApplicationContext& ctx);
	
	/**
	 * Parse a primitive from the given token stream.
	 */
	scene::INodePtr parsePrimitive(parser::DefTokeniser& tokeniser) const;
  
    /**
     * Read tokens from a map stream and create entities accordingly.
     */
    void readGraph(const MapImportInfo& importInfo) const;

	// Write scene graph to an ostream
	void writeGraph(const MapExportInfo& exportInfo) const;
};
typedef boost::shared_ptr<Doom3MapFormat> Doom3MapFormatPtr;

} // namespace map

#endif /* DOOM3MAPFORMAT_H_ */
