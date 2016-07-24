#include "InfoFileExporter.h"

#include "imodel.h"
#include "imapinfofile.h"
#include "iparticlenode.h"
#include "itextstream.h"
#include "InfoFile.h"
#include "debugging/ScenegraphUtils.h"

namespace map
{

InfoFileExporter::InfoFileExporter(std::ostream& stream) :
    _stream(stream),
    _layerInfoCount(0)
{
	GlobalMapInfoFileManager().foreachModule([](IMapInfoFileModule& module)
	{
		module.onInfoFileSaveStart();
	});

    // Write the information file header
    _stream << InfoFile::HEADER_SEQUENCE << " " << InfoFile::MAP_INFO_VERSION << std::endl;
    _stream << "{" << std::endl;

    // Export the names of the layers
    writeLayerNames();

    // Write the NodeToLayerMapping header
    _stream << "\t" << InfoFile::NODE_TO_LAYER_MAPPING << std::endl;
    _stream << "\t{" << std::endl;
}

InfoFileExporter::~InfoFileExporter()
{
	// Closing braces of NodeToLayerMapping block
    _stream << "\t}" << std::endl;
	
	rMessage() << _layerInfoCount << " node-to-layer mappings written." << std::endl;

	// Tell the info file modules to write their data now
	GlobalMapInfoFileManager().foreachModule([&](IMapInfoFileModule& module)
	{
		rMessage() << "Writing info file blocks for " << module.getName() << std::endl;

		module.writeBlocks(_stream);
	});

	// Write the closing braces of the information file
    _stream << "}" << std::endl;

	GlobalMapInfoFileManager().foreachModule([](IMapInfoFileModule& module)
	{
		module.onInfoFileSaveFinished();
	});
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
	GlobalMapInfoFileManager().foreachModule([&](IMapInfoFileModule& module)
	{
		module.onSaveEntity(node, entityNum);
	});

	handleNode(node);
}

void InfoFileExporter::visitPrimitive(const scene::INodePtr& node, std::size_t entityNum, std::size_t primitiveNum)
{
	GlobalMapInfoFileManager().foreachModule([&](IMapInfoFileModule& module)
	{
		module.onSavePrimitive(node, entityNum, primitiveNum);
	});

	handleNode(node);
}

void InfoFileExporter::writeLayerNames()
{
    // Open a "Layers" block
    _stream << "\t" << InfoFile::LAYERS << std::endl;
    _stream << "\t{" << std::endl;

    // Visit all layers and write them to the stream
    GlobalLayerSystem().foreachLayer([&](int layerId, const std::string& layerName)
    {
        _stream << "\t\t" << InfoFile::LAYER << " " << layerId << " { " << layerName << " }" << std::endl;
    });

    _stream << "\t}" << std::endl;
}

} // namespace
