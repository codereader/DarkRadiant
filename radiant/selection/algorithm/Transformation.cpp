#include "Transformation.h"

#include "i18n.h"
#include <string>
#include <map>
#include "math/Quaternion.h"
#include "iundo.h"
#include "imap.h"
#include "igrid.h"
#include "inamespace.h"
#include "iselection.h"
#include "imainframe.h"

#include "registry/registry.h"
#include "gtkutil/dialog/MessageBox.h"
#include "xyview/GlobalXYWnd.h"
#include "map/algorithm/Clone.h"
#include "map/BasicContainer.h"

#include <boost/algorithm/string/case_conv.hpp>

namespace selection
{

namespace algorithm
{

// greebo: see header for documentation
void rotateSelected(const Vector3& eulerXYZ)
{
	std::string command("rotateSelectedEulerXYZ: ");
	command += string::to_string(eulerXYZ);
	UndoableCommand undo(command.c_str());

	GlobalSelectionSystem().rotateSelected(Quaternion::createForEulerXYZDegrees(eulerXYZ));
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
		UndoableCommand undo(command.c_str());

		GlobalSelectionSystem().scaleSelected(scaleXYZ);
	}
	else {
		gtkutil::MessageBox::ShowError(_("Cannot scale by zero value."), GlobalMainFrame().getTopLevelWindow());
	}
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
	map::BasicContainerPtr _cloneRoot;

public:
	SelectionCloner() :
		_cloneRoot(new map::BasicContainer)
	{}

	scene::INodePtr getCloneRoot() {
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
			scene::INodePtr clone = map::Node_Clone(node);

			// Add the cloned node and its parent to the list
			_cloned.insert(Map::value_type(clone, node->getParent()));

			// Insert this node in the root
			_cloneRoot->addChildNode(clone);
		}
	}

	// Adds the cloned nodes to their designated parents. Pass TRUE to select the nodes.
	void moveClonedNodes(bool select) {
		for (Map::iterator i = _cloned.begin(); i != _cloned.end(); ++i)
		{
			// Remove the child from the basic container first
			_cloneRoot->removeChildNode(i->first);

			// Add the node to its parent
			i->second->addChildNode(i->first);

			if (select) {
				Node_setSelected(i->first, true);
			}
		}
	}
};

/** greebo: Tries to select the given node.
 */
void selectNode(scene::INodePtr node) {
	Node_setSelected(node, true);
}

void cloneSelected(const cmd::ArgumentList& args) {
	// Check for the correct editing mode (don't clone components)
	if (GlobalSelectionSystem().Mode() == SelectionSystem::eComponent) {
		return;
	}

	UndoableCommand undo("cloneSelected");

	// Create the list that will take the cloned instances
	SelectionCloner::Map cloned;

	SelectionCloner cloner;
	GlobalSceneGraph().root()->traverse(cloner);

	// Create a new namespace and move all cloned nodes into it
	INamespacePtr clonedNamespace = GlobalNamespaceFactory().createNamespace();
	assert(clonedNamespace != NULL);

	// Move items into the temporary namespace, this will setup the links
	clonedNamespace->connect(cloner.getCloneRoot());

	// Get the namespace of the current map
	IMapRootNodePtr mapRoot = GlobalMapModule().getRoot();
	if (mapRoot == NULL) return; // not map root (this can happen)

	INamespacePtr nspace = mapRoot->getNamespace();
	if (nspace)
    {
		// Prepare the nodes for import
		nspace->ensureNoConflicts(cloner.getCloneRoot());
		// Now move all nodes into the target namespace
		nspace->connect(cloner.getCloneRoot());
	}

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

// Specialised overload, called by the general nudgeSelected() routine
void nudgeSelected(ENudgeDirection direction, float amount, EViewType viewtype)
{
	AxisBase axes(AxisBase_forViewType(viewtype));

	Vector3 view_direction(-axes.z);
	Vector3 nudge(AxisBase_axisForDirection(axes, direction) * amount);

	if (GlobalSelectionSystem().ManipulatorMode() == SelectionSystem::eTranslate ||
        GlobalSelectionSystem().ManipulatorMode() == SelectionSystem::eDrag ||
        GlobalSelectionSystem().ManipulatorMode() == SelectionSystem::eClip)
    {
        GlobalSelectionSystem().translateSelected(nudge);

        // In clip mode, update the clipping plane
        if (GlobalSelectionSystem().ManipulatorMode() == SelectionSystem::eClip)
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

	std::string arg = boost::algorithm::to_lower_copy(args[0].getString());

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

	GlobalSelectionSystem().translateSelected(translate);
}

void moveSelectedAlongZ(float amount)
{
	std::ostringstream command;
	command << "nudgeSelected -axis z -amount " << amount;
	UndoableCommand undo(command.str());

	nudgeByAxis(2, amount);
}

void moveSelectedCmd(const cmd::ArgumentList& args)
{
	if (args.size() != 1)
	{
		rMessage() << "Usage: moveSelectionVertically [up|down]" << std::endl;
		return;
	}

	UndoableCommand undo("moveSelectionVertically");

	std::string arg = boost::algorithm::to_lower_copy(args[0].getString());

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
			return Quaternion(-c_half_sqrt2, 0, 0, -c_half_sqrt2);
		}
	case eAxisY:
		if(sign == eSignPositive)
		{
			return Quaternion(0, c_half_sqrt2, 0, c_half_sqrt2);
		}
		else
		{
			return Quaternion(0, -c_half_sqrt2, 0, -c_half_sqrt2);
		}
	default://case eAxisZ:
		if(sign == eSignPositive)
		{
			return Quaternion(0, 0, c_half_sqrt2, c_half_sqrt2);
		}
		else
		{
			return Quaternion(0, 0, -c_half_sqrt2, -c_half_sqrt2);
		}
	}
}

void rotateSelectionAboutAxis(axis_t axis, float deg)
{
	if (fabs(deg) == 90.0f)
	{
		GlobalSelectionSystem().rotateSelected(
			quaternion_for_axis90(axis, (deg > 0) ? eSignPositive : eSignNegative));
	}
	else
	{
		switch(axis)
		{
		case 0:
			GlobalSelectionSystem().rotateSelected(
				Quaternion::createForMatrix(Matrix4::getRotationAboutXDegrees(deg)));
			break;
		case 1:
			GlobalSelectionSystem().rotateSelected(
				Quaternion::createForMatrix(Matrix4::getRotationAboutYDegrees(deg)));
			break;
		case 2:
			GlobalSelectionSystem().rotateSelected(
				Quaternion::createForMatrix(Matrix4::getRotationAboutZDegrees(deg)));
			break;
		}
	}
}

void rotateSelectionX(const cmd::ArgumentList& args)
{
	UndoableCommand undo("rotateSelected -axis x -angle -90");
	rotateSelectionAboutAxis(eAxisX, -90);
}

void rotateSelectionY(const cmd::ArgumentList& args)
{
	UndoableCommand undo("rotateSelected -axis y -angle 90");
	rotateSelectionAboutAxis(eAxisY, 90);
}

void rotateSelectionZ(const cmd::ArgumentList& args)
{
	UndoableCommand undo("rotateSelected -axis z -angle -90");
	rotateSelectionAboutAxis(eAxisZ, -90);
}

void mirrorSelection(int axis)
{
	Vector3 flip(1, 1, 1);
	flip[axis] = -1;

	GlobalSelectionSystem().scaleSelected(flip);
}

void mirrorSelectionX(const cmd::ArgumentList& args)
{
	UndoableCommand undo("mirrorSelected -axis x");
	mirrorSelection(0);
}

void mirrorSelectionY(const cmd::ArgumentList& args)
{
	UndoableCommand undo("mirrorSelected -axis y");
	mirrorSelection(1);
}

void mirrorSelectionZ(const cmd::ArgumentList& args)
{
	UndoableCommand undo("mirrorSelected -axis z");
	mirrorSelection(2);
}

} // namespace algorithm

} // namespace selection
