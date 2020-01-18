#include "PortableMapWriter.h"

#include "igame.h"
#include "ientity.h"
#include "ilayer.h"
#include "iselectiongroup.h"

#include "string/string.h"
#include "selection/group/SelectionGroupManager.h"

namespace map
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

PortableMapWriter::PortableMapWriter() :
	_entityCount(0),
	_primitiveCount(0),
	_document(xml::Document::create()),
	_map(_document.addTopLevelNode("map")),
	_curEntityPrimitives(nullptr)
{}

void PortableMapWriter::beginWriteMap(std::ostream& stream)
{
	// Write layer information to the header
	auto layers = _map.createChild("layers");

	// Visit all layers and add a tag for each
	GlobalLayerSystem().foreachLayer([&](int layerId, const std::string& layerName)
	{
		auto layer = layers.createChild("layer");

		layer.setAttributeValue("id", string::to_string(layerId));
		layer.setAttributeValue("name", layerName);
	});

	// Write selection groups
	std::size_t selectionGroupCount = 0;

	auto selGroups = _map.createChild("selectionGroups");

	selection::getSelectionGroupManagerInternal().foreachSelectionGroup([&](selection::ISelectionGroup& group)
	{
		// Ignore empty groups
		if (group.size() == 0) return;

		auto selGroup = selGroups.createChild("selectionGroup");

		selGroup.setAttributeValue("id", string::to_string(group.getId()));
		selGroup.setAttributeValue("name", string::to_string(group.getName()));
	});
}

void PortableMapWriter::endWriteMap(std::ostream& stream)
{
	stream << _document.saveToString();
}

void PortableMapWriter::beginWriteEntity(const IEntityNodePtr& entity, std::ostream& stream)
{
	auto node = _map.createChild("entity");
	node.setAttributeValue("number", string::to_string(_entityCount++));

	auto primitiveNode = node.createChild("primitives");
	_curEntityPrimitives = xml::Node(primitiveNode.getNodePtr());

	auto keyValues = node.createChild("keyValues");

	// Export the entity key values
	entity->getEntity().forEachKeyValue([&](const std::string& key, const std::string& value)
	{
		auto kv = keyValues.createChild("keyValue");
		kv.setAttributeValue("key", key);
		kv.setAttributeValue("value", value);
	});

	appendLayerInformation(node, entity);
	appendSelectionGroupInformation(node, entity);
}

void PortableMapWriter::endWriteEntity(const IEntityNodePtr& entity, std::ostream& stream)
{
	// Reset the primitive count again
	_primitiveCount = 0;

	_curEntityPrimitives = xml::Node(nullptr);
}

void PortableMapWriter::beginWriteBrush(const IBrushNodePtr& brushNode, std::ostream& stream)
{
	assert(_curEntityPrimitives.getNodePtr() != nullptr);

	auto brushTag = _curEntityPrimitives.createChild("brush");
	brushTag.setAttributeValue("number", string::to_string(_primitiveCount++));

	const auto& brush = brushNode->getIBrush();

	// Iterate over each brush face, exporting the tags for each
	for (std::size_t i = 0; i < brush.getNumFaces(); ++i)
	{
		const auto& face = brush.getFace(i);

		// greebo: Don't export faces with degenerate or empty windings (they are "non-contributing")
		if (face.getWinding().size() <= 2)
		{
			return;
		}

		auto faceTag = brushTag.createChild("face");

		// Write the plane equation
		const Plane3& plane = face.getPlane3();

		auto planeTag = faceTag.createChild("plane");
		planeTag.setAttributeValue("x", getSafeDouble(plane.normal().x()));
		planeTag.setAttributeValue("y", getSafeDouble(plane.normal().y()));
		planeTag.setAttributeValue("z", getSafeDouble(plane.normal().z()));
		planeTag.setAttributeValue("d", getSafeDouble(-plane.dist()));

		// Write TexDef
		Matrix4 texdef = face.getTexDefMatrix();

		auto texTag = faceTag.createChild("textureProjection");
		texTag.setAttributeValue("xx", getSafeDouble(texdef.xx()));
		texTag.setAttributeValue("yx", getSafeDouble(texdef.yx()));
		texTag.setAttributeValue("tx", getSafeDouble(texdef.tx()));
		texTag.setAttributeValue("xy", getSafeDouble(texdef.xy()));
		texTag.setAttributeValue("yy", getSafeDouble(texdef.yy()));
		texTag.setAttributeValue("ty", getSafeDouble(texdef.ty()));

		// Write Shader
		auto shaderTag = faceTag.createChild("material");
		shaderTag.setAttributeValue("name", face.getShader());
		
		// Export (dummy) contents/flags
		auto detailTag = faceTag.createChild("contentsFlag");
		detailTag.setAttributeValue("value", string::to_string(brush.getDetailFlag()));
	}

	auto sceneNode = std::dynamic_pointer_cast<scene::INode>(brushNode);
	appendLayerInformation(brushTag, sceneNode);
	appendSelectionGroupInformation(brushTag, sceneNode);
}

void PortableMapWriter::endWriteBrush(const IBrushNodePtr& brush, std::ostream& stream)
{
	// nothing
}

void PortableMapWriter::beginWritePatch(const IPatchNodePtr& patchNode, std::ostream& stream)
{
	assert(_curEntityPrimitives.getNodePtr() != nullptr);

	auto patchTag = _curEntityPrimitives.createChild("patch");
	patchTag.setAttributeValue("number", string::to_string(_primitiveCount++));

	const IPatch& patch = patchNode->getPatch();

	patchTag.setAttributeValue("width", string::to_string(patch.getWidth()));
	patchTag.setAttributeValue("height", string::to_string(patch.getHeight()));

	patchTag.setAttributeValue("fixedSubdivisions", patch.subdivisionsFixed() ? "true" : "false");

	if (patch.subdivisionsFixed())
	{
		Subdivisions divisions = patch.getSubdivisions();

		patchTag.setAttributeValue("subdivisionsX", string::to_string(divisions.x()));
		patchTag.setAttributeValue("subdivisionsY", string::to_string(divisions.y()));
	}

	// Write Shader
	auto shaderTag = patchTag.createChild("material");
	shaderTag.setAttributeValue("name", patch.getShader());

	auto cvTag = patchTag.createChild("controlVertices");

	for (std::size_t c = 0; c < patch.getWidth(); c++)
	{
		for (std::size_t r = 0; r < patch.getHeight(); r++)
		{
			auto cv = cvTag.createChild("controlVertex");

			cv.setAttributeValue("row", string::to_string(r));
			cv.setAttributeValue("column", string::to_string(c));

			const auto& patchControl = patch.ctrlAt(r, c);

			cv.setAttributeValue("x", getSafeDouble(patchControl.vertex.x()));
			cv.setAttributeValue("y", getSafeDouble(patchControl.vertex.y()));
			cv.setAttributeValue("z", getSafeDouble(patchControl.vertex.z()));

			cv.setAttributeValue("u", getSafeDouble(patchControl.texcoord.x()));
			cv.setAttributeValue("v", getSafeDouble(patchControl.texcoord.y()));
		}
	}

	auto sceneNode = std::dynamic_pointer_cast<scene::INode>(patchNode);
	appendLayerInformation(patchTag, sceneNode);
	appendSelectionGroupInformation(patchTag, sceneNode);
}

void PortableMapWriter::endWritePatch(const IPatchNodePtr& patch, std::ostream& stream)
{
	// nothing
}

void PortableMapWriter::appendLayerInformation(xml::Node& xmlNode, const scene::INodePtr& sceneNode)
{
	auto layers = sceneNode->getLayers();
	auto layersTag = xmlNode.createChild("layers");

	// Write the list of node IDs
	for (const auto& layerId : layers)
	{
		auto layerTag = layersTag.createChild("layer");
		layerTag.setAttributeValue("id", string::to_string(layerId));
	}
}

void PortableMapWriter::appendSelectionGroupInformation(xml::Node& xmlNode, const scene::INodePtr& sceneNode)
{
	auto selectable = std::dynamic_pointer_cast<IGroupSelectable>(sceneNode);

	if (!selectable) return;

	auto groupIds = selectable->getGroupIds();
	auto groupsTag = xmlNode.createChild("selectionGroups");

	// Write the list of group IDs
	for (auto groupId : groupIds)
	{
		auto groupTag = groupsTag.createChild("selectionGroup");
		groupTag.setAttributeValue("id", string::to_string(groupId));
	}
}

} // namespace
