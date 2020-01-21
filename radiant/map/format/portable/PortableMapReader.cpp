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
