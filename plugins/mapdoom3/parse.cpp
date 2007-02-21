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
#include "iscriplib.h"
#include "qerplugin.h"
#include "scenelib.h"
#include "traverselib.h"
#include "stringio.h"

#include "gtkutil/ModalProgressDialog.h"
#include "gtkutil/dialog.h"

#include <boost/lexical_cast.hpp>

inline MapImporter* Node_getMapImporter(scene::Node& node)
{
  return NodeTypeCast<MapImporter>::cast(node);
}


typedef std::list< std::pair<std::string, std::string> > KeyValues;

NodeSmartReference g_nullNode(NewNullNode());


NodeSmartReference Entity_create(EntityCreator& entityTable, IEntityClassPtr entityClass, const KeyValues& keyValues)
{
  scene::Node& entity(entityTable.createEntity(entityClass));
  for(KeyValues::const_iterator i = keyValues.begin(); i != keyValues.end(); ++i)
  {
    Node_getEntity(entity)->setKeyValue((*i).first.c_str(), (*i).second.c_str());
  }
  return NodeSmartReference(entity);
}

NodeSmartReference Entity_parseTokens(Tokeniser& tokeniser, EntityCreator& entityTable, const PrimitiveParser& parser, int index)
{
  NodeSmartReference entity(g_nullNode);
  KeyValues keyValues;
	std::string classname = "";

  int count_primitives = 0;
	while(1) {
		
		// Get the token and check for NULL
		tokeniser.nextLine();
		const char* szToken = tokeniser.getToken();
		if (szToken == NULL) {
			throw std::runtime_error("Invalid token.");
		}
		
		std::string token = szToken;
    
    if (token == "}") // end entity
    {
      if(entity == g_nullNode)
      {
        // entity does not have brushes
        entity = Entity_create(entityTable, GlobalEntityClassManager().findOrInsert(classname, false), keyValues);
      }
      return entity;
    }
    else if(token == "{") // begin primitive
    {
      if(entity == g_nullNode)
      {
        // entity has brushes
        entity = Entity_create(entityTable, GlobalEntityClassManager().findOrInsert(classname, true), keyValues);
      }

      tokeniser.nextLine();

		// Try to import the primitive, throwing exception if failed
		NodeSmartReference primitive(parser.parsePrimitive(tokeniser));
		if (primitive == g_nullNode 
			|| !Node_getMapImporter(primitive)->importTokens(tokeniser))
		{
        	throw std::runtime_error("Brush " 
				+ boost::lexical_cast<std::string>(count_primitives) 
				+ ": parse error\n");
		}

		scene::Traversable* traversable = Node_getTraversable(entity);
		if(Node_getEntity(entity)->isContainer() 
		   && traversable != 0) 
		{
			// Try to insert the primitive into the entity. This may throw an exception if
			// the entity should not contain brushes (e.g. a func_static with a model key)
	        try {
	        	traversable->insert(primitive);
	        }
	        catch (std::runtime_error e) {
	        	// Warn, but just ignore the brush
	        	globalErrorStream() << "[mapdoom3] Entity " << index << " failed to accept brush, discarding\n";
	        }
		}
		else {
			globalErrorStream() << "entity " << index << ": type " << classname << ": discarding brush " << count_primitives << "\n";
		}
		
		++count_primitives;
    }
	else { 
		
		// Keyvalue pair. Get the value, and check that we haven't swallowed
		// the { or } which will happen if the number of tokens is wrong.
		
		const char* szValue = tokeniser.getToken();
		if (szValue == NULL) {
			throw std::runtime_error("Parsing keyvalues: invalid token");
		}
		 
      	std::string value = szValue;
		if (value == "{" || value == "}") {
			throw std::runtime_error(
				"Parsed invalid value \"" + value + "\"");
		}
				 
		// Push the pair, and set the classname if we have it
		keyValues.push_back(KeyValues::value_type(token, value));
		if(token == "classname") {
			classname = value;
		}
    }
  }
}

// Check if the given node is excluded based on entity class (debug code). 
// Return true if not excluded, false otherwise
bool checkEntityClass(NodeSmartReference node) {
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
void checkInsert(NodeSmartReference node, scene::Node& root, int count) {

	// Static "debug" flag obtained from the registry
	static bool _debug = GlobalRegistry().get("user/debug") == "1";
	
	// Abort if any of the tests fail
	if (_debug && (!checkEntityClass(node) || !checkEntityNum(count)))
		return;
	
	// Insert the node into the scenegraph root
	Node_getTraversable(root)->insert(node);
}
		
	
	

void Map_Read(scene::Node& root, 
			  Tokeniser& tokeniser, 
			  EntityCreator& entityTable, 
			  const PrimitiveParser& parser)
{
	// Create an info display panel to track load progress
	gtkutil::ModalProgressDialog dialog(GlobalRadiant().getMainWindow(),
										"Loading map");

	// Read each entity in the map, until EOF is reached
	for (int entCount = 0; ; entCount++) {

		// Update the dialog text. This will throw an exception if the cancel
		// button is clicked, which we must catch and handle.
		try {
			dialog.setText("Loading entity " 
						   + boost::lexical_cast<std::string>(entCount));
		}
		catch (gtkutil::ModalProgressDialog::OperationAbortedException e) {
			gtkutil::errorDialog("Map loading cancelled", 
								 GlobalRadiant().getMainWindow());
			return;			
		}

		// Check for end of file
		tokeniser.nextLine();
		if (!tokeniser.getToken()) // { or 0
			break;

		// Create an entity node by parsing from the stream. If there is an
		// exception, display it and return
		try {
			// Get a node reference to the new entity
			NodeSmartReference entity(Entity_parseTokens(tokeniser, 
														 entityTable, 
														 parser, 
														 entCount));
			// Insert the entity
			checkInsert(entity, root, entCount);
		}
		catch (std::runtime_error e) {
			gtkutil::errorDialog(e.what(), GlobalRadiant().getMainWindow());
			return;			
		}
		
	}
}
