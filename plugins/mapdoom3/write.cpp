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

#include "write.h"

#include "ientity.h"
#include "iscriplib.h"
#include "scenelib.h"

inline MapExporter* Node_getMapExporter(scene::Node& node)
{
  return NodeTypeCast<MapExporter>::cast(node);
}

/* Export all of the keyvalues from the given entity, and write them
 * into the given stream.
 */

void Entity_ExportTokens(const Entity& entity, std::ostream& os)
{
	// Create a local Entity visitor class to export the keyvalues
	// to the output stream	
	class WriteKeyValue : public Entity::Visitor
	{
		// Stream to write to
		std::ostream& _os;
	public:
	
		// Constructor
		WriteKeyValue(std::ostream& os)
	    : _os(os)
	    {}

		// Required visit function
    	void visit(const char* key, const char* value) {
			_os << "\"" << key << "\" \"" << value << "\"\n";
		}

	} visitor(os);

	// Visit the entity
	entity.forEachKeyValue(visitor);
}

/* Walker class to traverse the scene graph and write each entity
 * out to the token stream, including its member brushes.
 */

class WriteTokensWalker : public scene::Traversable::Walker
{
	mutable Stack<bool> m_stack;

	// Output stream to write to
	std::ostream& _outStream;
  
	// Number of entities written (map global)
	mutable int _entityCount;
	
	// Number of brushes written for the current entity (entity local)
	mutable int _brushCount;
	
public:
  
	// Constructor
	WriteTokensWalker(std::ostream& os)
    : _outStream(os), 
      _entityCount(0),
      _brushCount(0)
	{}
	
	// Pre-descent callback
  bool pre(scene::Node& node) const
  {
    m_stack.push(false);

	// Check whether we are have a brush or an entity. We might get 
	// called at either level.
    Entity* entity = Node_getEntity(node);

	if(entity != 0) { // ENTITY

    	// Write out the entity number comment
		_outStream << "// entity " << _entityCount++ << "\n";

		// Entity opening brace
		_outStream << "{\n";

		// Entity key values
		Entity_ExportTokens(*entity, _outStream);

		// Reset the brush count and push to the stack to record
		// descent to brush level
		_brushCount = 0;
		m_stack.top() = true;

    }
    else  { // BRUSH

    	// Get the brush token exporter
		MapExporter* exporter = Node_getMapExporter(node);
		if(exporter != 0) {

			// Brush count comment
	        _outStream << "// primitive " << _brushCount++ << "\n";

			// Pass the ostream to the primitive's contained tokenexporter
			exporter->exportTokens(_outStream);
      }
    }

    return true;
  }
  
	// Post-descent callback
	void post(scene::Node& node) const {

		// If we are popping an entity, write the closing brace
		if(m_stack.top()) {
			_outStream << "}" << std::endl;
		}

		// If the brush count is 0 and we are adding dummy brushes to please
		// the Doom 3 editor, do so here
		if (_brushCount == 0) {
			std::cout << "No brushes" << std::endl;
			_outStream << "// Need dummy brush here" << std::endl;
		}

		// Pop the stack
		m_stack.pop();
	}
};

void Map_Write(scene::Node& root, GraphTraversalFunc traverse, std::ostream& os)
{
	traverse(root, WriteTokensWalker(os));
}

