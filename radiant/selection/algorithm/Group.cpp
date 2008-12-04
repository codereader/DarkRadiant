#include "Group.h"

#include <set>
#include "igroupnode.h"
#include "selectionlib.h"
#include "scenelib.h"
#include "entitylib.h"
#include "map/Map.h"
#include "gtkutil/dialog.h"
#include "mainframe.h"

namespace selection {

namespace algorithm {

/**
 * greebo: This walker traverses a subgraph and reparents all
 * found primitives to the given parent node. After traversal
 * all parent nodes (old and new ones) are updated in terms of
 * their layer visibility state.
 */
class ReparentToEntityWalker : 
	public scene::NodeVisitor
{
	// The new parent
	scene::INodePtr _newParent;

	// The old parent nodes are updated after rearrangement
	std::set<scene::INodePtr> _nodesToUpdate;
public:
	ReparentToEntityWalker(scene::INodePtr parent) : 
		_newParent(parent) 
	{
		// The new parent will be updated too
		_nodesToUpdate.insert(_newParent);
	}

	~ReparentToEntityWalker() {
		scene::UpdateNodeVisibilityWalker updater;
		for (std::set<scene::INodePtr>::iterator i = _nodesToUpdate.begin();
			 i != _nodesToUpdate.end(); i++)
		{
			Node_traverseSubgraph(*i, updater);
		}
	}
	
	bool pre(const scene::INodePtr& node) {
		if (node != _newParent && Node_isPrimitive(node)) {
			// Don't traverse the children, it's sufficient, if
			// this node alone is re-parented 
			return false;
		}
		return true;
	}
	
	void post(const scene::INodePtr& node) {
		if (node != _newParent && Node_isPrimitive(node)) {
			// Retrieve the current parent of the visited node
			scene::INodePtr parent = node->getParent();

			// Check, if there is work to do in the first place 
			if (parent != _newParent) {
				// Copy the shared_ptr
				scene::INodePtr child(node);

				// Delete the node from the old parent
				parent->removeChildNode(child);

				// Mark this parent node for a visibility update
				_nodesToUpdate.insert(parent);
				
				// and insert it as child of the given parent (passed in the constructor) 
				_newParent->addChildNode(child);
				
				// Select the reparented child
				Node_setSelected(child, true);
			}
		}
	}
};

void revertGroupToWorldSpawn() {
		
	typedef std::list<scene::INodePtr> GroupNodeList;

	// Collect all groupnodes
	class GroupNodeCollector : 
		public SelectionSystem::Visitor
	{
		mutable GroupNodeList _groupNodes;
	public:
		void visit(const scene::INodePtr& node) const {
			if (node_is_group(node)) {
				_groupNodes.push_back(node);
			}
		}

		const GroupNodeList& getList() const {
			return _groupNodes;
		}
	}; 

	GroupNodeCollector walker;
	GlobalSelectionSystem().foreachSelected(walker);

	if (walker.getList().empty()) {
		return; // nothing to do!
	}

	// Deselect all, the children get selected after reparenting
	GlobalSelectionSystem().setSelectedAll(false);

	// Get the worldspawn node
	scene::INodePtr worldspawnNode = GlobalMap().findOrInsertWorldspawn();

	Entity* worldspawn = Node_getEntity(worldspawnNode);
	if (worldspawn == NULL) {
		return; // worldspawn not an entity?
	}

	for (GroupNodeList::const_iterator i = walker.getList().begin(); 
		 i != walker.getList().end(); ++i)
	{
		const scene::INodePtr& groupNode = *i;

		Entity* parent = Node_getEntity(groupNode);
	
		if (parent == NULL) continue; // not an entity
		
		// Cycle through all the children and reparent them to the worldspawn node
		ReparentToEntityWalker reparentor(worldspawnNode);
		groupNode->traverse(reparentor);

    	// At this point, all the child primitives have been selected by the walker
    	
    	// Check if the old parent entity node is empty
		if (!groupNode->hasChildNodes()) {
    		// Remove this node from its parent, it's not needed anymore
			scene::removeNodeFromParent(groupNode);
    	}
    	else {
    		globalErrorStream() << "Error while reparenting, cannot delete old parent (not empty)\n"; 
    	}
	}

	// Flag the map as changed
	GlobalMap().setModified(true);	
}

// Some helper methods
bool contains_entity(scene::INodePtr node) {
	return !Node_isBrush(node) && !Node_isPatch(node) && !Node_isEntity(node);
}

bool contains_primitive(scene::INodePtr node) {
	return Node_isEntity(node) && Node_getEntity(node)->isContainer();
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
	const scene::INodePtr _parent;

	mutable std::list<scene::INodePtr> _childrenToReparent;
public:
	ParentSelectedBrushesToEntityWalker(const scene::INodePtr& parent) : 
		_parent(parent)
	{}

	~ParentSelectedBrushesToEntityWalker() {
		for (std::list<scene::INodePtr>::iterator i = _childrenToReparent.begin();
			 i != _childrenToReparent.end(); i++)
		{
			// Remove this path from the old parent
			scene::removeNodeFromParent(*i);

			// Insert the child node into the parent node 
			_parent->addChildNode(*i);
		}

		// Update the scene
		SceneChangeNotify();
	}
	
	void visit(const scene::INodePtr& node) const {
		// Don't reparent instances to themselves
		if (_parent == node) {
			return;
		}

		// The type of the contained items
		ENodeType contains = node_get_contains(_parent);
		// The type of the child
		ENodeType type = node_get_nodetype(node);
		
		if (contains != eNodeUnknown && contains == type) {
			// Got a child, add it to the list
			_childrenToReparent.push_back(node);
		}
		else {
			gtkutil::errorDialog(
				"failed - " + nodetype_get_name(type) + " cannot be parented to " + 
				nodetype_get_name(contains) + " container.\n", 
				GlobalRadiant().getMainWindow()
			);
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
			GlobalSelectionSystem().ultimateSelected()
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

class GroupNodeChildSelector :
	public SelectionSystem::Visitor,
	public scene::NodeVisitor
{
	typedef std::list<scene::INodePtr> NodeList;
	mutable NodeList _groupNodes;

public:
	/**
	 * greebo: The destructor takes care of the actual selection changes. During
	 * selection traversal, the selection itself cannot be changed without
	 * invalidating the SelectionSystem's internal iterators.
	 */
	~GroupNodeChildSelector() {
		for (NodeList::iterator i = _groupNodes.begin(); i != _groupNodes.end(); i++) {
			// De-select the groupnode
			Node_setSelected(*i, false);

			// Select all the child nodes using self as visitor
			(*i)->traverse(*this);
		}
	}

	// SelectionSystem::Visitor implementation
	void visit(const scene::INodePtr& node) const {
		// Don't traverse hidden elements, just to be sure
		if (!node->visible()) {
			return;
		}
		
		// Is this a selected groupnode?
		if (Node_isSelected(node) && 
			Node_getGroupNode(node) != NULL)
		{
			// Marke the groupnode for de-selection
			_groupNodes.push_back(node);
		}
	}
	
	bool pre(const scene::INodePtr& node) {
		// Don't process starting point node or invisible nodes
		if (node->visible()) {
			Node_setSelected(node, true);
		}
		
		return true;
	}
};

void selectChildren() {
	// Traverse the selection and identify the groupnodes
	GlobalSelectionSystem().foreachSelected(
		GroupNodeChildSelector()
	);
}

/**
 * greebo: This walker traverses the entire scenegraph, 
 *         searching for entities with selected child primitives.
 *         If such an entity is found, it is traversed and all
 *         child primitives are selected.
 */
class ExpandSelectionToEntitiesWalker : 
	public scene::Graph::Walker
{
public:
	bool pre(const scene::Path& path, const scene::INodePtr& node) const {
		Entity* entity = Node_getEntity(node);

		if (entity != NULL) {
			// We have an entity, traverse and select children if any child is selected
			return entity->isContainer() && Node_selectedDescendant(node);
		}
		else if (Node_isPrimitive(node)) {
			// We have a primitive, select it
			Node_setSelected(node, true);
			// Don't traverse any deeper
			return false;
		}

		return true;
	}
};

void expandSelectionToEntities() {
	GlobalSceneGraph().traverse(ExpandSelectionToEntitiesWalker());
}

} // namespace algorithm

} // namespace selection
