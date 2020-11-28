#include "MapResourceLoader.h"

#include "fmt/format.h"
#include "scene/ChildPrimitives.h"
#include "scenelib.h"
#include "algorithm/MapImporter.h"
#include "messages/MapFileOperation.h"

namespace map
{

MapResourceLoader::MapResourceLoader(std::istream& stream, const MapFormat& format) :
    _stream(stream),
    _format(format)
{}

RootNodePtr MapResourceLoader::load()
{
    // Create a new map root node
    auto root = std::make_shared<RootNode>("");

    try
    {
        // Our importer taking care of scene insertion
        MapImporter importFilter(root, _stream);

        // Acquire a map reader/parser
        IMapReaderPtr reader = _format.getMapReader(importFilter);

        rMessage() << "Using " << _format.getMapFormatName() << " format to load the data." << std::endl;

        // Start parsing
        reader->readFromStream(_stream);

        // Prepare child primitives
        scene::addOriginToChildPrimitives(root);

        // Move the index mapping to this class before destroying the import filter
        _indexMapping.swap(importFilter.getNodeMap());

        return root;
    }
    catch (FileOperation::OperationCancelled&)
    {
        // Clear out the root node, otherwise we end up with half a map
        scene::NodeRemover remover;
        root->traverseChildren(remover);

        throw IMapResource::OperationException(_("Map loading cancelled"), true); // cancelled flag set
    }
    catch (IMapReader::FailureException& e)
    {
        // Clear out the root node, otherwise we end up with half a map
        scene::NodeRemover remover;
        root->traverseChildren(remover);

        // Convert the exception, pass the same message
        throw IMapResource::OperationException(e.what());
    }
}

void MapResourceLoader::loadInfoFile(std::istream& stream, const RootNodePtr& root)
{
    if (!stream.good())
    {
        rError() << "[MapResource] No valid info file stream" << std::endl;
        return;
    }

    rMessage() << "Parsing info file..." << std::endl;

    try
    {
        // Read the infofile
        InfoFile infoFile(stream, root, _indexMapping);

        // Start parsing, this will throw if any errors occur
        infoFile.parse();
    }
    catch (parser::ParseException& e)
    {
        rError() << "[MapResource] Unable to parse info file: " << e.what() << std::endl;
    }
}

}
