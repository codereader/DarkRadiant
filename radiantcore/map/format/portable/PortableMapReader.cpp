#include "PortableMapReader.h"

#include <regex>
#include "itextstream.h"
#include "iselectionset.h"
#include "iselectiongroup.h"
#include "ilayer.h"
#include "ibrush.h"
#include "ieclass.h"
#include "ientity.h"

#include "PortableMapFormat.h"
#include "Constants.h"

#include "scenelib.h"
#include "string/convert.h"
#include "xmlutil/Document.h"

namespace map
{

namespace format
{

namespace
{
	class BadDocumentFormatException : 
		public std::runtime_error
	{
	public:
		BadDocumentFormatException(const std::string& message) :
			std::runtime_error(message)
		{}
	};

	// Retrieves the first named child - will throw BadDocumentFormatException if there are 0 or 2+ matching nodes
	inline xml::Node getNamedChild(const xml::Node& node, const std::string& tagName)
	{
		auto children = node.getNamedChildren(tagName);

		if (children.size() != 1)
		{
			throw BadDocumentFormatException("Odd number of " + tagName + " nodes encountered.");
		}

		return children.front();
	}
}

using namespace map::format::constants;

PortableMapReader::PortableMapReader(IMapImportFilter& importFilter) :
	_importFilter(importFilter)
{}

void PortableMapReader::readFromStream(std::istream& stream)
{
	xml::Document doc(stream);

	auto mapNode = doc.getTopLevelNode();

	if (string::convert<std::size_t>(mapNode.getAttributeValue(ATTR_VERSION)) != PortableMapFormat::Version)
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
	try
	{
		_importFilter.getRootNode()->getLayerManager().reset();

		auto mapLayers = getNamedChild(mapNode, TAG_MAP_LAYERS);

		auto layers = mapLayers.getNamedChildren(TAG_MAP_LAYER);

		for (const auto& layer : layers)
		{
			auto id = string::convert<int>(layer.getAttributeValue(ATTR_MAP_LAYER_ID));
			auto name = layer.getAttributeValue(ATTR_MAP_LAYER_NAME);

			_importFilter.getRootNode()->getLayerManager().createLayer(name, id);
		}
	}
	catch (const BadDocumentFormatException& ex)
	{
		rError() << "PortableMapReader: " << ex.what() << std::endl;
	}
}

void PortableMapReader::readSelectionGroups(const xml::Node& mapNode)
{
	try
	{
		assert(_importFilter.getRootNode());
		_importFilter.getRootNode()->getSelectionGroupManager().deleteAllSelectionGroups();

		auto mapSelGroups = getNamedChild(mapNode, TAG_SELECTIONGROUPS);

		auto groups = mapSelGroups.getNamedChildren(TAG_SELECTIONGROUP);

		for (const auto& group : groups)
		{
			auto id = string::convert<std::size_t>(group.getAttributeValue(ATTR_SELECTIONGROUP_ID));
			auto name = group.getAttributeValue(ATTR_SELECTIONGROUP_NAME);

			auto newGroup = _importFilter.getRootNode()->getSelectionGroupManager().createSelectionGroup(id);
			newGroup->setName(name);
		}
	}
	catch (const BadDocumentFormatException & ex)
	{
		rError() << "PortableMapReader: " << ex.what() << std::endl;
	}
}

void PortableMapReader::readSelectionSets(const xml::Node& mapNode)
{
	try
	{
		_selectionSets.clear();

		assert(_importFilter.getRootNode());
		_importFilter.getRootNode()->getSelectionSetManager().deleteAllSelectionSets();

		auto mapSelSets = getNamedChild(mapNode, TAG_SELECTIONSETS);

		auto setNodes = mapSelSets.getNamedChildren(TAG_SELECTIONSET);

		for (const auto& setNode : setNodes)
		{
			auto id = string::convert<std::size_t>(setNode.getAttributeValue(ATTR_SELECTIONSET_ID));
			auto name = setNode.getAttributeValue(ATTR_SELECTIONSET_NAME);

			auto set = _importFilter.getRootNode()->getSelectionSetManager().createSelectionSet(name);
			_selectionSets[id] = set;
		}
	}
	catch (const BadDocumentFormatException & ex)
	{
		rError() << "PortableMapReader: " << ex.what() << std::endl;
	}
}

void PortableMapReader::readMapProperties(const xml::Node& mapNode)
{
	try
	{
		_importFilter.getRootNode()->clearProperties();

		auto mapProperties = getNamedChild(mapNode, TAG_MAP_PROPERTIES);

		auto propertyNodes = mapProperties.getNamedChildren(TAG_MAP_PROPERTY);

		for (const auto& propertyNode : propertyNodes)
		{
			auto key = propertyNode.getAttributeValue(ATTR_MAP_PROPERTY_KEY);
			auto value = propertyNode.getAttributeValue(ATTR_MAP_PROPERTY_VALUE);

			_importFilter.getRootNode()->setProperty(key, value);
		}
	}
	catch (const BadDocumentFormatException & ex)
	{
		rError() << "PortableMapReader: " << ex.what() << std::endl;
	}
}

void PortableMapReader::readEntities(const xml::Node& mapNode)
{
	auto entityNodes = mapNode.getNamedChildren(TAG_ENTITY);

	for (const auto& entityNode : entityNodes)
	{
		try
		{
			readEntity(entityNode);
		}
		catch (const BadDocumentFormatException & ex)
		{
			rError() << "PortableMapReader: Failed to parse entity: " << ex.what() << std::endl;
			continue;
		}
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
	}
}

void PortableMapReader::readBrush(const xml::Node& brushTag, const scene::INodePtr& entity)
{
	// Create a new brush
	auto node = GlobalBrushCreator().createBrush();

	// Cast the node, this must succeed
	auto brushNode = std::dynamic_pointer_cast<IBrushNode>(node);
	assert(brushNode);

	IBrush& brush = brushNode->getIBrush();

	auto facesTag = getNamedChild(brushTag, TAG_FACES);
	auto faceTags = facesTag.getNamedChildren(TAG_FACE);

	for (const auto& faceTag : faceTags)
	{
		try
		{
			auto planeTag = getNamedChild(faceTag, TAG_FACE_PLANE);

			// Construct a plane and parse its values
			Plane3 plane;

			plane.normal().x() = string::to_float(planeTag.getAttributeValue(ATTR_FACE_PLANE_X));
			plane.normal().y() = string::to_float(planeTag.getAttributeValue(ATTR_FACE_PLANE_Y));
			plane.normal().z() = string::to_float(planeTag.getAttributeValue(ATTR_FACE_PLANE_Z));
			plane.dist() = -string::to_float(planeTag.getAttributeValue(ATTR_FACE_PLANE_D)); // negate d

			auto texProjTag = getNamedChild(faceTag, TAG_FACE_TEXPROJ);

			// Parse TexDef
			Matrix3 texdef;

			texdef.xx() = string::to_float(texProjTag.getAttributeValue(ATTR_FACE_TEXTPROJ_XX));
			texdef.yx() = string::to_float(texProjTag.getAttributeValue(ATTR_FACE_TEXTPROJ_YX));
			texdef.zx() = string::to_float(texProjTag.getAttributeValue(ATTR_FACE_TEXTPROJ_TX));
			texdef.xy() = string::to_float(texProjTag.getAttributeValue(ATTR_FACE_TEXTPROJ_XY));
			texdef.yy() = string::to_float(texProjTag.getAttributeValue(ATTR_FACE_TEXTPROJ_YY));
			texdef.zy() = string::to_float(texProjTag.getAttributeValue(ATTR_FACE_TEXTPROJ_TY));

			// Parse Shader
			auto shaderTag = getNamedChild(faceTag, TAG_FACE_MATERIAL);
			std::string shader = shaderTag.getAttributeValue(ATTR_FACE_MATERIAL_NAME);

			// Parse Flags (usually each brush has all faces detail or all faces structural)
			auto detailTag = getNamedChild(faceTag, TAG_FACE_CONTENTSFLAG);

			IBrush::DetailFlag flag = static_cast<IBrush::DetailFlag>(
				string::convert<std::size_t>(detailTag.getAttributeValue(ATTR_FACE_CONTENTSFLAG_VALUE), IBrush::Structural));
			brush.setDetailFlag(flag);

			// Finally, add the new face to the brush
			brush.addFace(plane, texdef, shader);
		}
		catch (const BadDocumentFormatException& ex)
		{
			rError() << "PortableMapReader: Entity " << entity->name() << ", Brush " << 
				brushTag.getAttributeValue(ATTR_BRUSH_NUMBER) << ": " << ex.what() << std::endl;
		}
	}

    // Cleanup redundant face planes
    brush.removeRedundantFaces();

	_importFilter.addPrimitiveToEntity(node, entity);

	readLayerInformation(brushTag, node);
	readSelectionGroupInformation(brushTag, node);
	readSelectionSetInformation(brushTag, node);
}

void PortableMapReader::readPatch(const xml::Node& patchTag, const scene::INodePtr& entity)
{
	bool isFixedSubdiv = patchTag.getAttributeValue(ATTR_PATCH_FIXED_SUBDIV) == ATTR_VALUE_TRUE;

	auto patchType = isFixedSubdiv ? patch::PatchDefType::Def3 : patch::PatchDefType::Def2;

	scene::INodePtr node = GlobalPatchModule().createPatch(patchType);

	auto patchNode = std::dynamic_pointer_cast<IPatchNode>(node);
	assert(patchNode);

	IPatch& patch = patchNode->getPatch();

	// Parse shader
	auto shaderTag = getNamedChild(patchTag, TAG_PATCH_MATERIAL);
	patch.setShader(shaderTag.getAttributeValue(ATTR_PATCH_MATERIAL_NAME));

	std::size_t cols = string::convert<std::size_t>(patchTag.getAttributeValue(ATTR_PATCH_WIDTH));
	std::size_t rows = string::convert<std::size_t>(patchTag.getAttributeValue(ATTR_PATCH_HEIGHT));

	patch.setDims(cols, rows);

	if (isFixedSubdiv)
	{
		// Parse fixed tesselation
		std::size_t subdivX = string::convert<std::size_t>(patchTag.getAttributeValue(ATTR_PATCH_FIXED_SUBDIV_X));
		std::size_t subdivY = string::convert<std::size_t>(patchTag.getAttributeValue(ATTR_PATCH_FIXED_SUBDIV_Y));

		patch.setFixedSubdivisions(true, Subdivisions(subdivX, subdivY));
	}

	auto cvTag = getNamedChild(patchTag, TAG_PATCH_CONTROL_VERTICES);
	auto cvTags = cvTag.getNamedChildren(TAG_PATCH_CONTROL_VERTEX);

	for (const auto& vertexTag : cvTags)
	{
		std::size_t row = string::convert<std::size_t>(vertexTag.getAttributeValue(ATTR_PATCH_CONTROL_VERTEX_ROW));
		std::size_t col = string::convert<std::size_t>(vertexTag.getAttributeValue(ATTR_PATCH_CONTROL_VERTEX_COL));

		auto& ctrl = patch.ctrlAt(row, col);

		ctrl.vertex[0] = string::to_float(vertexTag.getAttributeValue(ATTR_PATCH_CONTROL_VERTEX_X));
		ctrl.vertex[1] = string::to_float(vertexTag.getAttributeValue(ATTR_PATCH_CONTROL_VERTEX_Y));
		ctrl.vertex[2] = string::to_float(vertexTag.getAttributeValue(ATTR_PATCH_CONTROL_VERTEX_Z));

		ctrl.texcoord[0] = string::to_float(vertexTag.getAttributeValue(ATTR_PATCH_CONTROL_VERTEX_U));
		ctrl.texcoord[1] = string::to_float(vertexTag.getAttributeValue(ATTR_PATCH_CONTROL_VERTEX_V));
	}

	patch.controlPointsChanged();

	_importFilter.addPrimitiveToEntity(node, entity);

	readLayerInformation(patchTag, node);
	readSelectionGroupInformation(patchTag, node);
	readSelectionSetInformation(patchTag, node);
}

void PortableMapReader::readEntity(const xml::Node& entityTag)
{
	std::map<std::string, std::string> entityKeyValues{};

	auto keyValuesTag = getNamedChild(entityTag, TAG_ENTITY_KEYVALUES);
	auto keyValueTags = keyValuesTag.getNamedChildren(TAG_ENTITY_KEYVALUE);

	for (const auto& keyValue : keyValueTags)
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
	auto entityNode = GlobalEntityModule().createEntity(eclass);

	for (const auto& pair : entityKeyValues)
	{
		entityNode->getEntity().setKeyValue(pair.first, pair.second);
	}

	readLayerInformation(entityTag, entityNode);
	readSelectionGroupInformation(entityTag, entityNode);
	readSelectionSetInformation(entityTag, entityNode);

	_importFilter.addEntity(entityNode);

	try
	{
		readPrimitives(getNamedChild(entityTag, TAG_ENTITY_PRIMITIVES), entityNode);
	}
	catch (const BadDocumentFormatException & ex)
	{
		rError() << "PortableMapReader: Entity " << entityNode->name() << ": " << ex.what() << std::endl;
	}
}

void PortableMapReader::readLayerInformation(const xml::Node& tag, const scene::INodePtr& sceneNode)
{
	auto layersTag = getNamedChild(tag, TAG_OBJECT_LAYERS);

	auto layerTags = layersTag.getNamedChildren(TAG_OBJECT_LAYER);
	auto layers = scene::LayerList{};

	// Read the list of node IDs
	for (const auto& layerTag : layerTags)
	{
		layers.insert(string::convert<int>(layerTag.getAttributeValue(ATTR_OBJECT_LAYER_ID)));
	}

	sceneNode->assignToLayers(layers);

	sceneNode->foreachNode([&](const scene::INodePtr& child)
	{
		if (!Node_isEntity(child) && !Node_isPrimitive(child))
		{
			child->assignToLayers(layers);
		}

		return true;
	});
}

void PortableMapReader::readSelectionGroupInformation(const xml::Node& tag, const scene::INodePtr& sceneNode)
{
    auto groupsTag = getNamedChild(tag, TAG_OBJECT_SELECTIONGROUPS);
    auto groupTags = groupsTag.getNamedChildren(TAG_OBJECT_SELECTIONGROUP);

    auto& groupManager = _importFilter.getRootNode()->getSelectionGroupManager();

    // Read the list of group IDs
    for (const auto& groupTag : groupTags)
    {
        auto groupId = string::convert<IGroupSelectable::GroupIds::value_type>(
            groupTag.getAttributeValue(ATTR_OBJECT_SELECTIONGROUP_ID)
        );

        auto group = groupManager.getSelectionGroup(groupId);

        if (group)
        {
            group->addNode(sceneNode);
        }
    }
}

void PortableMapReader::readSelectionSetInformation(const xml::Node& tag, const scene::INodePtr& sceneNode)
{
	auto setsTag = getNamedChild(tag, TAG_OBJECT_SELECTIONSETS);
	auto setTags = setsTag.getNamedChildren(TAG_OBJECT_SELECTIONSET);

	// Read the list of set indices
	for (const auto& setTag : setTags)
	{
		auto id = string::convert<std::size_t>(
			setTag.getAttributeValue(ATTR_OBJECT_SELECTIONSET_ID)
		);

		auto setIter = _selectionSets.find(id);
		
		if (setIter != _selectionSets.end())
		{
			setIter->second->addNode(sceneNode);
		}
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
        std::getline(stream, buffer);

		// Check if the format="portable" string is occurring somewhere
		std::regex pattern(R"(<map[^>]+format=\"portable\")");

		if (std::regex_search(buffer, pattern))
		{
			// Try to extract the version number
			std::regex versionPattern(R"(<map[^>]+version=\"(\d+)\")");
			std::smatch results;

			if (std::regex_search(buffer, results, versionPattern) &&
				string::convert<std::size_t>(results[1].str()) <= PortableMapFormat::Version)
			{
				return true;
			}
		}
	}

	return false;
}

}

}
