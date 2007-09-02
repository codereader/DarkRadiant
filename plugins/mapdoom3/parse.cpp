/*
Copyright (C) 2001-2006, William Joseph.
All Rights Reserved.

This file is part of GtkRadiant.

GtkRadiant is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

GtkRadiant is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GtkRadiant; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "parse.h"

#include <list>

#include "ientity.h"
#include "iregistry.h"
#include "brush/TexDef.h"
#include "ibrush.h"
#include "ipatch.h"
#include "ieclass.h"
#include "iradiant.h"
#include "ishaders.h"

#include "scenelib.h"
#include "traverselib.h"
#include "string/string.h"
#include "parser/DefTokeniser.h"

#include "gtkutil/ModalProgressDialog.h"
#include "gtkutil/dialog.h"

#include <map>
#include <string>

namespace {
	const std::string RKEY_MAP_LOAD_STATUS_INTERLEAVE = "user/ui/map/loadStatusInterleave";
}

/**
 * String map type.
 */
typedef std::map<std::string, std::string> StringMap;

inline MapImporterPtr Node_getMapImporter(scene::INodePtr node) {
	return boost::dynamic_pointer_cast<MapImporter>(node);
}

/**
 * Create an entity with the given properties.
 */
scene::INodePtr Entity_create(EntityCreator& entityTable, 
                              const StringMap& keyValues)
{
    // Get the classname from the StringMap
    StringMap::const_iterator iter = keyValues.find("classname");
    if (iter == keyValues.end()) {
        throw std::runtime_error("Entity_create(): could not find classname.");
    }
    
    // Otherwise create the entity and add all of the properties
    std::string className = iter->second;
    IEntityClassPtr classPtr = GlobalEntityClassManager().findClass(className);
    
    scene::INodePtr entity(entityTable.createEntity(classPtr));

    for (StringMap::const_iterator i = keyValues.begin(); 
         i != keyValues.end(); 
         ++i)
    {
        Node_getEntity(entity)->setKeyValue(i->first, i->second);
    }
    return entity;
}

scene::INodePtr Entity_parseTokens(
	parser::DefTokeniser& tokeniser, 
	EntityCreator& entityTable, 
	const PrimitiveParser& parser, 
	int index,
	int interleave,
	gtkutil::ModalProgressDialog& dialog)
{
    // Set up the progress dialog
	std::string dlgEntityText = "Loading entity " + intToStr(index);
	bool updateDialog = false;
	if (index % interleave == 0) { 
		updateDialog = true;
    }

    // Map of keyvalues for this entity
    StringMap keyValues;

    // The actual entity. This is initially null, and will be created when
    // primitives start or the end of the entity is reached
    scene::INodePtr entity;
    
    /* START PARSING */
    
	// First token must be an open brace
	tokeniser.assertNextToken("{");
	
	std::string token = tokeniser.nextToken();
	int numPrimitives = 0;
	
	while (true) {
	    
	    // Token must be either a key, a "{" to indicate the start of a 
	    // primitive, or a "}" to indicate the end of the entity

	    if (token == "{") { // PRIMITIVE

	        // Create the entity if necessary
	        if (!entity)
	            entity = Entity_create(entityTable, keyValues);
	        
	        // Update the dialog
	        if (updateDialog && (numPrimitives % interleave == 0)) {
    	        dialog.setText(
                    dlgEntityText + "\nPrimitive " + intToStr(numPrimitives)
                );
	        }
	        ++numPrimitives;
	        
	        // Try to parse the primitive, throwing exception if failed
	        scene::INodePtr primitive(parser.parsePrimitive(tokeniser));
	        if (!primitive 
	            || !Node_getMapImporter(primitive)->importTokens(tokeniser))
	        {
	            throw std::runtime_error("Primitive #" + intToStr(numPrimitives) 
	                                     + ": parse error\n");
	        }
	        
	        // Now add the primitive as a child of the entity
	        scene::TraversablePtr traversable = Node_getTraversable(entity);
	        if(Node_getEntity(entity)->isContainer() 
	           && traversable != 0) 
	        {
	            // Try to insert the primitive into the entity. This may throw 
	            // an exception if the entity should not contain brushes 
	            // (e.g. a func_static with a model key)
	            try {
	                traversable->insert(primitive);
	            }
	            catch (std::runtime_error e) {
	                // Warn, but just ignore the brush
	                globalErrorStream() 
	                    << "[mapdoom3] Entity " << index 
	                    << " failed to accept brush, discarding\n";
	            }
	        }
	        
	    }
	    else if (token == "}") { // END OF ENTITY
            // Create the entity if necessary and return it
	        if (!entity)
	            entity = Entity_create(entityTable, keyValues);
	        return entity;
	    }
	    else { // KEY
	        std::string value = tokeniser.nextToken();

	        // Sanity check (invalid number of tokens will get us out of sync)
	        if (value == "{" || value == "}") {
	            throw std::runtime_error(
	                "Parsed invalid value '" + value + "' for key '" 
	                + token + "'"
	            );
	        }
	        
	        // Otherwise add the keyvalue pair to our map
	        keyValues.insert(StringMap::value_type(token, value));
	    }
	    
	    // Get the next token
	    token = tokeniser.nextToken();
	    
	} // end of while
}

// Check if the given node is excluded based on entity class (debug code). 
// Return true if not excluded, false otherwise
bool checkEntityClass(scene::INodePtr node) {
	// Obtain list of entityclasses to skip
	static xml::NodeList skipLst = 
		GlobalRegistry().findXPath("debug/mapdoom3//discardEntityClass");

	// Obtain the entity class of this node
	IEntityClassConstPtr entityClass = 
			Node_getEntity(node)->getEntityClass();

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

// Check if the entity with the given number should be inserted (debug)
bool checkEntityNum(int num) {
	
	using boost::lexical_cast;

	// Entity range XPath
	static xml::NodeList entityRange = 
					GlobalRegistry().findXPath("debug/mapdoom3/entityRange");
	static xml::NodeList::iterator i = entityRange.begin();
	
	// Test the entity number is in the range
	if (i != entityRange.end()) {
		static int lower = lexical_cast<int>(i->getAttributeValue("start"));
		static int upper = lexical_cast<int>(i->getAttributeValue("end"));
	
		if (num < lower || num > upper) {
			std::cout << "DEBUG: Discarding entity " << num << ", out of range"
					  << std::endl;
			return false;
		}
	}
	return true;
}

// Insert an entity node into the scenegraph, checking if any of the debug flags
// exclude this node
void checkInsert(scene::INodePtr node, scene::INodePtr root, int count) {

	// Static "debug" flag obtained from the registry
	static bool _debug = GlobalRegistry().get("user/debug") == "1";
	
	// Abort if any of the tests fail
	if (_debug && (!checkEntityClass(node) || !checkEntityNum(count)))
		return;
	
	// Insert the node into the scenegraph root
	Node_getTraversable(root)->insert(node);
}
		
void Map_Read(scene::INodePtr root, 
			  parser::DefTokeniser& tokeniser, 
			  EntityCreator& entityTable, 
			  const PrimitiveParser& parser)
{
	int interleave = GlobalRegistry().getInt(RKEY_MAP_LOAD_STATUS_INTERLEAVE);
	if (interleave <= 0) {
		interleave = 10;
	} 
	
	// Create an info display panel to track load progress
	gtkutil::ModalProgressDialog dialog(GlobalRadiant().getMainWindow(),
										"Loading map");

	// Disable texture window updates for performance
	GlobalShaderSystem().setActiveShaderUpdates(false);
	
	// Read each entity in the map, until EOF is reached
	for (int entCount = 0; ; entCount++) {

		// Update the dialog text. This will throw an exception if the cancel
		// button is clicked, which we must catch and handle.
		if (entCount % interleave == 0) {
			try {
				dialog.setText("Loading entity " + intToStr(entCount));
			}
			catch (gtkutil::ModalProgressDialog::OperationAbortedException e) {
				gtkutil::errorDialog("Map loading cancelled", 
									 GlobalRadiant().getMainWindow());
				return;			
			}
		}

		// Check for end of file
		if (!tokeniser.hasMoreTokens())
			break;

		// Create an entity node by parsing from the stream. If there is an
		// exception, display it and return
		try {
			// Get a node reference to the new entity
			scene::INodePtr entity(Entity_parseTokens(tokeniser, 
			                                          entityTable, 
			                                          parser, 
			                                          entCount,
			                                          interleave,
			                                          dialog));
			// Insert the entity
			checkInsert(entity, root, entCount);
		}
		catch (std::runtime_error e) {
			gtkutil::errorDialog(
				"Failed on entity " + intToStr(entCount) + "\n\n" + e.what(), 
				GlobalRadiant().getMainWindow()
			);
			return;			
		}
		
	}
	
	// Re-enable texture window updates
	GlobalShaderSystem().setActiveShaderUpdates(true);
	
	
}
