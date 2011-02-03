#include "InfoFileExporter.h"

#include "imodel.h"
#include "itextstream.h"

namespace map
{

// Tokens / Constants
namespace
{
	// The version of the map info file
	const int MAP_INFO_VERSION = 2;

	// InfoFile tokens --------------------------------------------------
	const char* const HEADER_SEQUENCE = "DarkRadiant Map Information File Version";
	const char* const NODE_TO_LAYER_MAPPING = "NodeToLayerMapping";
	const char* const LAYER = "Layer";
	const char* const LAYERS = "Layers";
	const char* const NODE = "Node";
}

InfoFileExporter::InfoFileExporter(const scene::INodePtr& root, std::ostream& stream) :
	_stream(stream),
	_root(root),
	_layerInfoCount(0)
{
	// Write the information file header
	_stream << HEADER_SEQUENCE << " " << MAP_INFO_VERSION << std::endl;
	_stream << "{" << std::endl;

	// Export the names of the layers
	writeLayerNames();

	// Write the NodeToLayerMapping header
	_stream << "\t" << NODE_TO_LAYER_MAPPING << std::endl;
	_stream << "\t{" << std::endl;
}

InfoFileExporter::~InfoFileExporter()
{
	// Closing braces of NodeToLayerMapping block
	_stream << "\t}" << std::endl;

	// Write the closing braces of the information file
	_stream << "}" << std::endl;

	globalOutputStream() << _layerInfoCount << " node-to-layer mappings written." << std::endl;
}

bool InfoFileExporter::pre(const scene::INodePtr& node)
{
	// Don't export the layer settings for models, as they are not there
	// at map load/parse time.
	if (Node_isModel(node)) 
	{
		return false;
	}

	// Open a Node block
	_stream << "\t\t" << NODE << " { ";

	if (node != NULL)
	{
		scene::LayerList layers = node->getLayers();

		// Write a space-separated list of node IDs
		for (scene::LayerList::iterator i = layers.begin(); i != layers.end(); ++i)
		{
			_stream << *i << " ";
		}
	}

	// Close the Node block
	_stream << "}" << std::endl;

	_layerInfoCount++;

	return true;
}

void InfoFileExporter::writeLayerNames()
{
	// Open a "Layers" block
	_stream << "\t" << LAYERS << std::endl;
	_stream << "\t{" << std::endl;

	// Local helper to traverse the layers
	class LayerNameExporter :
		public scene::ILayerSystem::Visitor
	{
	private:
		// Stream to write to
		std::ostream& _os;
	public:
		// Constructor
		LayerNameExporter(std::ostream& os) :
			_os(os)
	    {}

		// Required visit function
    	void visit(int layerID, const std::string& layerName)
		{
			_os << "\t\t" << LAYER << " " << layerID << " { " << layerName << " }" << std::endl;
		}
	};

	// Visit all layers and write them to the stream
	LayerNameExporter visitor(_stream);
	GlobalLayerSystem().foreachLayer(visitor);

	_stream << "\t}" << std::endl;
}

} // namespace
