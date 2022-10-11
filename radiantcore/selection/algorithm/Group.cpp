#include "Group.h"

#include <set>
#include "i18n.h"
#include "igroupnode.h"
#include "ientity.h"
#include "itextstream.h"
#include "iselectiongroup.h"
#include "imap.h"
#include "selectionlib.h"
#include "entitylib.h"
#include "scene/SelectableNode.h"
#include "scene/GroupNodeChecker.h"
#include "command/ExecutionFailure.h"
#include "command/ExecutionNotPossible.h"
#include "selection/algorithm/Entity.h"
#include "scene/Group.h"

namespace selection {

namespace algorithm {

ISelectionGroupManager& getMapSelectionGroupManager()
{
	assert(GlobalMapModule().getRoot());

	return GlobalMapModule().getRoot()->getSelectionGroupManager();
}

void convertSelectedToFuncStatic(const cmd::ArgumentList& args)
{
	UndoableCommand command("convertSelectedToFuncStatic");

	// Attempt to create a func_static entity
	GlobalEntityModule().createEntityFromSelection("func_static", Vector3(0,0,0));
}

void revertGroupToWorldSpawn(const cmd::ArgumentList& args)
{
	UndoableCommand cmd("revertToWorldspawn");

	GroupNodeCollector walker;
	GlobalSelectionSystem().foreachSelected(walker);

	if (walker.getList().empty())
    {
		return; // nothing to do!
	}

	// Deselect all, the children get selected after reparenting
	GlobalSelectionSystem().setSelectedAll(false);

	// Get the worldspawn node
	scene::INodePtr worldspawnNode = GlobalMapModule().findOrInsertWorldspawn();

	Entity* worldspawn = Node_getEntity(worldspawnNode);

	if (!worldspawn)
    {
		return; // worldspawn not an entity?
	}

	for (const scene::INodePtr& groupNode : walker.getList())
	{
		Entity* parent = Node_getEntity(groupNode);

		if (!parent) continue; // not an entity

		ParentPrimitivesToEntityWalker reparentor(worldspawnNode);
		groupNode->traverseChildren(reparentor);

		// Perform the reparenting, this also checks for empty parent nodes
		// being left behind after this operation
		reparentor.reparent();

		// Select the reparented primitives after moving them to worldspawn
		reparentor.selectReparentedPrimitives();
	}
}

void ParentPrimitivesToEntityWalker::reparent()
{
	for (const scene::INodePtr& i : _childrenToReparent)
	{
		// Remove this path from the old parent
		scene::removeNodeFromParent(i);

		// Insert the child node into the parent node
		_parent->addChildNode(i);
	}

	rMessage() << "Reparented " << _childrenToReparent.size()
		<< " primitives." << std::endl;

	// Update parent node/subgraph visibility after reparenting
	scene::UpdateNodeVisibilityWalker updater(_parent->getRootNode()->getLayerManager());

	// Update the new parent too
	_parent->traverse(updater);

	for (const scene::INodePtr& i : _oldParents)
	{
		i->traverse(updater);
	}

	// Now check if any parents were left behind empty
	for (const scene::INodePtr& oldParent : _oldParents)
	{
		if (!scene::hasChildPrimitives(oldParent))
		{
			// Is empty, but make sure we're not removing the worldspawn
			if (Node_isWorldspawn(oldParent)) continue;

			// Is empty now, remove it
			scene::removeNodeFromParent(oldParent);
		}
	}

	// Update the scene
	SceneChangeNotify();
}

void ParentPrimitivesToEntityWalker::selectReparentedPrimitives()
{
	for (const scene::INodePtr& i : _childrenToReparent)
	{
		Node_setSelected(i, true);
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
	if (scene::hasChildPrimitives(node))
	{
		_groupNodes.push_back(node);
	}
}

// re-parents the selected brushes/patches
void parentSelection(const cmd::ArgumentList& args)
{
	// Retrieve the selection information structure
	if (!curSelectionIsSuitableForReparent())
	{
		throw cmd::ExecutionNotPossible(_("Cannot reparent primitives to entity. "
						 "Please select at least one brush/patch and exactly one func_* entity. "
						 "(The entity has to be selected last.)"));
	}

	UndoableCommand undo("parentSelectedPrimitives");

	// Take the last selected item (this is an entity)
	ParentPrimitivesToEntityWalker visitor(
		GlobalSelectionSystem().ultimateSelected()
	);

	GlobalSelectionSystem().foreachSelected(visitor);
	visitor.reparent();
}

void parentSelectionToWorldspawn(const cmd::ArgumentList& args) {
	UndoableCommand undo("parentSelectedPrimitives");

	scene::INodePtr world = GlobalMapModule().findOrInsertWorldspawn();
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
			(*i)->traverseChildren(*this);
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
 * searching for entities with selected child primitives.
 * If such an entity is found, it is traversed and all
 * child primitives are selected.
 */
class ExpandSelectionToSiblingsWalker :
	public scene::NodeVisitor
{
public:
	bool pre(const scene::INodePtr& node) override
	{
		Entity* entity = Node_getEntity(node);

		if (entity != nullptr)
		{
			// We have an entity, traverse and select children if any child is selected
			return entity->isContainer() && (Node_isSelected(node) || Node_hasSelectedChildNodes(node));
		}
		else if (Node_isPrimitive(node))
		{
			// We have a primitive, select it
			Node_setSelected(node, true);
			// Don't traverse any deeper
			return false;
		}

		return true;
	}
};

void expandSelectionToSiblings(const cmd::ArgumentList& args)
{
	ExpandSelectionToSiblingsWalker walker;
	GlobalSceneGraph().root()->traverse(walker);
}

/**
 * greebo: This walker traverses the entire subgraph,
 * searching for entities with selected child primitives.
 * If such an entity is found, it will be selected in place
 * of the child primitive.
 */
class PropagateSelectionToParentEntityWalker :
	public scene::NodeVisitor
{
public:
	bool pre(const scene::INodePtr& node) override
	{
		Entity* entity = Node_getEntity(node);

		if (entity != nullptr)
		{
			if (entity->isContainer() && !entity->isWorldspawn() && Node_hasSelectedChildNodes(node))
			{
				// De-select all child primitives
				node->foreachNode([](const scene::INodePtr& child)->bool
				{
					Node_setSelected(child, false);
					return true;
				});

				// Select the entity instead
				Node_setSelected(node, true);
			}

			return false; // don't traverse entities, that's covered by Node_hasSelectedChildNodes
		}

		return true;
	}
};

void selectParentEntitiesOfSelected(const cmd::ArgumentList& args)
{
	PropagateSelectionToParentEntityWalker walker;
	GlobalSceneGraph().root()->traverse(walker);
}

void mergeSelectedEntities(const cmd::ArgumentList& args)
{
	// Check the current selection, must consist of group nodes only
	scene::GroupNodeChecker walker;
	GlobalSelectionSystem().foreachSelected(walker);

	if (walker.onlyGroupsAreSelected() && walker.selectedGroupCount() > 1)
	{
		UndoableCommand cmd("mergeEntities");

		scene::INodePtr newParent = walker.getFirstSelectedGroupNode();

		// Gather all group nodes in the selection
		GroupNodeCollector collector;
		GlobalSelectionSystem().foreachSelected(collector);

		// Traverse all group nodes using a ParentPrimitivesToEntityWalker
		for (GroupNodeCollector::GroupNodeList::const_iterator i = collector.getList().begin();
			 i != collector.getList().end(); ++i)
		{
			if (*i == newParent) continue;

			ParentPrimitivesToEntityWalker reparentor(newParent);
			(*i)->traverseChildren(reparentor);

			reparentor.reparent();
		}

		rMessage() << collector.getList().size() << " group nodes merged." << std::endl;
	}
	else
	{
		throw cmd::ExecutionNotPossible(_("Cannot merge entities, "
							 "the selection must consist of func_* entities only.\n"
							 "(The first selected entity will be preserved.)"));
	}
}

void deleteAllSelectionGroupsCmd(const cmd::ArgumentList& args)
{
	if (!GlobalMapModule().getRoot())
	{
		rError() << "No map loaded, cannot delete groups." << std::endl;
		return;
	}

	UndoableCommand cmd("DeleteAllSelectionGroups");
	getMapSelectionGroupManager().deleteAllSelectionGroups();
}

void groupSelectedCmd(const cmd::ArgumentList& args)
{
	groupSelected();
}

void ungroupSelectedCmd(const cmd::ArgumentList& args)
{
	ungroupSelected();
}

} // namespace algorithm

} // namespace selection
