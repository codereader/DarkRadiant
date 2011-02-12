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
	IMapImportFilter& _importFilter;

	// The map type for one entity's keyvalues (spawnargs)
	typedef std::map<std::string, std::string> EntityKeyValues;

	// The size of the input file
	//long _fileSize;

	// The number of entities found in this map file so far
	std::size_t _entityCount;

	// The number of primitives of the currently parsed entity
	std::size_t _primitiveCount;

/*	// The progress dialog
	gtkutil::ModalProgressDialogPtr _dialog;

	// The progress dialog text for the current entity
	std::string _dlgEntityText;

    // Event rate limiter for the progress dialog
    EventRateLimiter _dialogEventLimiter;*/

	// TRUE if we're in debugging parse mode
	bool _debug;

public:
	Doom3MapReader(IMapImportFilter& importFilter);

	// IMapReader implementation
	void readFromStream(std::istream& stream);

private:
	// Parse the version tag at the beginning, throws on failure
	void parseMapVersion(parser::DefTokeniser& tok);

	// Parses an entity plus all child primitives, throws on failure
	void parseEntity(parser::DefTokeniser& tok);

	// Parse the primitive block and insert the child into the given parent
	void parsePrimitive(parser::DefTokeniser& tok, const scene::INodePtr& parentEntity);

	// Create an entity with the given properties and layers
	scene::INodePtr createEntity(const EntityKeyValues& keyValues);

#if 0
	// Inserts the entity into the root (and performs a couple of checks beforehand)
	void insertEntity(const scene::INodePtr& entity);

	// Check if the given node is excluded based on entity class (debug code).
	// Return true if not excluded, false otherwise
	bool checkEntityClass(const scene::INodePtr& entity);

	// Check if the entity with the given number should be inserted (debug)
	bool checkEntityNum();

private:
	// Gets the ratio of read bytes vs. total bytes in the input stream
	double getProgressFraction();
#endif
};

} // namespace map

#endif /* NODE_IMPORTER_H_ */
