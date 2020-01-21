#include "PortableMapReader.h"

#include <regex>
#include "itextstream.h"
#include "iselectionset.h"
#include "ilayer.h"
#include "PortableMapFormat.h"
#include "Constants.h"
#include "xmlutil/Document.h"
#include "selection/group/SelectionGroupManager.h"

namespace map
{

namespace format
{

using namespace map::format::constants;

PortableMapReader::PortableMapReader(IMapImportFilter& importFilter) :
	_importFilter(importFilter)
{}

void PortableMapReader::readFromStream(std::istream& stream)
{
	xml::Document doc(stream);

	auto mapNode = doc.getTopLevelNode();

	if (string::convert<std::size_t>(mapNode.getAttributeValue(ATTR_VERSION)) != PortableMapFormat::VERSION)
	{
		throw FailureException("Unsupported format version.");
	}

	readLayers(mapNode);
	readSelectionGroups(mapNode);
	readSelectionSets(mapNode);
	readMapProperties(mapNode);

	readEntities(mapNode);
}

void PortableMapReader::readLayers(const xml::Node& mapNode)
{
	GlobalLayerSystem().reset();

	auto mapLayers = mapNode.getNamedChildren(TAG_MAP_LAYERS);

	if (mapLayers.size() != 1)
	{
		rWarning() << "Odd number of " << TAG_MAP_LAYERS << " nodes encountered." << std::endl;
		return;
	}

	auto layers = mapLayers.front().getNamedChildren(TAG_MAP_LAYER);

	for (const auto& layer : layers)
	{
		auto id = string::convert<int>(layer.getAttributeValue(ATTR_MAP_LAYER_ID));
		auto name = layer.getAttributeValue(ATTR_MAP_LAYER_NAME);

		GlobalLayerSystem().createLayer(name, id);
	}
}

void PortableMapReader::readSelectionGroups(const xml::Node& mapNode)
{
	GlobalSelectionGroupManager().deleteAllSelectionGroups();

	auto mapSelGroups = mapNode.getNamedChildren(TAG_SELECTIONGROUPS);

	if (mapSelGroups.size() != 1)
	{
		rWarning() << "Odd number of " << TAG_SELECTIONGROUPS << " nodes encountered." << std::endl;
		return;
	}

	auto groups = mapSelGroups.front().getNamedChildren(TAG_SELECTIONGROUP);

	for (const auto& group : groups)
	{
		auto id = string::convert<std::size_t>(group.getAttributeValue(ATTR_SELECTIONGROUP_ID));
		auto name = group.getAttributeValue(ATTR_SELECTIONGROUP_NAME);

		auto newGroup = selection::getSelectionGroupManagerInternal().createSelectionGroupInternal(id);
		newGroup->setName(name);
	}
}

void PortableMapReader::readSelectionSets(const xml::Node& mapNode)
{
	_selectionSets.clear();
	GlobalSelectionSetManager().deleteAllSelectionSets();

	auto mapSelSets = mapNode.getNamedChildren(TAG_SELECTIONSETS);

	if (mapSelSets.size() != 1)
	{
		rWarning() << "Odd number of " << TAG_SELECTIONSETS << " nodes encountered." << std::endl;
		return;
	}

	auto setNodes = mapSelSets.front().getNamedChildren(TAG_SELECTIONSET);

	for (const auto& setNode : setNodes)
	{
		auto id = string::convert<std::size_t>(setNode.getAttributeValue(ATTR_SELECTIONSET_ID));
		auto name = setNode.getAttributeValue(ATTR_SELECTIONSET_NAME);

		auto set = GlobalSelectionSetManager().createSelectionSet(name);
		_selectionSets[id] = set;
	}
}

void PortableMapReader::readMapProperties(const xml::Node& mapNode)
{
	_importFilter.getRootNode()->clearProperties();

	auto mapProperties = mapNode.getNamedChildren(TAG_MAP_PROPERTIES);

	if (mapProperties.size() != 1)
	{
		rWarning() << "Odd number of " << TAG_MAP_PROPERTIES << " nodes encountered." << std::endl;
		return;
	}

	auto propertyNodes = mapProperties.front().getNamedChildren(TAG_MAP_PROPERTY);

	for (const auto& propertyNode : propertyNodes)
	{
		auto key = propertyNode.getAttributeValue(ATTR_MAP_PROPERTY_KEY);
		auto value = propertyNode.getAttributeValue(ATTR_MAP_PROPERTY_VALUE);

		_importFilter.getRootNode()->setProperty(key, value);
	}
}

void PortableMapReader::readEntities(const xml::Node& mapNode)
{
	auto entityNodes = mapNode.getNamedChildren(TAG_ENTITY);

	for (const auto& entityNode : entityNodes)
	{
		auto entity = readEntity(mapNode);

		auto primitiveNode = entityNode.getNamedChildren(TAG_ENTITY_PRIMITIVES);

		if (primitiveNode.size() != 1)
		{
			rWarning() << "Odd number of " << TAG_MAP_PROPERTIES << " nodes encountered." << std::endl;
			continue;
		}

		readPrimitives(primitiveNode.front(), entity);
	}
}

void PortableMapReader::readPrimitives(const xml::Node& primitivesNode, const scene::INodePtr& entity)
{
	auto childNodes = primitivesNode.getChildren();
	
	for (const auto childNode : childNodes)
	{
		const std::string name = childNode.getName();

		if (name == TAG_BRUSH)
		{
			readBrush(childNode, entity);
		}
		else if (name == TAG_PATCH)
		{
			readPatch(childNode, entity);
		}
		else
		{
			rWarning() << "Unknown primitive tag " << name << " encountered." << std::endl;
			continue;
		}
	}
}

void PortableMapReader::readBrush(const xml::Node& brushNode, const scene::INodePtr& entity)
{

}

void PortableMapReader::readPatch(const xml::Node& patchNode, const scene::INodePtr& entity)
{

}

scene::INodePtr PortableMapReader::readEntity(const xml::Node& entityNode)
{
	std::map<std::string, std::string> entityKeyValues{};

	auto kvNodes = entityNode.getNamedChildren(TAG_ENTITY_KEYVALUES);

	if (kvNodes.size() != 1)
	{
		rWarning() << "Odd number of " << TAG_ENTITY_KEYVALUES << " nodes encountered." << std::endl;
		return;
	}

	auto keyValueNodes = kvNodes.front().getNamedChildren(TAG_ENTITY_KEYVALUE);

	for (const auto& keyValue : keyValueNodes)
	{
		auto key = keyValue.getAttributeValue(ATTR_ENTITY_PROPERTY_KEY);
		auto value = keyValue.getAttributeValue(ATTR_ENTITY_PROPERTY_VALUE);

		entityKeyValues[key] = value;
	}

	// Get the classname from the EntityKeyValues
	auto found = entityKeyValues.find("classname");

	if (found == entityKeyValues.end())
	{
		throw FailureException("PortableMapReader: could not find classname for entity.");
	}

	// Otherwise create the entity and add all of the properties
	std::string className = found->second;
	auto eclass = GlobalEntityClassManager().findClass(className);

	if (!eclass)
	{
		rError() << "PortableMapReader: Could not find entity class: " << className << std::endl;

		// greebo: EntityClass not found, insert a brush-based one
		eclass = GlobalEntityClassManager().findOrInsert(className, true);
	}

	// Create the actual entity node
	auto sceneNode = GlobalEntityCreator().createEntity(eclass);

	for (const auto& pair : entityKeyValues)
	{
		sceneNode->getEntity().setKeyValue(pair.first, pair.second);
	}

	_importFilter.addEntity(sceneNode);

	return sceneNode;
}

bool PortableMapReader::CanLoad(std::istream& stream)
{
	// Instead of instantiating an XML parser and buffering the whole
	// file into memory, read in some lines and look for 
	// certain signature parts.
	// This will fail if the XML is oddly formatted with e.g. line-breaks
	std::string buffer(512, '\0');

	for (int i = 0; i < 25; ++i)
	{
		stream.getline(buffer.data(), buffer.length());

		// Check if the format="portable" string is occurring somewhere
		std::regex pattern(R"(<map[^>]+format=\"portable\")");

		if (std::regex_search(buffer, pattern))
		{
			// Try to extract the version number
			std::regex versionPattern(R"(<map[^>]+version=\"(\d+)\")");
			std::smatch results;

			if (std::regex_search(buffer, results, versionPattern) &&
				string::convert<std::size_t>(results[1].str()) <= PortableMapFormat::VERSION)
			{
				return true;
			}
		}
	}

	return false;
}

}

}
