#include "NodeExporter.h"

#include "iregistry.h"
#include "ieclass.h"
#include "ientity.h"
#include "ilayer.h"
#include "imodel.h"
#include "stream/textstream.h"

#include "Tokens.h"
#include "Doom3MapFormat.h"

namespace map {

	namespace {
		inline MapExporterPtr Node_getMapExporter(scene::INodePtr node) {
			return boost::dynamic_pointer_cast<MapExporter>(node);
		}
	}

NodeExporter::NodeExporter(std::ostream& mapStream, std::ostream& infoStream) : 
	_mapStream(mapStream), 
	_infoStream(infoStream),
	_entityCount(0),
	_brushCount(0),
	_layerInfoCount(0)
{
	// Check the game file to see whether we need dummy brushes or not
	if (GlobalRegistry().get("game/mapFormat/compatibility/addDummyBrushes") == "true")
		_writeDummyBrushes = true;
	else
		_writeDummyBrushes = false;

	// Write the information file header
	_infoStream << HEADER_SEQUENCE << " " << MAP_INFO_VERSION << "\n";
	_infoStream << "{\n";

	// Export the names of the layers
	writeLayerNames();

	// Write the NodeToLayerMapping header
	_infoStream << "\t" << NODE_TO_LAYER_MAPPING << "\n";
	_infoStream << "\t{\n";
}

NodeExporter::~NodeExporter() {
	// Closing braces of NodeToLayerMapping block
	_infoStream << "\t}\n";

	// Write the closing braces of the information file
	_infoStream << "}\n";

	globalOutputStream() << static_cast<int>(_layerInfoCount) << " node-to-layer mappings written.\n";
}

// Pre-descent callback
bool NodeExporter::pre(const scene::INodePtr& node) {
	// Don't export the layer settings for models, as they are not there
	// at map load/parse time.
	if (!Node_isModel(node)) {
		// Write the layer info to the infostream
		writeNodeLayerInfo(node);
	}

	// Check whether we are have a brush or an entity. We might get 
	// called at either level.
    Entity* entity = Node_getEntity(node);

	if (entity != NULL) { // ENTITY
		// Push the entity
		_entityStack.push_back(entity);

    	// Write out the entity number comment
		_mapStream << "// entity " << _entityCount++ << "\n";

		// Entity opening brace
		_mapStream << "{\n";

		// Entity key values
		exportEntity(*entity);

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
	        _mapStream << "// primitive " << _brushCount++ << "\n";

			// Pass the ostream to the primitive's contained tokenexporter
			exporter->exportTokens(_mapStream);
		}
    }

    return true;
}
  
// Post-descent callback
void NodeExporter::post(const scene::INodePtr& node) {
	// Check if we are popping an entity
	Entity* ent = _entityStack.back();

	if (ent != NULL) {
		// If the brush count is 0 and we are adding dummy brushes to please
		// the Doom 3 editor, do so here.
		/*if (_writeDummyBrushes 
			&& _brushCount == 0
			&& ent->getEntityClass()->isFixedSize()
			&& !ent->getEntityClass()->isLight()) 
		{
			_mapStream << DUMMY_BRUSH;
		}*/

		// Write the closing brace for the entity
		_mapStream << "}" << std::endl;
	}

	// Pop the stack
	_entityStack.pop_back();
}

void NodeExporter::exportEntity(const Entity& entity) {
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

	} visitor(_mapStream);

	// Visit the entity
	entity.forEachKeyValue(visitor);
}

void NodeExporter::writeNodeLayerInfo(const scene::INodePtr& node) {
	// Open a Node block
	_infoStream << "\t\t" << NODE << " { ";

	if (node != NULL) {
		scene::LayerList layers = node->getLayers();

		// Write a space-separated list of node IDs
		for (scene::LayerList::iterator i = layers.begin(); i != layers.end(); i++) {
			_infoStream << *i << " ";
		}
	}

	// Close the Node block
	_infoStream << "}\n";

	_layerInfoCount++;
}

void NodeExporter::writeLayerNames() {
	// Open a "Layers" block
	_infoStream << "\t" << LAYERS << "\n";
	_infoStream << "\t{\n";

	// Local helper to traverse the layers
	class LayerNameExporter : 
		public scene::ILayerSystem::Visitor
	{
		// Stream to write to
		std::ostream& _os;
	public:
		// Constructor
		LayerNameExporter(std::ostream& os) : 
			_os(os)
	    {}

		// Required visit function
    	void visit(int layerID, std::string layerName) {
			_os << "\t\t" << LAYER << " " << layerID << " { " << layerName << " }\n";
		}
	};

	// Visit all layers and write them to the stream
	LayerNameExporter visitor(_infoStream);
	GlobalLayerSystem().foreachLayer(visitor);

	_infoStream << "\t}\n";
}

} // namespace map
