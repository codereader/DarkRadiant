#ifndef NODE_IMPORTER_H_
#define NODE_IMPORTER_H_

#include <map>
#include "inode.h"
#include "imapformat.h"
#include "parser/DefTokeniser.h"

namespace map {

class Doom3MapReader :
	public IMapReader
{
protected:
	IMapImportFilter& _importFilter;

	// The map type for one entity's keyvalues (spawnargs)
	typedef std::map<std::string, std::string> EntityKeyValues;

	// The number of entities found in this map file so far
	std::size_t _entityCount;

	// The number of primitives of the currently parsed entity
	std::size_t _primitiveCount;

	// Our list of primitive parsers
	typedef std::map<std::string, PrimitiveParserPtr> PrimitiveParsers;
	PrimitiveParsers _primitiveParsers;

public:
	Doom3MapReader(IMapImportFilter& importFilter);

	// IMapReader implementation
	virtual void readFromStream(std::istream& stream);

protected:
	// Set up our set of primitive parsers
	virtual void initPrimitiveParsers();

	// Adds a specific primitive parser
	virtual void addPrimitiveParser(const PrimitiveParserPtr& parser);

	// Parse the version tag at the beginning, throws on failure
	virtual void parseMapVersion(parser::DefTokeniser& tok);

	// Parses an entity plus all child primitives, throws on failure
	virtual void parseEntity(parser::DefTokeniser& tok);

	// Parse the primitive block and insert the child into the given parent
	virtual void parsePrimitive(parser::DefTokeniser& tok, const scene::INodePtr& parentEntity);

	// Create an entity with the given properties and layers
	scene::INodePtr createEntity(const EntityKeyValues& keyValues);
};

} // namespace map

#endif /* NODE_IMPORTER_H_ */
