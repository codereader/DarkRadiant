#include "Group.h"

#include "selectionlib.h"
#include "entitylib.h"
#include "map.h"
#include "mainframe.h"

namespace selection {

namespace algorithm {
		
class ChildInstanceFinder : 
	public scene::Graph::Walker
{
	InstanceVector& _targetList;
	scene::Path& _parentPath;
	unsigned int _minPathDepth;
public:
	/** greebo: This populates the given targetList with
	 * all the child selectable Instances found under the parentPath.
	 * 
	 * Specify minPathDepth > 1 to choose the children only,
	 * set minPathDepth to 1 to allow choosing the parent as well.
	 */
	ChildInstanceFinder(InstanceVector& targetList, 
						scene::Path& parentPath,
						unsigned int minPathDepth = 1) : 
		_targetList(targetList),
		_parentPath(parentPath),
		_minPathDepth(minPathDepth)
	{}
	
	bool pre(const scene::Path& path, scene::Instance& instance) const {
		
		Selectable* selectable = Instance_getSelectable(instance);
		
		// If a selectable was found and it's not the parent itself, add it to the list
		if (selectable != NULL && 
			!(path == _parentPath) && 
			path.size() >= _minPathDepth) 
		{
			_targetList.push_back(&instance);
		}
		
		return true;
	}
};

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
			     	
			    	// Add the origin vector of the old parent entity to the reparented
			    	// children, as they are positioned relatively to the 0,0,0 origin now. 
				    // Note the minus sign compensating the substract operation of the called method.
				    map::selectedPrimitivesSubtractOrigin(-parentOrigin);
				}
			}
		} // node_is_group
	}
}

} // namespace algorithm

GroupCycle::GroupCycle() :
	_index(0),
	_updateActive(false)
{
	GlobalSelectionSystem().addObserver(this);
	rescanSelection();
}

void GroupCycle::selectionChanged(scene::Instance& instance) {
	rescanSelection();		
}

void GroupCycle::rescanSelection() {
	if (_updateActive) {
		return;
	}
	
	const SelectionInfo& info = GlobalSelectionSystem().getSelectionInfo();
	
	_list.clear();
	_index = 0;
	
	if (info.totalCount == 1 && info.entityCount == 1) {
		scene::Instance& instance = GlobalSelectionSystem().ultimateSelected();
		
		scene::Path startingPath = instance.path();
		algorithm::ChildInstanceFinder walker(_list, startingPath);
		
		GlobalSceneGraph().traverse_subgraph(walker, startingPath);
	}
}

void GroupCycle::updateSelection() {
	_updateActive = true;
	
	// Do some sanity checking before we run into crashes
	if (_index >= 0 && _index < static_cast<int>(_list.size())) {
		for (unsigned int i = 0; i < _list.size(); i++) {
			Selectable* selectable = Instance_getSelectable(*_list[i]);
			selectable->setSelected(false);
		}
		
		Selectable* selectable = Instance_getSelectable(*_list[_index]);
		selectable->setSelected(true);
	}
	
	SceneChangeNotify();
	
	_updateActive = false;
}

void GroupCycle::doCycleForward() {
	if (_list.size() > 0) {
		// Increase the index and wrap around at the list end
		_index = (_index+1) % _list.size();
		
		// Select the new candidate
		updateSelection();
	}
}

void GroupCycle::doCycleBackward() {
	if (_list.size() > 0) {
		// Decrease the index and wrap around, if necessary 
		_index--;
	
		if (_index < 0) {
			_index += _list.size();
		}
	
		// Select the new candidate
		updateSelection();
	}
}

void GroupCycle::cycleBackward() {
	Instance().doCycleBackward();
}

void GroupCycle::cycleForward() {
	Instance().doCycleForward();
}

GroupCycle& GroupCycle::Instance() {
	static GroupCycle _instance;
	
	return _instance;
}

} // namespace selection
