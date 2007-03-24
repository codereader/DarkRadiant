#include "Transformation.h"

#include <string>
#include "math/quaternion.h"
#include "iundo.h"
#include "iselection.h"
#include "scenelib.h"
#include "gtkutil/dialog.h"
#include "mainframe.h"
#include "map.h"

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
	bool pre(const scene::Path& path, scene::Instance& instance) const {
		if (path.size() == 1) {
			return true;
		}

		// Don't clone the root item
		if (!path.top().get().isRoot()) {
			if (Instance_isSelected(instance)) {
				return false;
			}
		}

		return true;
	}

	void post(const scene::Path& path, scene::Instance& instance) const {
		if (path.size() == 1) {
			return;
		}

		if (!path.top().get().isRoot()) {
			if (Instance_isSelected(instance)) {
				// Clone the current node
				NodeSmartReference clone(Node_Clone(path.top()));
				// Add this node to the list of namespaced items
				Map_gatherNamespaced(clone);
				// Insert the cloned item to the parent
				Node_getTraversable(path.parent().get())->insert(clone);
			}
		}
	}
};

void cloneSelected() {
	// Check for the correct editing mode (don't clone components)
	if(GlobalSelectionSystem().Mode() == SelectionSystem::ePrimitive) {
		UndoableCommand undo("cloneSelected");
		
		GlobalSceneGraph().traverse(SelectionCloner());
		Map_mergeClonedNames();
	}
}

	} // namespace algorithm
} // namespace selection
