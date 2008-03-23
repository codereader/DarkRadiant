#include "NodeExporter.h"

#include "iregistry.h"
#include "imap.h"
#include "ieclass.h"
#include "ientity.h"

namespace map {

	namespace {
		/* CONSTANTS */
		const char* DUMMY_BRUSH =
			"// dummy brush 0\n\
			{\n\
			brushDef3\n\
			{\n\
			( 0 0 -1 0 ) ( ( 0.125 0 0 ) ( 0 0.125 0 ) ) \"_default\" 0 0 0\n\
			( 0 0 1 -8 ) ( ( 0.125 0 0 ) ( 0 0.125 0 ) ) \"_default\" 0 0 0\n\
			( 0 -1 0 -16 ) ( ( 0.125 0 0 ) ( 0 0.125 0 ) ) \"_default\" 0 0 0\n\
			( 1 0 0 -16 ) ( ( 0.125 0 0 ) ( 0 0.125 0 ) ) \"_default\" 0 0 0\n\
			( 0 1 0 -16 ) ( ( 0.125 0 0 ) ( 0 0.125 0 ) ) \"_default\" 0 0 0\n\
			( -1 0 0 -16 ) ( ( 0.125 0 0 ) ( 0 0.125 0 ) ) \"_default\" 0 0 0\n\
			}\n\
			}\n";

		inline MapExporterPtr Node_getMapExporter(scene::INodePtr node) {
			return boost::dynamic_pointer_cast<MapExporter>(node);
		}
	}

NodeExporter::NodeExporter(std::ostream& os) : 
	_outStream(os), 
	_entityCount(0),
	_brushCount(0)
{
	// Check the game file to see whether we need dummy brushes or not
	if (GlobalRegistry().get("game/mapFormat/compatibility/addDummyBrushes") == "true")
		_writeDummyBrushes = true;
	else
		_writeDummyBrushes = false;
		
}
	
// Pre-descent callback
bool NodeExporter::pre(const scene::INodePtr& node) {
	// Check whether we are have a brush or an entity. We might get 
	// called at either level.
    Entity* entity = Node_getEntity(node);

	if(entity != 0) { // ENTITY

		// Push the entity
		_entityStack.push_back(entity);

    	// Write out the entity number comment
		_outStream << "// entity " << _entityCount++ << "\n";

		// Entity opening brace
		_outStream << "{\n";

		// Entity key values
		exportEntity(*entity, _outStream);

		// Reset the brush count 
		_brushCount = 0;

    }
    else  { // BRUSH

		// No entity flag to stack
		_entityStack.push_back(NULL);

    	// Get the brush token exporter
		MapExporterPtr exporter = Node_getMapExporter(node);
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
void NodeExporter::post(const scene::INodePtr& node) {
	// Check if we are popping an entity
	Entity* ent = _entityStack.back();

	if(ent != NULL) {
		
		// If the brush count is 0 and we are adding dummy brushes to please
		// the Doom 3 editor, do so here.
		if (_writeDummyBrushes 
			&& _brushCount == 0
			&& ent->getEntityClass()->isFixedSize()
			&& !ent->getEntityClass()->isLight()) 
		{
			_outStream << DUMMY_BRUSH;
		}

		// Write the closing brace for the entity
		_outStream << "}" << std::endl;

	}

	// Pop the stack
	_entityStack.pop_back();
}

/* Export all of the keyvalues from the given entity, and write them
 * into the given stream.
 */

void NodeExporter::exportEntity(const Entity& entity, std::ostream& os) {
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
    	void visit(const std::string& key, const std::string& value) {
			_os << "\"" << key << "\" \"" << value << "\"\n";
		}

	} visitor(os);

	// Visit the entity
	entity.forEachKeyValue(visitor);
}

} // namespace map
