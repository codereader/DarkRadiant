#include "Group.h"

#include "selectionlib.h"
#include "scenelib.h"
#include "entitylib.h"
#include "map.h"
#include "gtkutil/dialog.h"
#include "mainframe.h"

namespace selection {

namespace algorithm {
		
class ReparentToEntityWalker : 
	public scene::Graph::Walker
{
	// The old parent
	scene::Node& _newParent;
public:
	ReparentToEntityWalker(scene::Node& parent) : 
		_newParent(parent) 
	{}
	
	bool pre(const scene::Path& path, scene::Instance& instance) const {
		if (path.top().get_pointer() != &_newParent && 
			Node_isPrimitive(path.top()) && 
			path.size() > 1) 
		{
			// Don't traverse the children, it's sufficient, if
			// this node alone is re-parented 
			return false;
		}
		return true;
	}
	
	void post(const scene::Path& path, scene::Instance& instance) const {
		if (path.top().get_pointer() != &_newParent &&
			Node_isPrimitive(path.top()) &&
			path.size() > 1)
		{
			// Retrieve the current parent of the visited instance
			scene::Node& parent = path.parent();
			// Check, if there is work to do in the first place 
			if (&parent != &_newParent) {
				// Extract the node to this instance
				NodeSmartReference node(path.top().get());
				
				// Delete the node from the old parent
				Node_getTraversable(parent)->erase(node);
				
				// and insert it as child of the given parent (passed in the constructor) 
				Node_getTraversable(_newParent)->insert(node);
				
				Selectable* selectable = Instance_getSelectable(instance);
				
				if (selectable != NULL) {
					selectable->setSelected(true);
				}
			}
		}
	}
};

void revertGroupToWorldSpawn() {
	const SelectionInfo& info = GlobalSelectionSystem().getSelectionInfo();
	
	if (info.totalCount == 1 && info.entityCount == 1) {
		scene::Instance& instance = GlobalSelectionSystem().ultimateSelected();
		
		if (node_is_group(instance.path().top())) {
			
			Entity* parent = Node_getEntity(instance.path().top());
			
			if (parent != NULL) {
				// Deselect all, the children get selected after reparenting
				GlobalSelectionSystem().setSelectedAll(false);
				
				// Load the old origin, it has to be added to the brushes after reparenting
				Vector3 parentOrigin(parent->getKeyValue("origin"));
				
				// Get the worldspawn node
	    		scene::Node& worldspawnNode = Map_FindOrInsertWorldspawn(g_map);
	    	
	    		Entity* worldspawn = Node_getEntity(worldspawnNode);
	    		if (worldspawn != NULL) {
	    			// Cycle through all the children and reparent them to the worldspawn node
			    	GlobalSceneGraph().traverse_subgraph(
			    		ReparentToEntityWalker(worldspawnNode), // the visitor class
			    		instance.path()							// start at this path
			    	);
			    	// At this point, all the child primitives have been selected by the walker
			    	
			    	// Check if the old parent entity node is empty
			    	if (Node_getTraversable(instance.path().top())->empty()) {
			    		// Remove this path from the scenegraph
			    		Path_deleteTop(instance.path());
			    	}
			    	else {
			    		globalErrorStream() << "Error while reparenting, cannot delete old parent (not empty)\n"; 
			    	}
			     	
			     	// Flag the map as changed
			     	map::setModified(true);
				}
			}
		} // node_is_group
	}
}

// re-parents the selected brushes/patches
void parentSelection() {
	// Retrieve the selection information structure
	const SelectionInfo& info = GlobalSelectionSystem().getSelectionInfo();
	
	if (info.totalCount > 1 && info.entityCount == 1) {
		UndoableCommand undo("parentSelectedPrimitives");
		
		/*class ParentSelectedBrushesToEntityWalker : public SelectionSystem::Visitor {
			const scene::Path& m_parent;
		public:
			ParentSelectedBrushesToEntityWalker(const scene::Path& parent) : m_parent(parent) {}
			void visit(scene::Instance& instance) const {
				if(&m_parent != &instance.path()) {
					Path_parent(m_parent, instance.path());
				}
			}
		};
	
		ParentSelectedBrushesToEntityWalker visitor(GlobalSelectionSystem().ultimateSelected().path());
		GlobalSelectionSystem().foreachSelected(visitor);*/
	}
	else {
		
	}
}

} // namespace algorithm

} // namespace selection
