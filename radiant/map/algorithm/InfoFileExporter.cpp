#include "InfoFileExporter.h"

#include "imodel.h"
#include "iselectionset.h"
#include "iparticlenode.h"
#include "itextstream.h"
#include "../InfoFile.h"
#include "debugging/ScenegraphUtils.h"

#include <boost/algorithm/string/replace.hpp>

namespace map
{

InfoFileExporter::InfoFileExporter(std::ostream& stream) :
    _stream(stream),
    _layerInfoCount(0)
{
    // Write the information file header
    _stream << InfoFile::HEADER_SEQUENCE << " " << InfoFile::MAP_INFO_VERSION << std::endl;
    _stream << "{" << std::endl;

    // Export the names of the layers
    writeLayerNames();
	assembleSelectionSetInfo();

    // Write the NodeToLayerMapping header
    _stream << "\t" << InfoFile::NODE_TO_LAYER_MAPPING << std::endl;
    _stream << "\t{" << std::endl;
}

InfoFileExporter::~InfoFileExporter()
{
    // Closing braces of NodeToLayerMapping block
    _stream << "\t}" << std::endl;
	
	rMessage() << _layerInfoCount << " node-to-layer mappings written." << std::endl;

	writeSelectionSetInfo();
	
	// Write the closing braces of the information file
    _stream << "}" << std::endl;
}

void InfoFileExporter::handleNode(const scene::INodePtr& node)
{
	// Don't export the layer settings for models and particles, as they are not there
    // at map load/parse time - these shouldn't even be passed in here
    assert(node && !Node_isModel(node) && !particles::isParticleNode(node));

    // Open a Node block
    _stream << "\t\t" << InfoFile::NODE << " { ";

    scene::LayerList layers = node->getLayers();

    // Write a space-separated list of node IDs
    for (scene::LayerList::const_iterator i = layers.begin(); i != layers.end(); ++i)
    {
        _stream << *i << " ";
    }
    
    // Close the Node block
    _stream << "}";

    // Write additional node info, for easier debugging of layer issues
    _stream << " // " << getNodeInfo(node);

    _stream << std::endl;

    _layerInfoCount++;
}

void InfoFileExporter::visitEntity(const scene::INodePtr& node, std::size_t entityNum)
{
	handleNode(node);

	// Determine the item index for the selection set index mapping
	std::for_each(_selectionSetInfo.begin(), _selectionSetInfo.end(), [&] (SelectionSetExportInfo& info)
	{
		if (info.nodes.find(node) != info.nodes.end())
		{
			info.nodeIndices.insert(InfoFileExporter::SelectionSetExportInfo::IndexPair(entityNum, InfoFile::EMPTY_PRIMITVE_NUM));
		}
	});
}

void InfoFileExporter::visitPrimitive(const scene::INodePtr& node, std::size_t entityNum, std::size_t primitiveNum)
{
	handleNode(node);

	// Determine the item index for the selection set index mapping
	std::for_each(_selectionSetInfo.begin(), _selectionSetInfo.end(), [&] (SelectionSetExportInfo& info)
	{
		if (info.nodes.find(node) != info.nodes.end())
		{
			info.nodeIndices.insert(InfoFileExporter::SelectionSetExportInfo::IndexPair(entityNum, primitiveNum));
		}
	});
}

void InfoFileExporter::writeLayerNames()
{
    // Open a "Layers" block
    _stream << "\t" << InfoFile::LAYERS << std::endl;
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
            _os << "\t\t" << InfoFile::LAYER << " " << layerID << " { " << layerName << " }" << std::endl;
        }
    };

    // Visit all layers and write them to the stream
    LayerNameExporter visitor(_stream);
    GlobalLayerSystem().foreachLayer(visitor);

    _stream << "\t}" << std::endl;
}

void InfoFileExporter::writeSelectionSetInfo()
{
	// Selection Set output
	_stream << "\t" << InfoFile::SELECTION_SETS << std::endl;
		
	_stream << "\t{" << std::endl;
	
	std::size_t selectionSetCount = 0;

	std::for_each(_selectionSetInfo.begin(), _selectionSetInfo.end(), [&] (SelectionSetExportInfo& info)
	{
		std::string indices = "";

		std::for_each(info.nodeIndices.begin(), info.nodeIndices.end(), 
			[&] (const InfoFileExporter::SelectionSetExportInfo::IndexPair& pair)
		{
			if (pair.second == InfoFile::EMPTY_PRIMITVE_NUM)
			{
				// only entity number
				indices += "( " + string::to_string(pair.first) + " ) ";
			}
			else
			{
				// entity & primitive number
				indices += "( " + string::to_string(pair.first) + " " + string::to_string(pair.second) +  " ) ";
			}
		});

		// Make sure to escape the quotes of the set name, use the XML quote entity
		_stream << "\t\t" << InfoFile::SELECTION_SET << " " << selectionSetCount++ 
			<< " { \"" << boost::algorithm::replace_all_copy(info.set->getName(), "\"", "&quot;") << "\" } " 
			<< " { " << indices << " } "
			<< std::endl;
	});

	_stream << "\t}" << std::endl;

	rMessage() << _selectionSetInfo.size() << " selection sets exported." << std::endl;
}

void InfoFileExporter::assembleSelectionSetInfo()
{
	// Visit all selection sets and assemble the info into the structures
	GlobalSelectionSetManager().foreachSelectionSet([&] (const selection::ISelectionSetPtr& set)
	{
		// Get all nodes of this selection set and store them for later use
		_selectionSetInfo.push_back(InfoFileExporter::SelectionSetExportInfo());

		_selectionSetInfo.back().set = set;
		_selectionSetInfo.back().nodes = set->getNodes();
	});
}

} // namespace
