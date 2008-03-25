#include "NodeImporter.h"

#include "imap.h"
#include "iradiant.h"
#include "iregistry.h"
#include "ieclass.h"
#include "stream/textstream.h"
#include "string/string.h"

#include "gtkutil/dialog.h"
#include "MapImportInfo.h"
#include "scenelib.h"

#include "Tokens.h"
#include "Doom3MapFormat.h"
#include "InfoFile.h"

#include <boost/lexical_cast.hpp>

namespace map {

	namespace {
		const std::string RKEY_MAP_LOAD_STATUS_INTERLEAVE = "user/ui/map/loadStatusInterleave";

		inline MapImporterPtr Node_getMapImporter(scene::INodePtr node) {
			return boost::dynamic_pointer_cast<MapImporter>(node);
		}
	}

NodeImporter::NodeImporter(const MapImportInfo& importInfo, 
						   InfoFile& infoFile, 
						   const PrimitiveParser& parser) :
	_root(importInfo.root),
	_inputStream(&importInfo.inputStream),
	_tok(_inputStream),
	_infoFile(infoFile),
	_loadStatusInterleave(static_cast<std::size_t>(GlobalRegistry().getInt(RKEY_MAP_LOAD_STATUS_INTERLEAVE))),
	_entityCount(0),
	_primitiveCount(0),
	_layerInfoCount(0),
	_dialog(GlobalRadiant().getMainWindow(), "Loading map"),
	_parser(parser),
	_debug(GlobalRegistry().get("user/debug") == "1")
{}

void NodeImporter::parse() {
	// Try to parse the map version
	if (!parseMapVersion()) {
		// Failed => quit
		return;
	}

	// Read each entity in the map, until EOF is reached
	while (_tok.hasMoreTokens()) {
		// Update the dialog text. This will throw an exception if the cancel
		// button is clicked, which we must catch and handle.
		if (_entityCount % _loadStatusInterleave == 0) {
			try {
				_dialog.setText("Loading entity " + sizetToStr(_entityCount));
			}
			catch (gtkutil::ModalProgressDialog::OperationAbortedException e) {
				gtkutil::errorDialog("Map loading cancelled", 
									 GlobalRadiant().getMainWindow());
				return;			
			}
		}

		// Create an entity node by parsing from the stream. If there is an
		// exception, display it and return
		try {
			parseEntity();
		}
		catch (std::runtime_error e) {
			gtkutil::errorDialog(
				"Failed on entity " + sizetToStr(_entityCount) + "\n\n" + e.what(), 
				GlobalRadiant().getMainWindow()
			);
			return;			
		}

		_entityCount++;
	}
}

bool NodeImporter::parseMapVersion() {
	// Parse the map version
    float version = 0;

    try {
        _tok.assertNextToken(VERSION);
        version = boost::lexical_cast<float>(_tok.nextToken());
    }
    catch (parser::ParseException e) {
        globalErrorStream() 
            << "[mapdoom3] Unable to parse map version: " 
            << e.what() << "\n";
        return false;
    }
    catch (boost::bad_lexical_cast e) {
        globalErrorStream() 
            << "[mapdoom3] Unable to parse map version: " 
            << e.what() << "\n";
        return false;
    }

    // Check we have the correct version for this module
    if (version != MAPVERSION) {
        globalErrorStream() 
            << "Incorrect map version: required " << MAPVERSION 
            << ", found " << version << "\n";
        return false;
    }

	return true;
}

void NodeImporter::parsePrimitive(const scene::INodePtr& parentEntity) {
	// Check if the entitycount is matching the interleave
	bool updateDialog = (_entityCount % _loadStatusInterleave == 0);

    // Update the dialog
    if (updateDialog && (_primitiveCount % _loadStatusInterleave == 0)) {
        _dialog.setText(
            _dlgEntityText + "\nPrimitive " + sizetToStr(_primitiveCount)
        );
    }

    _primitiveCount++;
    
    // Try to parse the primitive, throwing exception if failed
    scene::INodePtr primitive(_parser.parsePrimitive(_tok));

    if (!primitive || !Node_getMapImporter(primitive)->importTokens(_tok)) {
        throw std::runtime_error("Primitive #" + sizetToStr(_primitiveCount) 
                                 + ": parse error\n");
    }
    
    // Now add the primitive as a child of the entity
    if (Node_getEntity(parentEntity)->isContainer()) {
        parentEntity->addChildNode(primitive);
    }
}

scene::INodePtr NodeImporter::createEntity(const EntityKeyValues& keyValues) {
    // Get the classname from the EntityKeyValues
    EntityKeyValues::const_iterator found = keyValues.find("classname");

    if (found == keyValues.end()) {
		throw std::runtime_error("NodeImporter::createEntity(): could not find classname.");
    }
    
    // Otherwise create the entity and add all of the properties
    std::string className = found->second;
	IEntityClassPtr classPtr = GlobalEntityClassManager().findClass(className);

	if (classPtr == NULL) {
		globalErrorStream() << "[mapdoom3]: Could not find entity class: " 
			                << className.c_str() << "\n";

		// greebo: EntityClass not found, insert a brush-based one
		classPtr = GlobalEntityClassManager().findOrInsert(className, true);
	}
    
	// Create the actual entity node
    scene::INodePtr entity(GlobalEntityCreator().createEntity(classPtr));

	Entity* ent = Node_getEntity(entity);
	assert(ent != NULL); // entity cast must not fail

    for (EntityKeyValues::const_iterator i = keyValues.begin(); 
         i != keyValues.end(); 
         ++i)
    {
        ent->setKeyValue(i->first, i->second);
    }

    return entity;
}

void NodeImporter::parseEntity() {
	// Set up the progress dialog text
	_dlgEntityText = "Loading entity " + sizetToStr(_entityCount);
	
    // Map of keyvalues for this entity
    EntityKeyValues keyValues;

    // The actual entity. This is initially null, and will be created when
    // primitives start or the end of the entity is reached
    scene::INodePtr entity;

	// Start parsing, first token must be an open brace
	_tok.assertNextToken("{");

	std::string token = _tok.nextToken();

	// Reset the primitive counter, we're starting a new entity
	_primitiveCount = 0;
	
	while (true) {
	    // Token must be either a key, a "{" to indicate the start of a 
	    // primitive, or a "}" to indicate the end of the entity

	    if (token == "{") { // PRIMITIVE
			// Create the entity right now, if not yet done
			if (entity == NULL) {
				entity = createEntity(keyValues);
			}

			// Parse the primitive block, and pass the parent entity
			parsePrimitive(entity);
	    }
	    else if (token == "}") { // END OF ENTITY
            // Create the entity if necessary and return it
	        if (entity == NULL) {
	            entity = createEntity(keyValues);
	        }
			break;
	    }
	    else { // KEY
	        std::string value = _tok.nextToken();

	        // Sanity check (invalid number of tokens will get us out of sync)
	        if (value == "{" || value == "}") {
	            throw std::runtime_error(
	                "Parsed invalid value '" + value + "' for key '" + token + "'"
	            );
	        }
	        
	        // Otherwise add the keyvalue pair to our map
	        keyValues.insert(EntityKeyValues::value_type(token, value));
	    }
	    
	    // Get the next token
	    token = _tok.nextToken();
	}

	// Insert the entity
	insertEntity(entity);
}

bool NodeImporter::checkEntityClass(const scene::INodePtr& entity) {
	// Obtain list of entityclasses to skip
	static xml::NodeList skipLst = 
		GlobalRegistry().findXPath("debug/mapdoom3//discardEntityClass");

	// Obtain the entity class of this node
	IEntityClassConstPtr entityClass = 
			Node_getEntity(entity)->getEntityClass();

	// Skip this entity class if it is in the list
	for (xml::NodeList::const_iterator i = skipLst.begin();
		 i != skipLst.end();
		 ++i)
	{
		if (i->getAttributeValue("value") == entityClass->getName()) {
			std::cout << "DEBUG: discarding entity class " 
					  << entityClass->getName() << std::endl;
			return false;
		}
	}

	return true;
}

bool NodeImporter::checkEntityNum() {
	// Entity range XPath
	static xml::NodeList entityRange = 
					GlobalRegistry().findXPath("debug/mapdoom3/entityRange");
	static xml::NodeList::iterator i = entityRange.begin();
	
	// Test the entity number is in the range
	if (i != entityRange.end()) {
		static std::size_t lower = strToSizet(i->getAttributeValue("start"));
		static std::size_t upper = strToSizet(i->getAttributeValue("end"));
	
		if (_entityCount < lower || _entityCount > upper) {
			std::cout << "DEBUG: Discarding entity " << _entityCount << ", out of range"
					  << std::endl;
			return false;
		}
	}
	return true;
}

void NodeImporter::insertEntity(const scene::INodePtr& entity) {
	// Abort if any of the tests fail
	if (_debug && (!checkEntityClass(entity) || !checkEntityNum())) {
		return;
	}
	
	// Insert the node into the given root
	_root->addChildNode(entity);
}

} // namespace map
