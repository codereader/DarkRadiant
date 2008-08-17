#include "Transformation.h"

#include <string>
#include <map>
#include "math/quaternion.h"
#include "iundo.h"
#include "imap.h"
#include "inamespace.h"
#include "iselection.h"
#include "scenelib.h"
#include "gtkutil/dialog.h"
#include "mainframe.h"
#include "map/algorithm/Clone.h"
#include "map/BasicContainer.h"

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
 *
 * Use it like this:
 * 1) Traverse the scenegraph, this will create clones.
 * 2) The clones get automatically inserted into a temporary container root.
 * 3) Now move the cloneRoot into a temporary namespace to establish the links.
 * 4) Import the nodes into the target namespace
 * 5) Move the nodes into the target scenegraph (using moveClonedNodes())
 */
class SelectionCloner : 
	public scene::Graph::Walker
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

	bool pre(const scene::Path& path, const scene::INodePtr& node) const {
		if (path.size() == 1) {
			return true;
		}

		// Don't clone the root item
		if (!node->isRoot() && Node_isSelected(node)) {
			return false;
		}

		return true;
	}

	void post(const scene::Path& path, const scene::INodePtr& node) const {
		if (path.size() == 1) {
			return;
		}

		if (!node->isRoot() && Node_isSelected(node)) {
			// Clone the current node
			scene::INodePtr clone = map::Node_Clone(node);
			
			// Add the cloned node and its parent to the list 
			_cloned.insert(Map::value_type(clone, path.parent()));

			// Insert this node in the root
			_cloneRoot->addChildNode(clone);
		}
	}

	// Adds the cloned nodes to their designated parents. Pass TRUE to select the nodes.
	void moveClonedNodes(bool select) {
		for (Map::iterator i = _cloned.begin(); i != _cloned.end(); i++) {
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

void cloneSelected() {
	// Check for the correct editing mode (don't clone components)
	if (GlobalSelectionSystem().Mode() != SelectionSystem::ePrimitive) {
		return;
	}

	UndoableCommand undo("cloneSelected");
	
	// Create the list that will take the cloned instances
	SelectionCloner::Map cloned;
	
	SelectionCloner cloner;
	GlobalSceneGraph().traverse(cloner);

	// Create a new namespace and move all cloned nodes into it
	INamespacePtr clonedNamespace = GlobalNamespaceFactory().createNamespace();
	assert(clonedNamespace != NULL);

	// Move items into the temporary namespace, this will setup the links
	clonedNamespace->connect(cloner.getCloneRoot());

	// Get the namespace of the current map
	IMapRootNodePtr mapRoot = GlobalMapModule().getRoot();
	if (mapRoot == NULL) return; // not map root (this can happen)

	INamespacePtr nspace = mapRoot->getNamespace();
	if (nspace != NULL) {
		// Prepare the nodes for import
		nspace->importNames(cloner.getCloneRoot());
		// Now move all nodes into the target namespace
		nspace->connect(cloner.getCloneRoot());
	}

	// Unselect the current selection
	GlobalSelectionSystem().setSelectedAll(false);

	// Finally, move the cloned nodes to their destination and select them
	cloner.moveClonedNodes(true);
}

	} // namespace algorithm
} // namespace selection
