#include "PortableMapWriter.h"

#include "igame.h"
#include "ientity.h"
#include "ipatch.h"
#include "imap.h"
#include "ibrush.h"
#include "ilayer.h"
#include "iselectiongroup.h"
#include "iselectionset.h"

#include "math/Plane3.h"
#include "math/Matrix4.h"
#include "string/convert.h"
#include "PortableMapFormat.h"
#include "Constants.h"

namespace map
{

namespace format
{

namespace
{

// Checks for NaN and infinity
inline std::string getSafeDouble(double d)
{
	if (isValid(d))
	{
		if (d == -0.0)
		{
			return "0"; // convert -0 to 0
		}
		else
		{
			return string::to_string(d);
		}
	}
	else
	{
		// Is infinity or NaN, write 0
		return "0";
	}
}

}

using namespace map::format::constants;

PortableMapWriter::PortableMapWriter() :
	_entityCount(0),
	_primitiveCount(0),
	_document(xml::Document::create()),
	_map(_document.addTopLevelNode("map")),
	_curEntityPrimitives(nullptr, nullptr)
{
	// Export name and version tag
	_map.setAttributeValue(ATTR_VERSION, string::to_string(PortableMapFormat::Version));
	_map.setAttributeValue(ATTR_FORMAT, ATTR_FORMAT_VALUE);
}

void PortableMapWriter::beginWriteMap(const scene::IMapRootNodePtr& root, std::ostream& stream)
{
	// Write layer information to the header
	auto layers = _map.createChild(TAG_MAP_LAYERS);

	// Visit all layers and add a tag for each
    auto& layerManager = root->getLayerManager();
    auto activeLayerId = layerManager.getActiveLayer();
    layerManager.foreachLayer([&](int layerId, const std::string& layerName)
	{
		auto layer = layers.createChild(TAG_MAP_LAYER);

		layer.setAttributeValue(ATTR_MAP_LAYER_ID, string::to_string(layerId));
		layer.setAttributeValue(ATTR_MAP_LAYER_NAME, layerName);
		layer.setAttributeValue(ATTR_MAP_LAYER_PARENT_ID, string::to_string(layerManager.getParentLayer(layerId)));
        layer.setAttributeValue(ATTR_MAP_LAYER_ACTIVE,
            activeLayerId == layerId ? ATTR_VALUE_TRUE : ATTR_VALUE_FALSE);
        layer.setAttributeValue(ATTR_MAP_LAYER_HIDDEN,
            layerManager.layerIsVisible(layerId) ? ATTR_VALUE_FALSE : ATTR_VALUE_TRUE);
	});

	// Write selection groups
	auto selGroups = _map.createChild(TAG_SELECTIONGROUPS);

	root->getSelectionGroupManager().foreachSelectionGroup([&](selection::ISelectionGroup& group)
	{
		// Ignore empty groups
		if (group.size() == 0) return;

		auto selGroup = selGroups.createChild(TAG_SELECTIONGROUP);

		selGroup.setAttributeValue(ATTR_SELECTIONGROUP_ID, string::to_string(group.getId()));
		selGroup.setAttributeValue(ATTR_SELECTIONGROUP_NAME, group.getName());
	});

	// Write selection sets
	auto selSets = _map.createChild(TAG_SELECTIONSETS);
	std::size_t selectionSetCount = 0;

	// Visit all selection sets
	root->getSelectionSetManager().foreachSelectionSet([&](const selection::ISelectionSetPtr& set)
	{
		auto selSet = selSets.createChild(TAG_SELECTIONSET);

		selSet.setAttributeValue(ATTR_SELECTIONSET_ID, string::to_string(selectionSetCount));
		selSet.setAttributeValue(ATTR_SELECTIONSET_NAME, set->getName());

		// Get all nodes of this selection set and store them for later lookup
		_selectionSets.push_back(SelectionSetExportInfo());

		_selectionSets.back().index = selectionSetCount;
		_selectionSets.back().nodes = set->getNodes();

		selectionSetCount++;
	});

	// Export all map properties
	auto props = _map.createChild(TAG_MAP_PROPERTIES);

	root->foreachProperty([&](const std::string& key, const std::string& value)
	{
		auto property = props.createChild(TAG_MAP_PROPERTY);

		property.setAttributeValue(ATTR_MAP_PROPERTY_KEY, key);
		property.setAttributeValue(ATTR_MAP_PROPERTY_VALUE, value);
	});
}

void PortableMapWriter::endWriteMap(const scene::IMapRootNodePtr& root, std::ostream& stream)
{
	stream << _document.saveToString();
}

void PortableMapWriter::beginWriteEntity(const IEntityNodePtr& entity, std::ostream& stream)
{
	auto node = _map.createChild(TAG_ENTITY);
	node.setAttributeValue(ATTR_ENTITY_NUMBER, string::to_string(_entityCount++));

	auto primitiveNode = node.createChild(TAG_ENTITY_PRIMITIVES);
	_curEntityPrimitives = xml::Node(primitiveNode);

	auto keyValues = node.createChild(TAG_ENTITY_KEYVALUES);

	// Export the entity key values
	entity->getEntity().forEachKeyValue([&](const std::string& key, const std::string& value)
	{
		auto kv = keyValues.createChild(TAG_ENTITY_KEYVALUE);
		kv.setAttributeValue(ATTR_ENTITY_PROPERTY_KEY, key);
		kv.setAttributeValue(ATTR_ENTITY_PROPERTY_VALUE, value);
	});

	appendLayerInformation(node, entity);
	appendSelectionGroupInformation(node, entity);
	appendSelectionSetInformation(node, entity);
}

void PortableMapWriter::endWriteEntity(const IEntityNodePtr& entity, std::ostream& stream)
{
	// Reset the primitive count again
	_primitiveCount = 0;

	_curEntityPrimitives = xml::Node(nullptr, nullptr);
}

void PortableMapWriter::beginWriteBrush(const IBrushNodePtr& brushNode, std::ostream& stream)
{
	assert(_curEntityPrimitives.isValid());

	auto brushTag = _curEntityPrimitives.createChild(TAG_BRUSH);
	brushTag.setAttributeValue(ATTR_BRUSH_NUMBER, string::to_string(_primitiveCount++));

	const auto& brush = brushNode->getIBrush();

	auto facesTag = brushTag.createChild(TAG_FACES);

	// Iterate over each brush face, exporting the tags for each
	for (std::size_t i = 0; i < brush.getNumFaces(); ++i)
	{
		const auto& face = brush.getFace(i);

		// greebo: Don't export faces with degenerate or empty windings (they are "non-contributing")
		if (face.getWinding().size() <= 2)
		{
			return;
		}

		auto faceTag = facesTag.createChild(TAG_FACE);

		// Write the plane equation
		const Plane3& plane = face.getPlane3();

		auto planeTag = faceTag.createChild(TAG_FACE_PLANE);
		planeTag.setAttributeValue(ATTR_FACE_PLANE_X, getSafeDouble(plane.normal().x()));
		planeTag.setAttributeValue(ATTR_FACE_PLANE_Y, getSafeDouble(plane.normal().y()));
		planeTag.setAttributeValue(ATTR_FACE_PLANE_Z, getSafeDouble(plane.normal().z()));
		planeTag.setAttributeValue(ATTR_FACE_PLANE_D, getSafeDouble(-plane.dist()));

		// Write TexDef
		auto textureMatrix = face.getProjectionMatrix();

		auto texTag = faceTag.createChild(TAG_FACE_TEXPROJ);
		texTag.setAttributeValue(ATTR_FACE_TEXTPROJ_XX, getSafeDouble(textureMatrix.xx()));
		texTag.setAttributeValue(ATTR_FACE_TEXTPROJ_YX, getSafeDouble(textureMatrix.yx()));
		texTag.setAttributeValue(ATTR_FACE_TEXTPROJ_TX, getSafeDouble(textureMatrix.zx()));
		texTag.setAttributeValue(ATTR_FACE_TEXTPROJ_XY, getSafeDouble(textureMatrix.xy()));
		texTag.setAttributeValue(ATTR_FACE_TEXTPROJ_YY, getSafeDouble(textureMatrix.yy()));
		texTag.setAttributeValue(ATTR_FACE_TEXTPROJ_TY, getSafeDouble(textureMatrix.zy()));

		// Write Shader
		auto shaderTag = faceTag.createChild(TAG_FACE_MATERIAL);
		shaderTag.setAttributeValue(ATTR_FACE_MATERIAL_NAME, face.getShader());

		// Export (dummy) contents/flags
		auto detailTag = faceTag.createChild(TAG_FACE_CONTENTSFLAG);
		detailTag.setAttributeValue(ATTR_FACE_CONTENTSFLAG_VALUE, string::to_string(brush.getDetailFlag()));
	}

	auto sceneNode = std::dynamic_pointer_cast<scene::INode>(brushNode);
	appendLayerInformation(brushTag, sceneNode);
	appendSelectionGroupInformation(brushTag, sceneNode);
	appendSelectionSetInformation(brushTag, sceneNode);
}

void PortableMapWriter::endWriteBrush(const IBrushNodePtr& brush, std::ostream& stream)
{
	// nothing
}

void PortableMapWriter::beginWritePatch(const IPatchNodePtr& patchNode, std::ostream& stream)
{
	assert(_curEntityPrimitives.isValid());

	auto patchTag = _curEntityPrimitives.createChild(TAG_PATCH);
	patchTag.setAttributeValue(ATTR_PATCH_NUMBER, string::to_string(_primitiveCount++));

	const IPatch& patch = patchNode->getPatch();

	patchTag.setAttributeValue(ATTR_PATCH_WIDTH, string::to_string(patch.getWidth()));
	patchTag.setAttributeValue(ATTR_PATCH_HEIGHT, string::to_string(patch.getHeight()));

	patchTag.setAttributeValue(ATTR_PATCH_FIXED_SUBDIV, patch.subdivisionsFixed() ? ATTR_VALUE_TRUE : ATTR_VALUE_FALSE);

	if (patch.subdivisionsFixed())
	{
		Subdivisions divisions = patch.getSubdivisions();

		patchTag.setAttributeValue(ATTR_PATCH_FIXED_SUBDIV_X, string::to_string(divisions.x()));
		patchTag.setAttributeValue(ATTR_PATCH_FIXED_SUBDIV_Y, string::to_string(divisions.y()));
	}

	// Write Shader
	auto shaderTag = patchTag.createChild(TAG_PATCH_MATERIAL);
	shaderTag.setAttributeValue(ATTR_PATCH_MATERIAL_NAME, patch.getShader());

	auto cvTag = patchTag.createChild(TAG_PATCH_CONTROL_VERTICES);

	for (std::size_t c = 0; c < patch.getWidth(); c++)
	{
		for (std::size_t r = 0; r < patch.getHeight(); r++)
		{
			auto cv = cvTag.createChild(TAG_PATCH_CONTROL_VERTEX);

			cv.setAttributeValue(ATTR_PATCH_CONTROL_VERTEX_ROW, string::to_string(r));
			cv.setAttributeValue(ATTR_PATCH_CONTROL_VERTEX_COL, string::to_string(c));

			const auto& patchControl = patch.ctrlAt(r, c);

			cv.setAttributeValue(ATTR_PATCH_CONTROL_VERTEX_X, getSafeDouble(patchControl.vertex.x()));
			cv.setAttributeValue(ATTR_PATCH_CONTROL_VERTEX_Y, getSafeDouble(patchControl.vertex.y()));
			cv.setAttributeValue(ATTR_PATCH_CONTROL_VERTEX_Z, getSafeDouble(patchControl.vertex.z()));

			cv.setAttributeValue(ATTR_PATCH_CONTROL_VERTEX_U, getSafeDouble(patchControl.texcoord.x()));
			cv.setAttributeValue(ATTR_PATCH_CONTROL_VERTEX_V, getSafeDouble(patchControl.texcoord.y()));
		}
	}

	auto sceneNode = std::dynamic_pointer_cast<scene::INode>(patchNode);
	appendLayerInformation(patchTag, sceneNode);
	appendSelectionGroupInformation(patchTag, sceneNode);
	appendSelectionSetInformation(patchTag, sceneNode);
}

void PortableMapWriter::endWritePatch(const IPatchNodePtr& patch, std::ostream& stream)
{
	// nothing
}

void PortableMapWriter::appendLayerInformation(xml::Node& xmlNode, const scene::INodePtr& sceneNode)
{
	const auto& layers = sceneNode->getLayers();
	auto layersTag = xmlNode.createChild(TAG_OBJECT_LAYERS);

	// Write the list of node IDs
	for (const auto& layerId : layers)
	{
		auto layerTag = layersTag.createChild(TAG_OBJECT_LAYER);
		layerTag.setAttributeValue(ATTR_OBJECT_LAYER_ID, string::to_string(layerId));
	}
}

void PortableMapWriter::appendSelectionGroupInformation(xml::Node& xmlNode, const scene::INodePtr& sceneNode)
{
	auto selectable = std::dynamic_pointer_cast<IGroupSelectable>(sceneNode);

	if (!selectable) return;

	auto groupIds = selectable->getGroupIds();
	auto groupsTag = xmlNode.createChild(TAG_OBJECT_SELECTIONGROUPS);

	// Write the list of group IDs
	for (auto groupId : groupIds)
	{
		auto groupTag = groupsTag.createChild(TAG_OBJECT_SELECTIONGROUP);
		groupTag.setAttributeValue(ATTR_OBJECT_SELECTIONGROUP_ID, string::to_string(groupId));
	}
}

void PortableMapWriter::appendSelectionSetInformation(xml::Node& xmlNode, const scene::INodePtr& sceneNode)
{
	auto sets = xmlNode.createChild(TAG_OBJECT_SELECTIONSETS);

	for (const auto& info : _selectionSets)
	{
		if (info.nodes.find(sceneNode) != info.nodes.end())
		{
			auto setTag = sets.createChild(TAG_OBJECT_SELECTIONSET);
			setTag.setAttributeValue(ATTR_OBJECT_SELECTIONSET_ID, string::to_string(info.index));
		}
	}
}

}

} // namespace
