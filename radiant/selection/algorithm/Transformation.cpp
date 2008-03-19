#include "Transformation.h"

#include <string>
#include "math/quaternion.h"
#include "iundo.h"
#include "inamespace.h"
#include "iselection.h"
#include "scenelib.h"
#include "gtkutil/dialog.h"
#include "mainframe.h"
#include "map/algorithm/Clone.h"

namespace selection {
	namespace algorithm {
		
// greebo: see header for documentation
void rotateSelected(const Vector3& eulerXYZ) {
	std::string command("rotateSelectedEulerXYZ: ");
	command += eulerXYZ;
	UndoableCommand undo(command.c_str());

	GlobalSelectionSystem().rotateSelected(quaternion_for_euler_xyz_degrees(eulerXYZ));
}

// greebo: see header for documentation
void scaleSelected(const Vector3& scaleXYZ) {
	
	if (fabs(scaleXYZ[0]) > 0.0001f && 
		fabs(scaleXYZ[1]) > 0.0001f && 
		fabs(scaleXYZ[2]) > 0.0001f) 
	{
		std::string command("scaleSelected: ");
		command += scaleXYZ;
		UndoableCommand undo(command.c_str());
	
		GlobalSelectionSystem().scaleSelected(scaleXYZ);
	}
	else {
		gtkutil::errorDialog("Cannot scale by zero value.", MainFrame_getWindow());
	}
}

/** greebo: A visitor class cloning the visited selected items.
 * 			The items are collected by calling gatherNamespaced()
 * 			and can be "merged" into the existing scenegraph afterwards
 * 			which increments their name postfixes.
 */
class SelectionCloner : 
	public scene::Graph::Walker
{
public:
	typedef std::vector<scene::INodePtr> List;

private:	
	List& _list;

public:
	SelectionCloner(List& list) :
		_list(list)
	{}

	bool pre(const scene::Path& path, const scene::INodePtr& node) const {
		if (path.size() == 1) {
			return true;
		}

		// Don't clone the root item
		if (!node->isRoot()) {
			if (Node_isSelected(node)) {
				return false;
			}
		}

		return true;
	}

	void post(const scene::Path& path, const scene::INodePtr& node) const {
		if (path.size() == 1) {
			return;
		}

		if (!path.top()->isRoot() && Node_isSelected(node)) {
			// Clone the current node
			scene::INodePtr clone = map::Node_Clone(path.top());
			
			// Add this node to the list of namespaced items
			GlobalNamespace().gatherNamespaced(clone);
			
			// Add the cloned node to the list 
			_list.push_back(clone);
			
			// Insert the cloned item to the parent
			path.parent()->addChildNode(clone);
		}
	}
};

/** greebo: Tries to select the given node.
 */
void selectNode(scene::INodePtr node) {
	Node_setSelected(node, true);
}

void cloneSelected() {
	// Check for the correct editing mode (don't clone components)
	if(GlobalSelectionSystem().Mode() == SelectionSystem::ePrimitive) {
		UndoableCommand undo("cloneSelected");
		
		// Create the list that will take the cloned instances
		SelectionCloner::List list;
		
		GlobalSceneGraph().traverse(SelectionCloner(list));
		GlobalNamespace().mergeClonedNames();
		
		// Unselect the current selection
		GlobalSelectionSystem().setSelectedAll(false);
		
		// Now select the cloned nodes
		for (unsigned int i = 0; i < list.size(); i++) {
			selectNode(list[i]);
		}
	}
}

	} // namespace algorithm
} // namespace selection
