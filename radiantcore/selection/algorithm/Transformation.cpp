#include "Transformation.h"

#include <functional>
#include <cmath>
#include <random>
#include <unordered_map>

#include "i18n.h"
#include <string>
#include <map>
#include "math/Quaternion.h"
#include "math/curve.h"
#include "iundo.h"
#include "imap.h"
#include "igrid.h"
#include "iorthoview.h"
#include "inamespace.h"
#include "iselection.h"
#include "itextstream.h"
#include "iselectiongroup.h"
#include "icurve.h"
#include "ientity.h"
#include "iarray.h"
#include "ibrush.h"
#include "icameraview.h"
#include "ipatch.h"

#include "noise/Noise.h"

#include "scenelib.h"
#include "selectionlib.h"
#include "registry/registry.h"
#include "scene/Clone.h"
#include "scene/Entity.h"
#include "scene/EntityNode.h"
#include "map/algorithm/Import.h"
#include "scene/BasicRootNode.h"
#include "debugging/debugging.h"
#include "selection/TransformationVisitors.h"
#include "selection/SceneWalkers.h"
#include "command/ExecutionFailure.h"
#include "parser/Tokeniser.h"
#include "string/convert.h"
#include "brush/FaceInstance.h"

#include "string/case_conv.h"

using namespace ui;

namespace selection
{

namespace algorithm
{

namespace
{
	const std::string RKEY_OFFSET_CLONED_OBJECTS = "user/ui/offsetClonedObjects";
}

void rotateSelected(const Quaternion& rotation)
{
	// Perform the rotation according to the current mode
	if (GlobalSelectionSystem().getSelectionMode() == SelectionMode::Component)
	{
		GlobalSelectionSystem().foreachSelectedComponent(
			RotateComponentSelected(rotation, GlobalSelectionSystem().getPivot2World().translation()));
	}
	else
	{
		// Cycle through the selections and rotate them
		GlobalSelectionSystem().foreachSelected(RotateSelected(rotation,
			GlobalSelectionSystem().getPivot2World().translation()));
	}

	// Update the views
	SceneChangeNotify();

	GlobalSceneGraph().foreachNode(scene::freezeTransformableNode);
}

// greebo: see header for documentation
void rotateSelected(const Vector3& eulerXYZ)
{
	std::string command("rotateSelectedEulerXYZ: ");
	command += string::to_string(eulerXYZ);
	UndoableCommand undo(command.c_str());

	rotateSelected(Quaternion::createForEulerXYZDegrees(eulerXYZ));
}

void rotateSelectedEulerXYZ(const cmd::ArgumentList& args)
{
	if (args.size() != 1)
	{
		rWarning() << "Usage: RotateSelectedEulerXYZ <eulerAngles:Vector3>" << std::endl;
		return;
	}

	rotateSelected(args[0].getVector3());
}

// greebo: see header for documentation
void scaleSelected(const Vector3& scaleXYZ)
{
	if (fabs(scaleXYZ[0]) > 0.0001f &&
		fabs(scaleXYZ[1]) > 0.0001f &&
		fabs(scaleXYZ[2]) > 0.0001f)
	{
		std::string command("scaleSelected: ");
		command += string::to_string(scaleXYZ);
		UndoableCommand undo(command);

		// Pass the scale to the according traversor
		if (GlobalSelectionSystem().getSelectionMode() == SelectionMode::Component)
		{
			GlobalSelectionSystem().foreachSelectedComponent(ScaleComponentSelected(scaleXYZ,
				GlobalSelectionSystem().getPivot2World().translation()));
		}
		else
		{
			GlobalSelectionSystem().foreachSelected(ScaleSelected(scaleXYZ,
				GlobalSelectionSystem().getPivot2World().translation()));
		}

		// Update the scene views
		SceneChangeNotify();

		GlobalSceneGraph().foreachNode(scene::freezeTransformableNode);
	}
	else
	{
		throw cmd::ExecutionFailure(_("Cannot scale by zero value."));
	}
}

void scaleSelectedCmd(const cmd::ArgumentList& args)
{
	if (args.size() != 1)
	{
		rWarning() << "Usage: ScaleSelected <scale:Vector3>" << std::endl;
		return;
	}

	scaleSelected(args[0].getVector3());
}

/** greebo: A visitor class cloning the visited selected items.
 *
 * Use it like this:
 * 1) Traverse the scenegraph, this will create clones.
 * 2) The clones get automatically inserted into a temporary container root.
 * 3) Now move the cloneRoot into a temporary namespace to establish the links.
 * 4) Import the nodes into the target namespace
 * 5) Move the nodes into the target scenegraph (using moveClonedNodes())
 */
class SelectionCloner :
	public scene::NodeVisitor
{
public:
	// This maps cloned nodes to the parent nodes they should be inserted in
	typedef std::map<scene::INodePtr, scene::INodePtr> Map;

private:
	// The map which will associate the cloned nodes to their designated parents
	mutable Map _cloned;

	// A container, which temporarily holds the cloned nodes
    std::shared_ptr<scene::BasicRootNode> _cloneRoot;

	// Map group IDs in this selection to new groups
	typedef std::map<std::size_t, ISelectionGroupPtr> GroupMap;
	GroupMap _groupMap;

public:
	SelectionCloner() :
		_cloneRoot(new scene::BasicRootNode)
	{}

	const std::shared_ptr<scene::BasicRootNode>& getCloneRoot()
	{
		return _cloneRoot;
	}

	const Map& getClonedNodes() const
	{
		return _cloned;
	}

	bool pre(const scene::INodePtr& node)
	{
		// Don't clone root items
		if (node->isRoot())
		{
			return true;
		}

		if (Node_isSelected(node))
		{
			// Don't traverse children of cloned nodes
			return false;
		}

		return true;
	}

	void post(const scene::INodePtr& node)
	{
		if (node->isRoot())
		{
			return;
		}

		if (Node_isSelected(node))
		{
			// Clone the current node
			scene::INodePtr clone = scene::cloneNodeIncludingDescendants(node,
				sigc::mem_fun(*this, &SelectionCloner::postProcessClonedNode));

			// Add the cloned node and its parent to the list
			_cloned.emplace(clone, node->getParent());

			// Insert this node in the root
			_cloneRoot->addChildNode(clone);

			// Cloned child nodes are assigned the layers of the source nodes
			// update the layer visibility flags using the layer manager of the source tree
			scene::UpdateNodeVisibilityWalker visibilityUpdater(node->getRootNode()->getLayerManager());
			clone->traverse(visibilityUpdater);
		}
	}

	void postProcessClonedNode(const scene::INodePtr& sourceNode, const scene::INodePtr& clonedNode)
	{
		// Collect and add the group IDs of the source node
		std::shared_ptr<IGroupSelectable> groupSelectable = std::dynamic_pointer_cast<IGroupSelectable>(sourceNode);

		if (groupSelectable)
		{
			auto sourceRoot = sourceNode->getRootNode();
			assert(sourceRoot);

			const IGroupSelectable::GroupIds& groupIds = groupSelectable->getGroupIds();

			// Get the Groups the source node was assigned to, and add the
			// cloned node to the mapped group, one by one, keeping the order intact
			for (std::size_t id : groupIds)
			{
				// Try to insert the ID, ignore if already exists
				// Get a new mapping for the given group ID
				const ISelectionGroupPtr& mappedGroup = getMappedGroup(id, sourceRoot);

				// Assign the new group ID to this clone
				mappedGroup->addNode(clonedNode);
			}
		}
	}

	// Gets the replacement ID for the given group ID
	const ISelectionGroupPtr& getMappedGroup(std::size_t id, const scene::IMapRootNodePtr& sourceRoot)
	{
		auto found = _groupMap.emplace(id, ISelectionGroupPtr());

		if (!found.second)
		{
			// We already covered this ID, return the mapped group
			return found.first->second;
		}

		// Insertion was successful, so we didn't cover this ID yet
		found.first->second = sourceRoot->getSelectionGroupManager().createSelectionGroup();

		return found.first->second;
	}

	// Adds the cloned nodes to their designated parents. Pass TRUE to select the nodes.
	void moveClonedNodes(bool select)
	{
		for (const auto& pair : _cloned)
		{
			// Remove the child from the basic container first
			_cloneRoot->removeChildNode(pair.first);

			// Add the node to its parent
			pair.second->addChildNode(pair.first);

			// Move the cloned node (and its children) to the active layer
			auto rootNode = pair.second->getRootNode();
			if (rootNode)
			{
				auto activeLayer = rootNode->getLayerManager().getActiveLayer();
				pair.first->moveToLayer(activeLayer);
				pair.first->foreachNode([=](const scene::INodePtr& child)
				{
					child->moveToLayer(activeLayer);
					return true;
				});
			}

			if (select)
			{
				Node_setSelected(pair.first, true);
			}
		}
	}
};

void cloneSelected(const cmd::ArgumentList& args)
{
	// Check for the correct editing mode (don't clone components)
	if (GlobalSelectionSystem().getSelectionMode() == SelectionMode::Component ||
        GlobalMapModule().getEditMode() != IMap::EditMode::Normal)
    {
		return;
	}

	// Get the namespace of the current map
	auto mapRoot = GlobalMapModule().getRoot();
	if (!mapRoot) return; // not map root (this can happen)

	UndoableCommand undo("cloneSelected");

	SelectionCloner cloner;
	GlobalSceneGraph().root()->traverse(cloner);

	// Create a new namespace and move all cloned nodes into it
	INamespacePtr clonedNamespace = GlobalNamespaceFactory().createNamespace();
	assert(clonedNamespace);

	// Move items into the temporary namespace, this will setup the links
	clonedNamespace->connect(cloner.getCloneRoot());

	// Adjust all new names to fit into the existing map namespace
	map::algorithm::prepareNamesForImport(mapRoot, cloner.getCloneRoot());

	// Unselect the current selection
	GlobalSelectionSystem().setSelectedAll(false);

	// Finally, move the cloned nodes to their destination and select them
	cloner.moveClonedNodes(true);

	if (registry::getValue<int>(RKEY_OFFSET_CLONED_OBJECTS) == 1)
	{
		// Move the current selection by one grid unit to the "right" and "downwards"
		nudgeSelected(eNudgeDown);
		nudgeSelected(eNudgeRight);
	}
}

std::vector<scene::INodePtr> createArrayClones(
	const std::vector<scene::INodePtr>& originalSelection,
	int count,
	const std::function<void(int, const scene::INodePtr&)>& applyTransform)
{
	auto mapRoot = GlobalMapModule().getRoot();
	std::vector<scene::INodePtr> allClones;

	if (!mapRoot) return allClones;

	for (int i = 1; i <= count; ++i)
	{
		GlobalSelectionSystem().setSelectedAll(false);

		// Select the original for cloning
		SelectionCloner cloner;
		for (const auto& node : originalSelection)
		{
			Node_setSelected(node, true);
		}

		GlobalSceneGraph().root()->traverse(cloner);
		GlobalSelectionSystem().setSelectedAll(false);

		INamespacePtr clonedNamespace = GlobalNamespaceFactory().createNamespace();
		assert(clonedNamespace);
		clonedNamespace->connect(cloner.getCloneRoot());

		map::algorithm::prepareNamesForImport(mapRoot, cloner.getCloneRoot());
		cloner.moveClonedNodes(false);

		for (const auto& [clonedNode, parentNode] : cloner.getClonedNodes())
		{
			applyTransform(i, clonedNode);
			allClones.push_back(clonedNode);
		}
	}

	for (const auto& node : allClones)
	{
		Node_setSelected(node, true);
	}

	for (const auto& node : originalSelection)
	{
		Node_setSelected(node, true);
	}

	return allClones;
}

void arrayCloneSelectedLine(int count, ArrayOffsetMethod offsetMethod, const Vector3& offset, const Vector3& rotation)
{
	if (GlobalSelectionSystem().getSelectionMode() == SelectionMode::Component ||
		GlobalMapModule().getEditMode() != IMap::EditMode::Normal)
	{
		return;
	}

	if (count < 1) return;

	auto mapRoot = GlobalMapModule().getRoot();
	if (!mapRoot) return;

	UndoableCommand undo("arrayCloneSelectedLine");

	// Store the original selection to iterate over
	std::vector<scene::INodePtr> originalSelection;
	GlobalSelectionSystem().foreachSelected([&](const scene::INodePtr& node)
	{
		originalSelection.push_back(node);
	});

	// Calculate the actual offset based on offset method
	Vector3 effectiveOffset = offset;

	if (offsetMethod == ArrayOffsetMethod::Relative)
	{
		// Get bounding box of selection and multiply offset by its extents
		AABB bounds = GlobalSelectionSystem().getWorkZone().bounds;
		Vector3 extents = bounds.getExtents() * 2; // getExtents returns half-extents
		effectiveOffset = Vector3(
			offset.x() * extents.x(),
			offset.y() * extents.y(),
			offset.z() * extents.z()
		);
	}
	else if (offsetMethod == ArrayOffsetMethod::Endpoint)
	{
		// Offset represents the total distance, divide by count
		effectiveOffset = offset / static_cast<double>(count);
	}

	createArrayClones(originalSelection, count, [&](int i, const scene::INodePtr& clonedNode)
	{
		Vector3 currentOffset = effectiveOffset * static_cast<double>(i);
		Vector3 currentRotation = rotation * static_cast<double>(i);

		ITransformablePtr transformable = scene::node_cast<ITransformable>(clonedNode);
		if (transformable)
		{
			transformable->setType(TRANSFORM_PRIMITIVE);
			transformable->setTranslation(currentOffset);
			transformable->freezeTransform();

			// Apply rotation if any
			if (currentRotation.getLengthSquared() > 0)
			{
				Quaternion rot = Quaternion::createForEulerXYZDegrees(currentRotation);
				transformable->setType(TRANSFORM_PRIMITIVE);
				transformable->setRotation(rot);
				transformable->freezeTransform();
			}
		}
	});
}

void arrayCloneSelectedLineCmd(const cmd::ArgumentList& args)
{
	if (args.size() != 4)
	{
		rWarning() << "Usage: ArrayCloneSelectionLine <count:int> <offsetMethod:int> <offset:Vector3> <rotation:Vector3>" << std::endl;
		return;
	}

	int count = args[0].getInt();
	ArrayOffsetMethod offsetMethod = static_cast<ArrayOffsetMethod>(args[1].getInt());
	Vector3 offset = args[2].getVector3();
	Vector3 rotation = args[3].getVector3();

	arrayCloneSelectedLine(count, offsetMethod, offset, rotation);
}

void arrayCloneSelectedCircle(int count, float radius, float startAngle, float endAngle, bool rotateToCenter)
{
	if (GlobalSelectionSystem().getSelectionMode() == SelectionMode::Component ||
		GlobalMapModule().getEditMode() != IMap::EditMode::Normal)
	{
		return;
	}

	if (count < 1) return;

	auto mapRoot = GlobalMapModule().getRoot();
	if (!mapRoot) return;

	UndoableCommand undo("arrayCloneSelectedCircle");

	// Store the original selection to iterate over
	std::vector<scene::INodePtr> originalSelection;
	GlobalSelectionSystem().foreachSelected([&](const scene::INodePtr& node)
	{
		originalSelection.push_back(node);
	});

	// Get center of selection as the circle center
	AABB bounds = GlobalSelectionSystem().getWorkZone().bounds;
	Vector3 center = bounds.getOrigin();

	// Convert angles to radians
	float startRad = static_cast<float>(degrees_to_radians(startAngle));
	float endRad = static_cast<float>(degrees_to_radians(endAngle));
	float angleRange = endRad - startRad;

	// Distribute copies evenly around the arc
	// If full circle (360 degrees), don't place copy at both start and end
	bool fullCircle = std::abs(endAngle - startAngle) >= 360.0f;
	int divisor = fullCircle ? count : (count > 1 ? count - 1 : 1);

	createArrayClones(originalSelection, count, [&](int i, const scene::INodePtr& clonedNode)
	{
		// Calculate angle for this copy (i starts at 1, so subtract 1 for 0-based index)
		float t = static_cast<float>(i - 1) / static_cast<float>(divisor);
		float angle = startRad + angleRange * t;

		// Calculate position on circle (in XY plane)
		Vector3 offset(
			radius * cos(angle),
			radius * sin(angle),
			0
		);

		ITransformablePtr transformable = scene::node_cast<ITransformable>(clonedNode);
		if (transformable)
		{
			transformable->setType(TRANSFORM_PRIMITIVE);
			transformable->setTranslation(offset);
			transformable->freezeTransform();

			if (rotateToCenter)
			{
				float rotAngle = angle + static_cast<float>(math::PI);
				Quaternion rot = Quaternion::createForZ(rotAngle);
				transformable->setType(TRANSFORM_PRIMITIVE);
				transformable->setRotation(rot);
				transformable->freezeTransform();
			}
		}
	});
}

void arrayCloneSelectedCircleCmd(const cmd::ArgumentList& args)
{
	if (args.size() != 5)
	{
		rWarning() << "Usage: ArrayCloneSelectionCircle <count:int> <radius:float> <startAngle:float> <endAngle:float> <rotateToCenter:int>" << std::endl;
		return;
	}

	int count = args[0].getInt();
	float radius = static_cast<float>(args[1].getDouble());
	float startAngle = static_cast<float>(args[2].getDouble());
	float endAngle = static_cast<float>(args[3].getDouble());
	bool rotateToCenter = args[4].getInt() != 0;

	arrayCloneSelectedCircle(count, radius, startAngle, endAngle, rotateToCenter);
}

void arrayCloneSelectedSpline(int count, bool alignToSpline)
{
	if (GlobalSelectionSystem().getSelectionMode() == SelectionMode::Component ||
		GlobalMapModule().getEditMode() != IMap::EditMode::Normal)
	{
		return;
	}

	if (count < 1) return;

	auto mapRoot = GlobalMapModule().getRoot();
	if (!mapRoot) return;

	// Find the curve entity and other selected nodes
	scene::INodePtr curveNode;
	std::vector<scene::INodePtr> nodesToClone;

	GlobalSelectionSystem().foreachSelected([&](const scene::INodePtr& node)
	{
		CurveNodePtr curve = Node_getCurve(node);
		if (curve && !curve->hasEmptyCurve())
		{
			if (!curveNode)
			{
				curveNode = node;
			}
		}
		else
		{
			nodesToClone.push_back(node);
		}
	});

	if (!curveNode)
	{
		throw cmd::ExecutionFailure(_("Cannot create spline array: No curve entity selected.\nSelect a curve entity along with objects to clone."));
	}

	if (nodesToClone.empty())
	{
		throw cmd::ExecutionFailure(_("Cannot create spline array: No objects selected to clone.\nSelect objects along with the curve entity."));
	}

	// Get the entity world transform to transform control points to world space
	Matrix4 curveTransform = curveNode->localToWorld();

	// We need to get the control points from the entity spawnargs
	Entity* entity = curveNode->tryGetEntity();
	if (!entity)
	{
		throw cmd::ExecutionFailure(_("Cannot create spline array: Could not access curve entity data."));
	}

	// Try to get curve data from entity spawnargs
	std::string curveKey = entity->getKeyValue("curve_CatmullRomSpline");
	if (curveKey.empty())
	{
		curveKey = entity->getKeyValue("curve_Nurbs");
	}

	if (curveKey.empty())
	{
		throw cmd::ExecutionFailure(_("Cannot create spline array: No curve data found on entity."));
	}

	// Parse the curve controls points
	ControlPoints controlPoints;
	parser::BasicStringTokeniser tokeniser(curveKey, " ");
	try
	{
		std::size_t size = string::convert<int>(tokeniser.nextToken());
		if (size < 2)
		{
			throw cmd::ExecutionFailure(_("Cannot create spline array: Curve has less than 2 control points."));
		}

		tokeniser.assertNextToken("(");

		for (std::size_t i = 0; i < size; ++i)
		{
			Vector3 point;
			point.x() = string::convert<float>(tokeniser.nextToken());
			point.y() = string::convert<float>(tokeniser.nextToken());
			point.z() = string::convert<float>(tokeniser.nextToken());

			// Transform to world space
			point = curveTransform.transformPoint(point);
			controlPoints.push_back(point);
		}
	}
	catch (const cmd::ExecutionFailure&)
	{
		throw;
	}
	catch (const std::exception& e)
	{
		throw cmd::ExecutionFailure(std::string(_("Cannot create spline array: Failed to parse curve - ")) + e.what());
	}

	AABB objectsBounds;
	for (const auto& node : nodesToClone)
	{
		objectsBounds.includeAABB(node->worldAABB());
	}
	Vector3 objectsCenter = objectsBounds.getOrigin();

	UndoableCommand undo("arrayCloneSelectedSpline");

	createArrayClones(nodesToClone, count, [&](int i, const scene::INodePtr& clonedNode)
	{
		// Calculate t parameter (0 to 1) along the spline
		double t = (count > 1) ? static_cast<double>(i - 1) / static_cast<double>(count - 1) : 0.0;

		Vector3 position = CatmullRom_evaluate(controlPoints, t);
		Vector3 offset = position - objectsCenter;

		ITransformablePtr transformable = scene::node_cast<ITransformable>(clonedNode);
		if (transformable)
		{
			transformable->setType(TRANSFORM_PRIMITIVE);
			transformable->setTranslation(offset);
			transformable->freezeTransform();

			if (alignToSpline && controlPoints.size() >= 2)
			{
				Vector3 tangent;
				float epsilon = 0.01f;
				if (t + epsilon <= 1.0f)
				{
					Vector3 nextPos = CatmullRom_evaluate(controlPoints, t + epsilon);
					tangent = (nextPos - position).getNormalised();
				}
				else if (t - epsilon >= 0.0f)
				{
					Vector3 prevPos = CatmullRom_evaluate(controlPoints, t - epsilon);
					tangent = (position - prevPos).getNormalised();
				}
				else
				{
					// forward
					tangent = Vector3(1, 0, 0);
				}

				Vector3 forward(1, 0, 0);
				if (tangent.getLengthSquared() > 0.001)
				{
					Quaternion rot = Quaternion::createForUnitVectors(forward, tangent);
					transformable->setType(TRANSFORM_PRIMITIVE);
					transformable->setRotation(rot);
					transformable->freezeTransform();
				}
			}
		}
	});
}

void arrayCloneSelectedSplineCmd(const cmd::ArgumentList& args)
{
	if (args.size() != 2)
	{
		rWarning() << "Usage: ArrayCloneSelectionSpline <count:int> <alignToSpline:int>" << std::endl;
		return;
	}

	int count = args[0].getInt();
	bool alignToSpline = args[1].getInt() != 0;

	arrayCloneSelectedSpline(count, alignToSpline);
}

struct AxisBase
{
	Vector3 x;
	Vector3 y;
	Vector3 z;

	AxisBase(const Vector3& x_, const Vector3& y_, const Vector3& z_) :
		x(x_),
		y(y_),
		z(z_)
	{}
};

AxisBase AxisBase_forViewType(OrthoOrientation viewtype)
{
	switch(viewtype)
	{
	case OrthoOrientation::XY:
		return AxisBase(g_vector3_axis_x, g_vector3_axis_y, g_vector3_axis_z);
	case OrthoOrientation::XZ:
		return AxisBase(g_vector3_axis_x, g_vector3_axis_z, g_vector3_axis_y);
	case OrthoOrientation::YZ:
		return AxisBase(g_vector3_axis_y, g_vector3_axis_z, g_vector3_axis_x);
	}

	ERROR_MESSAGE("invalid viewtype");
	return AxisBase(Vector3(0, 0, 0), Vector3(0, 0, 0), Vector3(0, 0, 0));
}

Vector3 AxisBase_axisForDirection(const AxisBase& axes, ENudgeDirection direction)
{
	switch (direction)
	{
	case eNudgeLeft:
		return -axes.x;
	case eNudgeUp:
		return axes.y;
	case eNudgeRight:
		return axes.x;
	case eNudgeDown:
		return -axes.y;
	}

	ERROR_MESSAGE("invalid direction");
	return Vector3(0, 0, 0);
}

void translateSelected(const Vector3& translation)
{
	// Apply the transformation and freeze the changes
	if (GlobalSelectionSystem().getSelectionMode() == SelectionMode::Component)
	{
		GlobalSelectionSystem().foreachSelectedComponent(TranslateComponentSelected(translation));
	}
	else
	{
		// Cycle through the selected items and apply the translation
		GlobalSelectionSystem().foreachSelected(TranslateSelected(translation));
	}

	// Update the scene so that the changes are made visible
	SceneChangeNotify();

	GlobalSceneGraph().foreachNode(scene::freezeTransformableNode);
}

// Specialised overload, called by the general nudgeSelected() routine
void nudgeSelected(ENudgeDirection direction, float amount, OrthoOrientation viewtype)
{
	AxisBase axes(AxisBase_forViewType(viewtype));

	//Vector3 view_direction(-axes.z);
	Vector3 nudge(AxisBase_axisForDirection(axes, direction) * amount);

	if (GlobalSelectionSystem().getActiveManipulatorType() == selection::IManipulator::Translate ||
        GlobalSelectionSystem().getActiveManipulatorType() == selection::IManipulator::Drag ||
        GlobalSelectionSystem().getActiveManipulatorType() == selection::IManipulator::Clip)
    {
        translateSelected(nudge);

        // In clip mode, update the clipping plane
        if (GlobalSelectionSystem().getActiveManipulatorType() == selection::IManipulator::Clip)
        {
            GlobalClipper().update();
        }
    }
}

void nudgeSelected(ENudgeDirection direction)
{
	nudgeSelected(direction, GlobalGrid().getGridSize(), GlobalOrthoViewManager().getActiveViewType());
}

void nudgeSelectedCmd(const cmd::ArgumentList& args)
{
	if (args.size() != 1)
	{
		rMessage() << "Usage: nudgeSelected [up|down|left|right]" << std::endl;
		return;
	}

	UndoableCommand undo("nudgeSelected");

	std::string arg = string::to_lower_copy(args[0].getString());

	if (arg == "up") {
		nudgeSelected(eNudgeUp);
	}
	else if (arg == "down") {
		nudgeSelected(eNudgeDown);
	}
	else if (arg == "left") {
		nudgeSelected(eNudgeLeft);
	}
	else if (arg == "right") {
		nudgeSelected(eNudgeRight);
	}
	else {
		// Invalid argument
		rMessage() << "Usage: nudgeSelected [up|down|left|right]" << std::endl;
		return;
	}
}

void nudgeByAxis(int nDim, float fNudge)
{
	Vector3 translate(0, 0, 0);
	translate[nDim] = fNudge;

	translateSelected(translate);
}

void moveSelectedAlongZ(float amount)
{
	std::ostringstream command;
	command << "nudgeSelected -axis z -amount " << amount;
	UndoableCommand undo(command.str());

	nudgeByAxis(2, amount);
}

void moveSelectedVerticallyCmd(const cmd::ArgumentList& args)
{
	if (args.size() != 1)
	{
		rMessage() << "Usage: moveSelectionVertically [up|down]" << std::endl;
		return;
	}

	if (GlobalSelectionSystem().countSelected() == 0)
	{
		rMessage() << "Nothing selected." << std::endl;
		return;
	}

	UndoableCommand undo("moveSelectionVertically");

	std::string arg = string::to_lower_copy(args[0].getString());

	if (arg == "up")
	{
		moveSelectedAlongZ(GlobalGrid().getGridSize());
	}
	else if (arg == "down")
	{
		moveSelectedAlongZ(-GlobalGrid().getGridSize());
	}
	else
	{
		// Invalid argument
		rMessage() << "Usage: moveSelectionVertically [up|down]" << std::endl;
		return;
	}
}

void moveSelectedCmd(const cmd::ArgumentList& args)
{
	if (args.size() != 1)
	{
		rMessage() << "Usage: moveSelection <vector>" << std::endl;
		return;
	}

	if (GlobalSelectionSystem().countSelected() == 0)
	{
		rMessage() << "Nothing selected." << std::endl;
		return;
	}

	UndoableCommand undo("moveSelection");

	auto translation = args[0].getVector3();
    translateSelected(translation);
}

enum axis_t
{
	eAxisX = 0,
	eAxisY = 1,
	eAxisZ = 2,
};

enum sign_t
{
	eSignPositive = 1,
	eSignNegative = -1,
};

inline Quaternion quaternion_for_axis90(axis_t axis, sign_t sign)
{
	switch(axis)
	{
	case eAxisX:
		if (sign == eSignPositive)
		{
			return Quaternion(c_half_sqrt2, 0, 0, c_half_sqrt2);
		}
		else
		{
			return Quaternion(-c_half_sqrt2, 0, 0, c_half_sqrt2);
		}
	case eAxisY:
		if(sign == eSignPositive)
		{
			return Quaternion(0, c_half_sqrt2, 0, c_half_sqrt2);
		}
		else
		{
			return Quaternion(0, -c_half_sqrt2, 0, c_half_sqrt2);
		}
	default://case eAxisZ:
		if(sign == eSignPositive)
		{
			return Quaternion(0, 0, c_half_sqrt2, c_half_sqrt2);
		}
		else
		{
			return Quaternion(0, 0, -c_half_sqrt2, c_half_sqrt2);
		}
	}
}

void rotateSelectionX(const cmd::ArgumentList& args)
{
	if (GlobalSelectionSystem().countSelected() == 0)
	{
		rMessage() << "Nothing selected." << std::endl;
		return;
	}

	UndoableCommand undo("rotateSelected -axis x -angle -90");
    rotateSelected(quaternion_for_axis90(eAxisX, eSignNegative));
}

void rotateSelectionY(const cmd::ArgumentList& args)
{
	if (GlobalSelectionSystem().countSelected() == 0)
	{
		rMessage() << "Nothing selected." << std::endl;
		return;
	}

	UndoableCommand undo("rotateSelected -axis y -angle 90");
    rotateSelected(quaternion_for_axis90(eAxisY, eSignPositive));
}

void rotateSelectionZ(const cmd::ArgumentList& args)
{
	if (GlobalSelectionSystem().countSelected() == 0)
	{
		rMessage() << "Nothing selected." << std::endl;
		return;
	}

	UndoableCommand undo("rotateSelected -axis z -angle -90");
    rotateSelected(quaternion_for_axis90(eAxisZ, eSignNegative));
}

void mirrorSelection(int axis)
{
	Vector3 flip(1, 1, 1);
	flip[axis] = -1;

	scaleSelected(flip);
}

void mirrorSelectionX(const cmd::ArgumentList& args)
{
	if (GlobalSelectionSystem().countSelected() == 0)
	{
		rMessage() << "Nothing selected." << std::endl;
		return;
	}

	UndoableCommand undo("mirrorSelected -axis x");
	mirrorSelection(0);
}

void mirrorSelectionY(const cmd::ArgumentList& args)
{
	if (GlobalSelectionSystem().countSelected() == 0)
	{
		rMessage() << "Nothing selected." << std::endl;
		return;
	}

	UndoableCommand undo("mirrorSelected -axis y");
	mirrorSelection(1);
}

void mirrorSelectionZ(const cmd::ArgumentList& args)
{
	if (GlobalSelectionSystem().countSelected() == 0)
	{
		rMessage() << "Nothing selected." << std::endl;
		return;
	}

	UndoableCommand undo("mirrorSelected -axis z");
	mirrorSelection(2);
}

struct ScatterPoint
{
	Vector3 position;
	Vector3 normal;
};

struct FaceGeometry
{
	std::vector<Vector3> vertices;
	Vector3 normal;
	double area;
};

// Calculate the area of a triangle
double triangleArea(const Vector3 &p0, const Vector3 &p1, const Vector3 &p2) {
  Vector3 v1 = p1 - p0;
  Vector3 v2 = p2 - p0;
  return v1.cross(v2).getLength() * 0.5;
}

// Calculate the total area of a polygon
double polygonArea(const std::vector<Vector3> &vertices) {
  if (vertices.size() < 3)
    return 0.0;

  double totalArea = 0.0;
  for (size_t i = 1; i < vertices.size() - 1; ++i) {
    totalArea += triangleArea(vertices[0], vertices[i], vertices[i + 1]);
  }
  return totalArea;
}

// Sample a random point on a triangle using barycentric coordinates
Vector3 sampleTriangle(const Vector3 &p0, const Vector3 &p1, const Vector3 &p2,
                       std::mt19937 &gen) {
  std::uniform_real_distribution<double> dist(0.0, 1.0);
  double r1 = dist(gen);
  double r2 = dist(gen);

  // Ensure point is inside triangle
  if (r1 + r2 > 1.0) {
    r1 = 1.0 - r1;
    r2 = 1.0 - r2;
  }

  return p0 + (p1 - p0) * r1 + (p2 - p0) * r2;
}

// Sample a random point on a polygon
Vector3 samplePolygon(const std::vector<Vector3> &vertices, std::mt19937 &gen) {
  if (vertices.size() < 3)
    return vertices.empty() ? Vector3(0, 0, 0) : vertices[0];

  // Calculate areas of all triangles
  std::vector<double> areas;
  double totalArea = 0.0;

  for (size_t i = 1; i < vertices.size() - 1; ++i) {
    double area = triangleArea(vertices[0], vertices[i], vertices[i + 1]);
    areas.push_back(area);
    totalArea += area;
  }

  if (totalArea <= 0.0)
    return vertices[0];

  // Select a triangle weighted by area
  std::uniform_real_distribution<double> dist(0.0, totalArea);
  double r = dist(gen);

  double cumulative = 0.0;
  size_t selectedTriangle = 0;
  for (size_t i = 0; i < areas.size(); ++i) {
    cumulative += areas[i];
    if (r <= cumulative) {
      selectedTriangle = i;
      break;
    }
  }

  // Sample from the selected triangle
  return sampleTriangle(vertices[0], vertices[selectedTriangle + 1],
                        vertices[selectedTriangle + 2], gen);
}

// Poisson Disk sampling on a set of faces
std::vector<ScatterPoint>
poissonDiskSample(const std::vector<FaceGeometry> &faces, double minDistance,
                  int maxPoints, std::mt19937 &gen) {
  std::vector<ScatterPoint> result;
  const int maxAttempts = 30;

  // Calculate total area
  double totalArea = 0.0;
  for (const auto &face : faces) {
    totalArea += face.area;
  }

  if (totalArea <= 0.0)
    return result;

  // Simple spatial grid for fast neighbor lookup
  double cellSize = minDistance / std::sqrt(2.0);
  std::unordered_map<int64_t, std::vector<size_t>> grid;

  auto getCellKey = [cellSize](const Vector3 &p) -> int64_t {
    int x = static_cast<int>(std::floor(p.x() / cellSize));
    int y = static_cast<int>(std::floor(p.y() / cellSize));
    int z = static_cast<int>(std::floor(p.z() / cellSize));
    // Simple hash combining
    return (static_cast<int64_t>(x) * 73856093) ^
           (static_cast<int64_t>(y) * 19349663) ^
           (static_cast<int64_t>(z) * 83492791);
  };

  auto isTooClose = [&](const Vector3 &p) -> bool {
    int cx = static_cast<int>(std::floor(p.x() / cellSize));
    int cy = static_cast<int>(std::floor(p.y() / cellSize));
    int cz = static_cast<int>(std::floor(p.z() / cellSize));

    // Check neighboring cells
    for (int dx = -2; dx <= 2; ++dx) {
      for (int dy = -2; dy <= 2; ++dy) {
        for (int dz = -2; dz <= 2; ++dz) {
          int64_t key = (static_cast<int64_t>(cx + dx) * 73856093) ^
                        (static_cast<int64_t>(cy + dy) * 19349663) ^
                        (static_cast<int64_t>(cz + dz) * 83492791);

          auto it = grid.find(key);
          if (it != grid.end()) {
            for (size_t idx : it->second) {
              if ((result[idx].position - p).getLengthSquared() <
                  minDistance * minDistance) {
                return true;
              }
            }
          }
        }
      }
    }
    return false;
  };

  // Generate candidate points
  std::uniform_real_distribution<double> areaDist(0.0, totalArea);

  int attempts = 0;
  int maxTotalAttempts = maxPoints * maxAttempts * 10;

  while (static_cast<int>(result.size()) < maxPoints &&
         attempts < maxTotalAttempts) {
    ++attempts;

    // Select a random face weighted by area
    double r = areaDist(gen);
    double cumulative = 0.0;
    const FaceGeometry *selectedFace = &faces[0];

    for (const auto &face : faces) {
      cumulative += face.area;
      if (r <= cumulative) {
        selectedFace = &face;
        break;
      }
    }

    // Sample a point on the selected face
    Vector3 point = samplePolygon(selectedFace->vertices, gen);

    // Check if its not too close from other pointts
    if (!isTooClose(point)) {
      ScatterPoint sp;
      sp.position = point;
      sp.normal = selectedFace->normal;

      // Add to grid
      int64_t key = getCellKey(point);
      grid[key].push_back(result.size());

      result.push_back(sp);
    }
  }

  return result;
}

// Random sampling on faces
std::vector<ScatterPoint> randomSample(const std::vector<FaceGeometry> &faces,
                                       int numPoints, std::mt19937 &gen) {
  std::vector<ScatterPoint> result;

  // Calculate total area
  double totalArea = 0.0;
  for (const auto &face : faces) {
    totalArea += face.area;
  }

  if (totalArea <= 0.0)
    return result;

  std::uniform_real_distribution<double> areaDist(0.0, totalArea);

  for (int i = 0; i < numPoints; ++i) {
    // Select a random face weighted by area
    double r = areaDist(gen);
    double cumulative = 0.0;
    const FaceGeometry *selectedFace = &faces[0];

    for (const auto &face : faces) {
      cumulative += face.area;
      if (r <= cumulative) {
        selectedFace = &face;
        break;
      }
    }

    // Sample a point on the selected face
    ScatterPoint sp;
    sp.position = samplePolygon(selectedFace->vertices, gen);
    sp.normal = selectedFace->normal;
    result.push_back(sp);
  }

  return result;
}

// Create rotation quaternion to align Z-up with a given normal
Quaternion alignToNormal(const Vector3 &normal) {
  Vector3 up(0, 0, 1);

  // If normal is nearly parallel to up, no rotation needed
  double dot = up.dot(normal);
  if (dot > 0.9999) {
    return Quaternion::Identity();
  }

  if (dot < -0.9999) {
    return Quaternion::createForX(math::PI);
  }

  // Calculate rotation axis and angle
  Vector3 axis = up.cross(normal);
  axis.normalise();
  double angle = acos(dot);

  return Quaternion::createForAxisAngle(axis, angle);
}

void scatterObjects(ScatterDensityMethod densityMethod,
                    ScatterDistribution distribution, float density, int amount,
                    float minDistance, int seed,
                    ScatterFaceDirection faceDirection, float rotationRange,
                    bool alignToSurfaceNormal) {
  // Check for the correct editing mode
  if (GlobalMapModule().getEditMode() != IMap::EditMode::Normal) {
    return;
  }

  auto mapRoot = GlobalMapModule().getRoot();
  if (!mapRoot)
    return;

  Vector3 filterDirection;
  bool useCameraFacing = false;
  Vector3 cameraPosition;

  switch (faceDirection) {
  case ScatterFaceDirection::FacingCamera:
    useCameraFacing = true;
    cameraPosition = GlobalCameraManager().getActiveView().getCameraOrigin();
    break;
  case ScatterFaceDirection::PositiveX:
    filterDirection = Vector3(1, 0, 0);
    break;
  case ScatterFaceDirection::NegativeX:
    filterDirection = Vector3(-1, 0, 0);
    break;
  case ScatterFaceDirection::PositiveY:
    filterDirection = Vector3(0, 1, 0);
    break;
  case ScatterFaceDirection::NegativeY:
    filterDirection = Vector3(0, -1, 0);
    break;
  case ScatterFaceDirection::PositiveZ:
    filterDirection = Vector3(0, 0, 1);
    break;
  case ScatterFaceDirection::NegativeZ:
    filterDirection = Vector3(0, 0, -1);
    break;
  }

  // Collect faces from selected brushes and models to scatter
  std::vector<FaceGeometry> faces;
  std::vector<scene::INodePtr> modelsToScatter;

  // Calculate the minimum Z component of normal for upward-facing filter
  GlobalSelectionSystem().foreachSelected([&](const scene::INodePtr &node) {
    // Check if it's a brush - use it as a surface
    if (Node_isBrush(node)) {
      IBrush *brush = Node_getIBrush(node);
      if (brush) {
        // Get the brush's world transform
        Matrix4 brushTransform = node->localToWorld();

        // Iterate over all faces of the brush
        for (std::size_t i = 0; i < brush->getNumFaces(); ++i) {
          IFace &face = brush->getFace(i);
          const IWinding &winding = face.getWinding();

          if (winding.size() >= 3) {
            // Get the face normal in world space
            Vector3 localNormal = face.getPlane3().normal();
            Vector3 worldNormal =
                brushTransform.transformDirection(localNormal).getNormalised();

            // Calculate face center for camera-facing check
            Vector3 faceCenter(0, 0, 0);
            for (std::size_t j = 0; j < winding.size(); ++j) {
              faceCenter += brushTransform.transformPoint(winding[j].vertex);
            }
            faceCenter /= static_cast<double>(winding.size());

            // Filter faces based on direction setting
            bool passesFilter = false;
            if (useCameraFacing) {
              // Face must point toward camera (dot product of normal and
              // direction to camera > 0)
              Vector3 toCamera = (cameraPosition - faceCenter).getNormalised();
              passesFilter = worldNormal.dot(toCamera) > 0.0;
            } else {
              // Face must point in the specified direction (within 90 degrees)
              passesFilter = worldNormal.dot(filterDirection) > 0.0;
            }

            if (!passesFilter) {
              continue;
            }

            FaceGeometry fg;
            fg.normal = worldNormal;

            // Transform vertices to world space
            for (std::size_t j = 0; j < winding.size(); ++j) {
              Vector3 worldVertex =
                  brushTransform.transformPoint(winding[j].vertex);
              fg.vertices.push_back(worldVertex);
            }

            fg.area = polygonArea(fg.vertices);
            if (fg.area > 0.0) {
              faces.push_back(fg);
            }
          }
        }
      }
      return;
    }

    auto entity = node->tryGetEntity();
    if (entity) {
      if (entity->getKeyValue("classname") != "worldspawn") {
        modelsToScatter.push_back(node);
      }
    }
  });

  if (faces.empty()) {
    throw cmd::ExecutionFailure(
        _("Cannot scatter: No brush surfaces found.\nSelect brushes to scatter "
          "on, along with entity models."));
  }

  if (modelsToScatter.empty()) {
    throw cmd::ExecutionFailure(
        _("Cannot scatter: No models selected to scatter.\nSelect entity "
          "models along with the target brushes."));
  }

  double totalArea = 0.0;
  for (const auto &face : faces) {
    totalArea += face.area;
  }

  // Determine how much to scatter
  int numPoints;
  if (densityMethod == ScatterDensityMethod::Amount) {
    numPoints = amount;
  } else {
    numPoints = static_cast<int>(totalArea * density);
    if (numPoints < 1)
      numPoints = 1;
  }

  // Add a top boundary just in case
  if (numPoints > 10000) {
    numPoints = 10000;
  }

  // Initialize random generator with seed
  std::mt19937 gen(seed);

  // Generate scatter points
  std::vector<ScatterPoint> scatterPoints;
  if (distribution == ScatterDistribution::PoissonDisk) {
    scatterPoints = poissonDiskSample(faces, minDistance, numPoints, gen);
  } else {
    scatterPoints = randomSample(faces, numPoints, gen);
  }

  if (scatterPoints.empty()) {
    throw cmd::ExecutionFailure(
        _("Cannot scatter: No valid scatter points generated.\nTry adjusting "
          "density or minimum distance."));
  }

  UndoableCommand undo("scatterObjects");

  // Random distributions for transform variation
  std::uniform_real_distribution<float> rotationDist(0.0f, rotationRange);
  std::uniform_int_distribution<size_t> modelDist(0,
                                                  modelsToScatter.size() - 1);

  // Cache source positions and height offsets before we start cloning
  std::map<scene::INodePtr, Vector3> sourcePositions;
  std::map<scene::INodePtr, double> sourceHeightOffsets;
  for (const auto &sourceNode : modelsToScatter) {
    AABB bounds = sourceNode->worldAABB();
    sourcePositions[sourceNode] = bounds.getOrigin();
    sourceHeightOffsets[sourceNode] = bounds.getExtents().z();
  }

  std::vector<scene::INodePtr> scatteredNodes;

  for (const auto &sp : scatterPoints) {
    GlobalSelectionSystem().setSelectedAll(false);

    // Select a random model to clone
    size_t modelIndex = modelDist(gen);
    const scene::INodePtr &sourceNode = modelsToScatter[modelIndex];

    // Get the source position and height offset
    Vector3 sourcePosition = sourcePositions[sourceNode];
    double heightOffset = sourceHeightOffsets[sourceNode];

    // Clone it
    SelectionCloner cloner;
    Node_setSelected(sourceNode, true);
    GlobalSceneGraph().root()->traverse(cloner);
    Node_setSelected(sourceNode, false);

    INamespacePtr clonedNamespace = GlobalNamespaceFactory().createNamespace();
    clonedNamespace->connect(cloner.getCloneRoot());
    map::algorithm::prepareNamesForImport(mapRoot, cloner.getCloneRoot());

    cloner.moveClonedNodes(false);

    const auto &clonedNodes = cloner.getClonedNodes();
    if (!clonedNodes.empty()) {
      const auto &[clonedNode, parentNode] = *clonedNodes.begin();

      ITransformablePtr transformable =
          scene::node_cast<ITransformable>(clonedNode);
      if (transformable) {
        // First align to surface normal, then add random Z rot
        Quaternion rotation = Quaternion::Identity();

        if (alignToSurfaceNormal) {
          rotation = alignToNormal(sp.normal);
        }

        // Add random rotation around the surface normal
        auto localZRotation = degrees_to_radians(rotationDist(gen));
        Quaternion localZRot = Quaternion::createForZ(localZRotation);
        rotation = rotation.getMultipliedBy(localZRot);

        // Calculate translation from source position to scatter point
        Vector3 scatterPos = sp.position + (sp.normal * heightOffset);
        Vector3 translation = scatterPos - sourcePosition;

        // Apply translation
        transformable->setType(TRANSFORM_PRIMITIVE);
        transformable->setTranslation(translation);
        transformable->freezeTransform();

        // Apply rotation if not identity
        if (rotation.x() != 0 || rotation.y() != 0 || rotation.z() != 0) {
          transformable->setType(TRANSFORM_PRIMITIVE);
          transformable->setRotation(rotation);
          transformable->freezeTransform();
        }
      }

      // Collect for selection at the end
      scatteredNodes.push_back(clonedNode);
    }
  }

  // Select all scattered nodes
  GlobalSelectionSystem().setSelectedAll(false);
  for (const auto &node : scatteredNodes) {
    Node_setSelected(node, true);
  }
}

void scatterObjectsCmd(const cmd::ArgumentList &args) {
  if (args.size() != 9) {
    rWarning()
        << "Usage: ScatterObjects <densityMethod:int> <distribution:int> "
           "<density:float> "
        << "<amount:int> <minDistance:float> <seed:int> <faceDirection:int> "
        << "<rotationRange:float> <alignToNormal:int>" << std::endl;
    return;
  }

  ScatterDensityMethod densityMethod =
      static_cast<ScatterDensityMethod>(args[0].getInt());
  ScatterDistribution distribution =
      static_cast<ScatterDistribution>(args[1].getInt());
  float density = static_cast<float>(args[2].getDouble());
  int amount = args[3].getInt();
  float minDistance = static_cast<float>(args[4].getDouble());
  int seed = args[5].getInt();
  ScatterFaceDirection faceDirection =
      static_cast<ScatterFaceDirection>(args[6].getInt());
  float rotationRange = static_cast<float>(args[7].getDouble());
  bool alignToNormal = args[8].getInt() != 0;

  scatterObjects(densityMethod, distribution, density, amount, minDistance,
                 seed, faceDirection, rotationRange, alignToNormal);
}

void generateTerrainCmd(const cmd::ArgumentList& args)
{
	if (args.size() != 16)
	{
		rWarning() << "Usage: GenerateTerrain <algorithm:int> <seed:int> <frequency:double> <amplitude:double> "
			<< "<octaves:int> <persistence:double> <lacunarity:double> <offset:double> "
			<< "<columns:int> <rows:int> <physicalWidth:double> <physicalHeight:double> "
			<< "<spawnX:double> <spawnY:double> <spawnZ:double> <material:string>" << std::endl;
		return;
	}

	noise::NoiseParameters params;
	params.algorithm = static_cast<noise::Algorithm>(args[0].getInt());
	params.seed = static_cast<unsigned int>(args[1].getInt());
	params.frequency = args[2].getDouble();
	params.amplitude = args[3].getDouble();
	params.octaves = args[4].getInt();
	params.persistence = args[5].getDouble();
	params.lacunarity = args[6].getDouble();
	params.offset = args[7].getDouble();

	std::size_t columns = static_cast<std::size_t>(args[8].getInt());
	std::size_t rows = static_cast<std::size_t>(args[9].getInt());
	float physicalWidth = static_cast<float>(args[10].getDouble());
	float physicalHeight = static_cast<float>(args[11].getDouble());

	Vector3 spawnPos(args[12].getDouble(), args[13].getDouble(), args[14].getDouble());
	std::string material = args[15].getString();

	UndoableCommand undo("terrainGeneratorCreate");

	GlobalSelectionSystem().setSelectedAll(false);

	scene::INodePtr node = GlobalPatchModule().createPatch(patch::PatchDefType::Def2);

	GlobalMapModule().findOrInsertWorldspawn()->addChildNode(node);

	IPatch* patch = Node_getIPatch(node);
	if (!patch)
	{
		return;
	}

	patch->setDims(columns, rows);

	noise::NoiseGenerator noiseGen(params);
	float spacingX = physicalWidth / static_cast<float>(columns - 1);
	float spacingY = physicalHeight / static_cast<float>(rows - 1);
	float offsetX = static_cast<float>(spawnPos.x()) - physicalWidth / 2.0f;
	float offsetY = static_cast<float>(spawnPos.y()) - physicalHeight / 2.0f;
	float baseZ = static_cast<float>(spawnPos.z());

	for (std::size_t row = 0; row < rows; ++row)
	{
		for (std::size_t col = 0; col < columns; ++col)
		{
			PatchControl& ctrl = patch->ctrlAt(row, col);

			float worldX = offsetX + col * spacingX;
			float worldY = offsetY + row * spacingY;
			float noiseValue = static_cast<float>(noiseGen.sample(worldX, worldY));

			ctrl.vertex.x() = worldX;
			ctrl.vertex.y() = worldY;
			ctrl.vertex.z() = baseZ + noiseValue;
			ctrl.texcoord.x() = static_cast<float>(col) / static_cast<float>(columns - 1);
			ctrl.texcoord.y() = static_cast<float>(row) / static_cast<float>(rows - 1);
		}
	}

	patch->controlPointsChanged();
	patch->setShader(material);
	patch->scaleTextureNaturally();

	Node_setSelected(node, true);
}

} // namespace algorithm

} // namespace selection
