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
	// The new parent
	scene::INodePtr _newParent;
public:
	ReparentToEntityWalker(scene::INodePtr parent) : 
		_newParent(parent) 
	{}
	
	bool pre(const scene::Path& path, scene::Instance& instance) const {
		if (path.top() != _newParent && 
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
		if (path.top() != _newParent &&
			Node_isPrimitive(path.top()) &&
			path.size() > 1)
		{
			// Retrieve the current parent of the visited instance
			scene::INodePtr parent = path.parent();
			// Check, if there is work to do in the first place 
			if (parent != _newParent) {
				// Extract the node to this instance
				scene::INodePtr node(path.top());
				
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
				
				// Get the worldspawn node
	    		scene::INodePtr worldspawnNode = Map_FindOrInsertWorldspawn(GlobalMap());
	    	
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
			     	GlobalMap().setModified(true);
				}
			}
		} // node_is_group
	}
}

// Some helper methods
bool contains_entity(scene::INodePtr node) {
	return Node_getTraversable(node) != NULL && !Node_isBrush(node) && 
		   !Node_isPatch(node) && !Node_isEntity(node);
}

bool contains_primitive(scene::INodePtr node) {
	return Node_isEntity(node) && Node_getTraversable(node) != NULL 
		   && Node_getEntity(node)->isContainer();
}

ENodeType node_get_contains(scene::INodePtr node) {
	if (contains_entity(node)) {
		return eNodeEntity;
	}
	if (contains_primitive(node)) {
		return eNodePrimitive;
	}
	return eNodeUnknown;
}

class ParentSelectedBrushesToEntityWalker : 
	public SelectionSystem::Visitor
{
	const scene::Path& _parent;
public:
	ParentSelectedBrushesToEntityWalker(const scene::Path& parent) : 
		_parent(parent)
	{}
	
	void visit(scene::Instance& instance) const {
		
		const scene::Path& child = instance.path();
		
		// Don't reparent instances to themselves
		if (&_parent != &child) {
			// The type of the contained items
			ENodeType contains = node_get_contains(_parent.top());
			// The type of the 
			ENodeType type = node_get_nodetype(child.top());
			
			if (contains != eNodeUnknown && contains == type) {
				// Retrieve the child node
				scene::INodePtr node(child.top());
				// Remove this path from the scenegraph
				Path_deleteTop(child);
				// Insert the child node into the parent node 
				Node_getTraversable(_parent.top())->insert(node);
				// Update the scene
				SceneChangeNotify();
			}
			else {
				gtkutil::errorDialog(
					"failed - " + nodetype_get_name(type) + " cannot be parented to " + 
					nodetype_get_name(contains) + " container.\n", 
					GlobalRadiant().getMainWindow()
				);
			}
		}
	}
};

// re-parents the selected brushes/patches
void parentSelection() {
	// Retrieve the selection information structure
	const SelectionInfo& info = GlobalSelectionSystem().getSelectionInfo();
	
	if (info.totalCount > 1 && info.entityCount == 1) {
		UndoableCommand undo("parentSelectedPrimitives");
		
		// Take the last selected item (this should be an entity)
		ParentSelectedBrushesToEntityWalker visitor(
			GlobalSelectionSystem().ultimateSelected().path()
		);
		GlobalSelectionSystem().foreachSelected(visitor);
	}
	else {
		gtkutil::errorDialog("Cannot reparent primitives to entity. "
							 "Please select at least one brush/patch and exactly one entity."
							 "(The entity has to be selected last.)", 
							 GlobalRadiant().getMainWindow());
	}
}

} // namespace algorithm

} // namespace selection
