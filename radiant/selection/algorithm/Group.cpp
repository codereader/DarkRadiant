#include "Group.h"

#include "i18n.h"
#include <set>
#include "igroupnode.h"
#include "imainframe.h"
#include "itextstream.h"
#include "selectionlib.h"
#include "scenelib.h"
#include "entitylib.h"
#include "map/Map.h"
#include "gtkutil/dialog.h"
#include "../../entity.h"

namespace selection {

namespace algorithm {

void convertSelectedToFuncStatic(const cmd::ArgumentList& args)
{
	UndoableCommand command("convertSelectedToFuncStatic");	

	// Attempt to create a func_static entity
	try
	{
		entity::createEntityFromSelection("func_static", Vector3(0,0,0));	
	}
	catch (EntityCreationException& e)
	{
		gtkutil::errorDialog(e.what(), GlobalMainFrame().getTopLevelWindow());
	}
}

void revertGroupToWorldSpawn(const cmd::ArgumentList& args)
{
	UndoableCommand cmd("revertToWorldspawn");

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

	for (GroupNodeCollector::GroupNodeList::const_iterator i = walker.getList().begin(); 
		 i != walker.getList().end(); ++i)
	{
		const scene::INodePtr& groupNode = *i;

		Entity* parent = Node_getEntity(groupNode);
	
		if (parent == NULL) continue; // not an entity

		ParentPrimitivesToEntityWalker reparentor(worldspawnNode);
		groupNode->traverse(reparentor);

		// Perform the reparenting, this also checks for empty parent nodes
		// being left behind after this operation
		reparentor.reparent();

		// Select the reparented primitives after moving them to worldspawn
		reparentor.selectReparentedPrimitives();
	}

	// Flag the map as changed
	GlobalMap().setModified(true);	
}

void ParentPrimitivesToEntityWalker::reparent()
{
	for (std::list<scene::INodePtr>::iterator i = _childrenToReparent.begin();
		 i != _childrenToReparent.end(); i++)
	{
		// Remove this path from the old parent
		scene::removeNodeFromParent(*i);

		// Insert the child node into the parent node 
		_parent->addChildNode(*i);
	}

	globalOutputStream() << "Reparented " << _childrenToReparent.size() 
		<< " primitives." << std::endl;

	// Update parent node/subgraph visibility after reparenting
	scene::UpdateNodeVisibilityWalker updater;

	// Update the new parent too
	Node_traverseSubgraph(_parent, updater);

	for (std::set<scene::INodePtr>::iterator i = _oldParents.begin();
		 i != _oldParents.end(); i++)
	{
		Node_traverseSubgraph(*i, updater);
	}

	// Now check if any parents were left behind empty
	for (std::set<scene::INodePtr>::iterator i = _oldParents.begin();
		 i != _oldParents.end(); i++)
	{
		if (!(*i)->hasChildNodes())
		{
			// Is empty, but make sure we're not removing the worldspawn
			if (node_is_worldspawn(*i)) continue;

			// Is empty now, remove it
			scene::removeNodeFromParent(*i);
		}
	}

	// Update the scene
	SceneChangeNotify();
}

void ParentPrimitivesToEntityWalker::selectReparentedPrimitives()
{
	for (std::list<scene::INodePtr>::iterator i = _childrenToReparent.begin();
		 i != _childrenToReparent.end(); i++)
	{
		Node_setSelected(*i, true);
	}
}
	
void ParentPrimitivesToEntityWalker::visit(const scene::INodePtr& node) const
{
	// Don't reparent instances to themselves
	if (_parent == node) return;

	if (Node_isPrimitive(node))
	{
		// Got a child, add it to the list
		_childrenToReparent.push_back(node);

		// Mark the entity for later emptiness check
		_oldParents.insert(node->getParent());
	}
}

bool ParentPrimitivesToEntityWalker::pre(const scene::INodePtr& node)
{
	// Don't reparent nodes to themselves
	if (_parent != node && Node_isPrimitive(node))
	{
		// Got a child, add it to the list
		_childrenToReparent.push_back(node);

		// Mark the entity for later emptiness check
		_oldParents.insert(node->getParent());

		return false; // don't traverse primitives any further
	}

	return true;
}

void GroupNodeCollector::visit(const scene::INodePtr& node) const
{
	if (node_is_group(node))
	{
		_groupNodes.push_back(node);
	}
}

GroupNodeChecker::GroupNodeChecker() :
	_onlyGroups(true),
	_numGroups(0)
{}

void GroupNodeChecker::visit(const scene::INodePtr& node) const
{
	if (!node_is_group(node))
	{
		_onlyGroups = false;
	}
	else
	{
		_numGroups++;

		if (_firstGroupNode == NULL)
		{
			_firstGroupNode = node;
		}
	}
}

bool GroupNodeChecker::onlyGroupsAreSelected() const
{
	return _numGroups > 0 && _onlyGroups;
}

std::size_t GroupNodeChecker::selectedGroupCount() const
{
	return _numGroups;
}

scene::INodePtr GroupNodeChecker::getFirstSelectedGroupNode() const
{
	return _firstGroupNode;
}

// re-parents the selected brushes/patches
void parentSelection(const cmd::ArgumentList& args)
{
	try
	{
		// Retrieve the selection information structure
		const SelectionInfo& info = GlobalSelectionSystem().getSelectionInfo();

		if (info.totalCount <= 1 || info.entityCount != 1)
		{
			throw std::runtime_error("Current selection not suitable.");
		}

		Entity* entity = Node_getEntity(GlobalSelectionSystem().ultimateSelected());

		if (entity == NULL)
		{
			throw std::runtime_error("Last selected node is not an entity.");
		}

		UndoableCommand undo("parentSelectedPrimitives");
		
		// Take the last selected item (this is an entity)
		ParentPrimitivesToEntityWalker visitor(
			GlobalSelectionSystem().ultimateSelected()
		);

		GlobalSelectionSystem().foreachSelected(visitor);
		visitor.reparent();
	}
	catch (std::runtime_error& e)
	{
		gtkutil::errorDialog(_("Cannot reparent primitives to entity. "
							 "Please select at least one brush/patch and exactly one entity."
							 "(The entity has to be selected last.)"), 
							 GlobalMainFrame().getTopLevelWindow());

		globalErrorStream() << "Cannot reparent primitives: " << e.what() << std::endl;
	}
}

void parentSelectionToWorldspawn(const cmd::ArgumentList& args) {
	UndoableCommand undo("parentSelectedPrimitives");

	scene::INodePtr world = GlobalMap().findOrInsertWorldspawn();
	if (world == NULL) return;

	// Take the last selected item (this should be an entity)
	ParentPrimitivesToEntityWalker visitor(world);
	GlobalSelectionSystem().foreachSelected(visitor);
	visitor.reparent();
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

void selectChildren(const cmd::ArgumentList& args) {
	// Traverse the selection and identify the groupnodes
	GlobalSelectionSystem().foreachSelected(
		GroupNodeChildSelector()
	);
}

/**
 * greebo: This walker traverses the entire subgraph, 
 *         searching for entities with selected child primitives.
 *         If such an entity is found, it is traversed and all
 *         child primitives are selected.
 */
class ExpandSelectionToEntitiesWalker : 
	public scene::NodeVisitor
{
public:
	bool pre(const scene::INodePtr& node) {
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

void expandSelectionToEntities(const cmd::ArgumentList& args) {
	ExpandSelectionToEntitiesWalker walker;
	Node_traverseSubgraph(GlobalSceneGraph().root(), walker);
}

void mergeSelectedEntities(const cmd::ArgumentList& args)
{
	// Check the current selection, must consist of group nodes only
	GroupNodeChecker walker;
	GlobalSelectionSystem().foreachSelected(walker);

	if (walker.onlyGroupsAreSelected() && walker.selectedGroupCount() > 1)
	{
		UndoableCommand cmd("mergeEntities");

		scene::INodePtr newParent = walker.getFirstSelectedGroupNode();

		// Gather all group nodes in the selection
		GroupNodeCollector walker;
		GlobalSelectionSystem().foreachSelected(walker);

		// Traverse all group nodes using a ParentPrimitivesToEntityWalker
		for (GroupNodeCollector::GroupNodeList::const_iterator i = walker.getList().begin(); 
			 i != walker.getList().end(); ++i)
		{
			if (*i == newParent) continue;

			ParentPrimitivesToEntityWalker reparentor(newParent);
			(*i)->traverse(reparentor);

			reparentor.reparent();
		}

		globalOutputStream() << walker.getList().size() << " group nodes merged." << std::endl;
	}
	else
	{
		gtkutil::errorDialog(_("Cannot merge entities, "
							 "the selection must consist of func_* entities only.\n"
							 "(The first selected entity will be preserved.)"), 
							 GlobalMainFrame().getTopLevelWindow());
	}
}

} // namespace algorithm

} // namespace selection
