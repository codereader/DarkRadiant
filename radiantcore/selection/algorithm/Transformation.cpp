#include "Transformation.h"

#include <functional>

#include "i18n.h"
#include <string>
#include <map>
#include "math/Quaternion.h"
#include "iundo.h"
#include "imap.h"
#include "igrid.h"
#include "iorthoview.h"
#include "inamespace.h"
#include "iselection.h"
#include "itextstream.h"
#include "iselectiongroup.h"

#include "scenelib.h"
#include "registry/registry.h"
#include "scene/Clone.h"
#include "map/algorithm/Import.h"
#include "scene/BasicRootNode.h"
#include "debugging/debugging.h"
#include "selection/TransformationVisitors.h"
#include "selection/SceneWalkers.h"
#include "command/ExecutionFailure.h"

#include "string/case_conv.h"

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

AxisBase AxisBase_forViewType(EViewType viewtype)
{
	switch(viewtype)
	{
	case XY:
		return AxisBase(g_vector3_axis_x, g_vector3_axis_y, g_vector3_axis_z);
	case XZ:
		return AxisBase(g_vector3_axis_x, g_vector3_axis_z, g_vector3_axis_y);
	case YZ:
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
void nudgeSelected(ENudgeDirection direction, float amount, EViewType viewtype)
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
	nudgeSelected(direction, GlobalGrid().getGridSize(), GlobalXYWndManager().getActiveViewType());
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

} // namespace algorithm

} // namespace selection
