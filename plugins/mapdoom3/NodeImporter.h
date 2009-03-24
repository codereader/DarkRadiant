#ifndef NODE_IMPORTER_H_
#define NODE_IMPORTER_H_

#include <map>
#include "inode.h"
#include "imap.h"
#include "parser/DefTokeniser.h"
#include "gtkutil/ModalProgressDialog.h"
#include "EventRateLimiter.h"

#include "PrimitiveParser.h"

namespace map {

class InfoFile;

class NodeImporter {

	// The map type for one entity's keyvalues (spawnargs)
	typedef std::map<std::string, std::string> EntityKeyValues;

	// The container which will hold the imported nodes
	scene::INodePtr _root;
	
	std::istream _inputStream;

	// The tokeniser used to split the stream into pieces
	parser::BasicDefTokeniser<std::istream> _tok;

	// The helper class containing the meta-information for this map file
	InfoFile& _infoFile;

	// The number of entities found in this map file so far
	std::size_t _entityCount;

	// The number of primitives of the currently parsed entity
	std::size_t _primitiveCount;

	// The number of layer sets written to the file
	std::size_t _layerInfoCount;

	// The progress dialog
	gtkutil::ModalProgressDialogPtr _dialog;

	// The progress dialog text for the current entity
	std::string _dlgEntityText;

   // Event rate limiter for the progress dialog
   EventRateLimiter _dialogEventLimiter;

	// The helper module, which will parse the primitive tokens
	const PrimitiveParser& _parser;

	// TRUE if we're in debugging parse mode
	bool _debug;

public:
	NodeImporter(const MapImportInfo& importInfo, 
		         InfoFile& infoFile, 
				 const PrimitiveParser& parser);

	// Start parsing, this should not "leak" any exceptions
	// Returns TRUE if the parsing succeeded without errors or exceptions.
	bool parse();

private:
	// Parse the version tag at the beginning, returns TRUE on success
	bool parseMapVersion();

	// Parses an entity plus all child primitives
	void parseEntity();

	// Parse the primitive block and insert the child into the given parent
	void parsePrimitive(const scene::INodePtr& parentEntity);

	// Create an entity with the given properties and layers
	scene::INodePtr createEntity(const EntityKeyValues& keyValues);

	// Inserts the entity into the root (and performs a couple of checks beforehand)
	void insertEntity(const scene::INodePtr& entity);

	// Check if the given node is excluded based on entity class (debug code). 
	// Return true if not excluded, false otherwise
	bool checkEntityClass(const scene::INodePtr& entity);

	// Check if the entity with the given number should be inserted (debug)
	bool checkEntityNum();
};

} // namespace map

#endif /* NODE_IMPORTER_H_ */
