#include "LayerInfoFileModule.h"

#include "ilayer.h"
#include "ientity.h"
#include "itextstream.h"
#include "scenelib.h"
#include "scene/LayerValidityCheckWalker.h"
#include "string/convert.h"
#include "debugging/ScenegraphUtils.h"
#include "parser/DefTokeniser.h"

namespace scene
{

namespace
{
	constexpr const char* const NODE_TO_LAYER_MAPPING = "NodeToLayerMapping";
	constexpr const char* const LAYER_HIERARCHY = "LayerHierarchy";
	constexpr const char* const LAYER = "Layer";
	constexpr const char* const LAYERS = "Layers";
	constexpr const char* const NODE = "Node";
	constexpr const char* const PARENT = "Parent";
}

LayerInfoFileModule::LayerInfoFileModule() :
	_layerInfoCount(0)
{
	_standardLayerList.insert(0);
}

std::string LayerInfoFileModule::getName()
{
	return "Layer Mapping";
}

void LayerInfoFileModule::clear()
{
	_layerInfoCount = 0;
	_output.str(std::string());
	_output.clear();
	_layerNameBuffer.str(std::string());
	_layerNameBuffer.clear();
    _layerHierarchyBuffer.str(std::string());
    _layerHierarchyBuffer.clear();

	_layerNames.clear();
	_layerMappings.clear();
	_layerParentIds.clear();
}

void LayerInfoFileModule::onInfoFileSaveStart()
{
	clear();
}

void LayerInfoFileModule::onBeginSaveMap(const scene::IMapRootNodePtr& root)
{
	// Open a "Layers" block
	_layerNameBuffer << "\t" << LAYERS << std::endl;
	_layerNameBuffer << "\t{" << std::endl;

    // Open a separate block for the parent-child relationships
    _layerHierarchyBuffer << "\t" << LAYER_HIERARCHY << std::endl;
    _layerHierarchyBuffer << "\t{" << std::endl;

	// Visit all layers and write them to the stream
    auto& layerManager = root->getLayerManager();

    layerManager.foreachLayer([&](int layerId, const std::string& layerName)
	{
		_layerNameBuffer << "\t\t" << LAYER << " " << layerId << " { " << layerName << " }" << std::endl;
        _layerHierarchyBuffer << "\t\t" << LAYER << " " << layerId << " " << PARENT << " { " << layerManager.getParentLayer(layerId) << " }" << std::endl;
	});

    // Close both blocks
	_layerNameBuffer << "\t}" << std::endl;
    _layerHierarchyBuffer << "\t}" << std::endl;
}

void LayerInfoFileModule::onFinishSaveMap(const scene::IMapRootNodePtr& root)
{}

void LayerInfoFileModule::onSavePrimitive(const INodePtr& node, std::size_t entityNum, std::size_t primitiveNum)
{
	saveNode(node);
}

void LayerInfoFileModule::onSaveEntity(const INodePtr& node, std::size_t entityNum)
{
	saveNode(node);
}

void LayerInfoFileModule::saveNode(const INodePtr& node)
{
	// Don't export the layer settings for models and particles, as they are not there
	// at map load/parse time - these shouldn't even be passed in here
	assert(Node_isEntity(node) || Node_isPrimitive(node));

	// Open a Node block
	_output << "\t\t" << NODE << " { ";

	auto layers = node->getLayers();

	// Write a space-separated list of node IDs
	for (const auto& i : layers)
	{
		_output << i << " ";
	}

	// Close the Node block
	_output << "}";

	// Write additional node info, for easier debugging of layer issues
	_output << " // " << getNodeInfo(node);

	_output << std::endl;

	_layerInfoCount++;
}

void LayerInfoFileModule::writeBlocks(std::ostream& stream)
{
	// Write the layer names block
	stream << _layerNameBuffer.str();

    // Write the layer hierarchy block
	stream << _layerHierarchyBuffer.str();

	// Write the NodeToLayerMapping block
	stream << "\t" << NODE_TO_LAYER_MAPPING << std::endl;
	stream << "\t{" << std::endl;

	// Write the output buffer to the stream
    if (_output.tellp() > 0)
    {
	    stream << _output.rdbuf();
    }

	// Closing braces of NodeToLayerMapping block
	stream << "\t}" << std::endl;

	rMessage() << _layerInfoCount << " node-to-layer mappings written." << std::endl;
}

void LayerInfoFileModule::onInfoFileSaveFinished()
{
	clear();
}

void LayerInfoFileModule::onInfoFileLoadStart()
{
	clear();
}

bool LayerInfoFileModule::canParseBlock(const std::string& blockName)
{
	return blockName == LAYERS || blockName == NODE_TO_LAYER_MAPPING || blockName == LAYER_HIERARCHY;
}

void LayerInfoFileModule::parseBlock(const std::string& blockName, parser::DefTokeniser& tok)
{
	assert(canParseBlock(blockName));

	if (blockName == LAYERS)
	{
		parseLayerNames(tok);
	}
	else if (blockName == NODE_TO_LAYER_MAPPING)
	{
		parseNodeToLayerMapping(tok);
	}
    else if (blockName == LAYER_HIERARCHY)
	{
		parseLayerHierarchy(tok);
	}
}

void LayerInfoFileModule::parseLayerNames(parser::DefTokeniser& tok)
{
	// The opening brace
	tok.assertNextToken("{");

	while (tok.hasMoreTokens()) 
	{
		std::string token = tok.nextToken();

		if (token == LAYER)
		{
			// Get the ID
			std::string layerIDStr = tok.nextToken();
			int layerID = string::convert<int>(layerIDStr);

			tok.assertNextToken("{");

			// Assemble the name
			std::string name;

			token = tok.nextToken();

			while (token != "}")
			{
				name += token;
				token = tok.nextToken();
			}

            rDebug() << "[InfoFile]: Parsed layer #" << layerID << " with name " << name << std::endl;

			_layerNames.emplace(layerID, name);

			continue;
		}

		if (token == "}")
		{
			break;
		}
	}
}

void LayerInfoFileModule::parseNodeToLayerMapping(parser::DefTokeniser& tok)
{
	// The opening brace
	tok.assertNextToken("{");

	while (tok.hasMoreTokens())
	{
		std::string token = tok.nextToken();

		if (token == NODE)
		{
			tok.assertNextToken("{");

			// Create a new LayerList
			_layerMappings.push_back(scene::LayerList());

			while (tok.hasMoreTokens())
			{
				std::string nodeToken = tok.nextToken();

				if (nodeToken == "}")
				{
					break;
				}

				// Add the ID to the list
				_layerMappings.back().insert(string::convert<int>(nodeToken));
			}
		}

		if (token == "}")
		{
			break;
		}
	}
}

void LayerInfoFileModule::parseLayerHierarchy(parser::DefTokeniser& tok)
{
    // The opening brace
    tok.assertNextToken("{");

    while (tok.hasMoreTokens())
    {
        std::string token = tok.nextToken();

        if (token == LAYER)
        {
            int layerId = string::convert<int>(tok.nextToken());

            // The block just contains the parent ID
            tok.assertNextToken(PARENT);
            tok.assertNextToken("{");
            auto parentLayerId = string::convert<int>(tok.nextToken());
            tok.assertNextToken("}");

            if (parentLayerId != -1)
            {
                rDebug() << "[InfoFile]: Layer #" << layerId << " is a child of " << parentLayerId << std::endl;
            }

            _layerParentIds.emplace(layerId, parentLayerId);

            continue;
        }

        if (token == "}")
        {
            break;
        }
    }
}

void LayerInfoFileModule::applyInfoToScene(const IMapRootNodePtr& root, const map::NodeIndexMap& nodeMap)
{
    auto& layerManager = root->getLayerManager();

	// Create the layers according to the data found in the map information file
	for (const auto& [id, name] : _layerNames)
	{
		// Create the named layer with the saved ID
        layerManager.createLayer(name, id);

        // Set parents
        auto parentLayer = _layerParentIds.find(id);

        if (parentLayer != _layerParentIds.end())
        {
            layerManager.setParentLayer(id, parentLayer->second);
        }
	}

	// Set the layer mapping iterator to the beginning
	auto mapping = _layerMappings.begin();

	// Assign the layers
	root->foreachNode([&](const INodePtr& node)
	{
		// To prevent all the support node types from getting layers assigned
		// filter them out, only Entities and Primitives get mapped in the info file
		if (Node_isEntity(node) || Node_isPrimitive(node))
		{
			// Check if the node index is out of bounds
			if (mapping == _layerMappings.end())
			{
				node->assignToLayers(_standardLayerList);
				return true;
			}

			// Retrieve the next set of layer mappings and assign them
			node->assignToLayers(*(mapping++));
			return true;
		}

		// All other node types inherit the layers from their parent node
		// Model / particle / target line
        if (auto parent = node->getParent(); parent)
		{
			node->assignToLayers(parent->getLayers());
		}

		return true;
	});

	rDebug() << "Sanity-checking the layer assignments...";

	// Sanity-check the layer mapping, it's possible that some .darkradiant
	// files are mapping nodes to non-existent layer IDs
	LayerValidityCheckWalker checker;
	root->traverseChildren(checker);

	rDebug() << "done, had to fix " << checker.getNumFixed() << " assignments." << std::endl;
}

void LayerInfoFileModule::onInfoFileLoadFinished()
{
	clear();
}

}
