#include "General.h"

#include <stack>
#include "iselection.h"
#include "iundo.h"
#include "scenelib.h"
#include "iselectable.h"

#include "brushmanip.h"
#include "patchmanip.h"
#include "map/Map.h"
#include "selection/shaderclipboard/ShaderClipboard.h"
#include "ui/texturebrowser/TextureBrowser.h"

#include "SelectionPolicies.h"
#include "selection/SceneWalkers.h"
#include "select.h"

namespace selection {
	namespace algorithm {

EntitySelectByClassnameWalker::EntitySelectByClassnameWalker(const ClassnameList& classnames) :
	_classnames(classnames)
{}

bool EntitySelectByClassnameWalker::pre(const scene::INodePtr& node) {
	// don't traverse invisible nodes
	if (!node->visible()) return false; 

	Entity* entity = Node_getEntity(node);
	
	if (entity != NULL) {

		if (entityMatches(entity)) {
			// Got a matching entity
			Node_setSelected(node, true);
		}

		// Don't traverse entities
		return false;
	}

	// Not an entity, traverse
	return true;
}

bool EntitySelectByClassnameWalker::entityMatches(Entity* entity) const {
	for (ClassnameList::const_iterator i = _classnames.begin(); 
		 i != _classnames.end(); ++i)
	{
		if (entity->getKeyValue("classname") == *i) {
			return true;
		}
	}

	return false;
}

/**
 * greebo: Traverse the scene and collect all classnames of 
 *         selected entities.
 */
class EntityGetSelectedClassnamesWalker : 
	public SelectionSystem::Visitor
{
	mutable ClassnameList _classnames;

public:
	const ClassnameList& getClassnameList() const {
		return _classnames;
	}

	virtual void visit(const scene::INodePtr& node) const {
		Entity* entity = Node_getEntity(node);

		if (entity != NULL) {
			_classnames.push_back(entity->getKeyValue("classname"));
		}
	}
};

void selectAllOfType(const cmd::ArgumentList& args) {
	if (GlobalSelectionSystem().Mode() == SelectionSystem::eComponent) {
		if (GlobalSelectionSystem().ComponentMode() == SelectionSystem::eFace) {
			// Deselect all faces
			GlobalSelectionSystem().setSelectedAllComponents(false);
			// Select all faces carrying the shader selected in the Texture Browser
			Scene_BrushSelectByShader_Component(GlobalSceneGraph(), GlobalTextureBrowser().getSelectedShader());
		}
	}
	else {
		// Find any classnames of selected entities
		EntityGetSelectedClassnamesWalker classnameFinder;
		GlobalSelectionSystem().foreachSelected(classnameFinder);

		// De-select everything
		GlobalSelectionSystem().setSelectedAll(false);

		if (!classnameFinder.getClassnameList().empty()) {
			// Instantiate a selector class
			EntitySelectByClassnameWalker classnameSelector(
				classnameFinder.getClassnameList()
			);

			// Traverse the scenegraph, select all matching the classname list
			Node_traverseSubgraph(GlobalSceneGraph().root(), classnameSelector);
		}
		else {
			Scene_BrushSelectByShader(GlobalSceneGraph(), GlobalTextureBrowser().getSelectedShader());
			Scene_PatchSelectByShader(GlobalSceneGraph(), GlobalTextureBrowser().getSelectedShader());
		}
	}

	SceneChangeNotify();
}

class HideSubgraphWalker : 
	public scene::NodeVisitor
{
public:
	bool pre(const scene::INodePtr& node)
	{
		node->enable(scene::Node::eHidden);
		return true;
	}
};

class ShowSubgraphWalker : 
	public scene::NodeVisitor
{
public:
	bool pre(const scene::INodePtr& node)
	{
		node->disable(scene::Node::eHidden);
		return true;
	}
};

inline void hideSubgraph(const scene::INodePtr& node, bool hide)
{
	if (hide)
	{
		HideSubgraphWalker walker;
		Node_traverseSubgraph(node, walker);
	}
	else
	{
		ShowSubgraphWalker walker;
		Node_traverseSubgraph(node, walker);
	}
}

inline void hideNode(const scene::INodePtr& node, bool hide)
{
	if (hide)
	{
		node->enable(scene::Node::eHidden);
	}
	else
	{
		node->disable(scene::Node::eHidden);
	}
}

// This walker hides all selected nodes
class HideSelectedWalker : 
	public SelectionSystem::Visitor
{
	bool _hide;
public:
	HideSelectedWalker(bool hide) : 
		_hide(hide)
	{}

	void visit(const scene::INodePtr& node) const
	{
		hideSubgraph(node, _hide);
	}
};

void hideSelected(const cmd::ArgumentList& args) {
	// Traverse the selection, hiding all nodes
	GlobalSelectionSystem().foreachSelected(HideSelectedWalker(true));

	// Then de-select the hidden nodes
	GlobalSelectionSystem().setSelectedAll(false);

	SceneChangeNotify();
}

// Hides all nodes that are not selected
class HideDeselectedWalker : 
	public scene::NodeVisitor
{
	bool _hide;

	std::stack<bool> _stack;
public:
	HideDeselectedWalker(bool hide) : 
		_hide(hide)
	{}

	bool pre(const scene::INodePtr& node)
	{
		// Check the selection status
		bool isSelected = Node_isSelected(node);

		// greebo: Don't check root nodes for selected state
		if (!node->isRoot() && isSelected)
		{
			// We have a selected instance, "remember" this by setting the parent 
			// stack element to TRUE
			if (!_stack.empty())
			{
				_stack.top() = true;
			}
		}

		// We are going one level deeper, add a new stack element for this subtree
		_stack.push(false);

		// Try to go deeper, but don't do this for deselected instances
		return !isSelected;
	}

	void post(const scene::INodePtr& node)
	{
		// greebo: We've traversed this subtree, now check if we had selected children
		if (!node->isRoot() && 
			!_stack.empty() && _stack.top() == false && 
			!Node_isSelected(node))
		{
			// No selected child nodes, hide this node
			hideSubgraph(node, _hide);
		}

		// Go upwards again, one level
		_stack.pop();
	}
};

void hideDeselected(const cmd::ArgumentList& args) {
	HideDeselectedWalker walker(true);
	Node_traverseSubgraph(GlobalSceneGraph().root(), walker);

	// Hide all components, there might be faces selected
	GlobalSelectionSystem().setSelectedAllComponents(false);

	SceneChangeNotify();
}

class HideAllWalker : 
	public scene::NodeVisitor
{
	bool _hide;
public:
	HideAllWalker(bool hide) : 
		_hide(hide)
	{}

	bool pre(const scene::INodePtr& node) {
		hideNode(node, _hide);
		return true;
	}
};

void showAllHidden(const cmd::ArgumentList& args) {
	HideAllWalker walker(false);
	Node_traverseSubgraph(GlobalSceneGraph().root(), walker);
	SceneChangeNotify();
}

class InvertSelectionWalker : 
	public scene::NodeVisitor
{
	SelectionSystem::EMode _mode;
	SelectablePtr _selectable;
public:
	InvertSelectionWalker(SelectionSystem::EMode mode) : 
		_mode(mode)
	{}
	
	bool pre(const scene::INodePtr& node) {
		// Check if we have a selectable
		SelectablePtr selectable = Node_getSelectable(node);

		if (selectable) {
			switch (_mode) {
				case SelectionSystem::eEntity:
					if (Node_isEntity(node) != 0) {
						_selectable = node->visible() ? selectable : SelectablePtr();
					}
					break;
				case SelectionSystem::ePrimitive:
					_selectable = node->visible() ? selectable : SelectablePtr();
					break;
				case SelectionSystem::eComponent:
					// Check if we have a componentselectiontestable instance
					ComponentSelectionTestablePtr compSelTestable = 
						Node_getComponentSelectionTestable(node);

					// Only add it to the list if the instance has components and is already selected
					if (compSelTestable != NULL && selectable->isSelected()) {
						_selectable = node->visible() ? selectable : SelectablePtr();
					}
					break;
			}
		}
		
		// Do we have a groupnode? If yes, don't traverse the children
		Entity* entity = Node_getEntity(node);

		if (entity != NULL && node_is_group(node) && 
			entity->getKeyValue("classname") != "worldspawn") 
		{
			// Don't traverse the children of this groupnode
			return false;
		}

		return true;
	}
	
	void post(const scene::INodePtr& node) {
		if (_selectable != NULL) {
			_selectable->invertSelected();
			_selectable = SelectablePtr();
		}
	}
};

void invertSelection(const cmd::ArgumentList& args) {
	InvertSelectionWalker walker(GlobalSelectionSystem().Mode());
	Node_traverseSubgraph(GlobalSceneGraph().root(), walker);
}

/**
 * \brief
 * SelectionSystem::Visitor which adds all selected nodes to a list, so that
 * they can subsequently be deleted from the scenegraph.
 */
class DeleteSelectedVisitor :
	public SelectionSystem::Visitor
{
	mutable std::set<scene::INodePtr> _eraseList;

public:

    /**
     * \brief
     * Perform the actual deletion.
     */
    void performDeletion()
    {
        for (std::set<scene::INodePtr>::iterator i = _eraseList.begin();
             i != _eraseList.end();
             ++i) 
        {
			scene::INodePtr parent = (*i)->getParent();

			// Remove the childnodes
			scene::removeNodeFromParent(*i);

			if (!parent->hasChildNodes()) {
				// Remove the parent as well
				scene::removeNodeFromParent(parent);
			}
		}
	}

    /* SelectionSystem::Visitor implementation */
	void visit(const scene::INodePtr& node) const 
    {
		// Check for selected nodes whose parent is not NULL and are not root
		if (node->getParent() != NULL && !node->isRoot()) 
        {
			// Found a candidate
			_eraseList.insert(node);
		}
	}
};

void deleteSelection() 
{
	// Traverse the scene, deleting all selected nodes
	DeleteSelectedVisitor walker;
	GlobalSelectionSystem().foreachSelected(walker);
    walker.performDeletion();
	
	SceneChangeNotify();
}

void deleteSelectionCmd(const cmd::ArgumentList& args) {
	UndoableCommand undo("deleteSelected");

	deleteSelection();
}

/**
  Loops over all selected brushes and stores their
  world AABBs in the specified array.
*/
class CollectSelectedBrushesBounds : 
	public SelectionSystem::Visitor
{
	AABB* _bounds;				// array of AABBs
	std::size_t _max;			// max AABB-elements in array
	mutable std::size_t _count;// count of valid AABBs stored in array

public:
	CollectSelectedBrushesBounds(AABB* bounds, std::size_t max) : 
		_bounds(bounds),
		_max(max),
		_count(0)
	{}

	std::size_t getCount() const {
		return _count;
	}

	void visit(const scene::INodePtr& node) const {
		ASSERT_MESSAGE(_count <= _max, "Invalid _count in CollectSelectedBrushesBounds");

		// stop if the array is already full
		if (_count == _max) {
			return;
		}

		if (Node_isSelected(node) && Node_isBrush(node)) {
			_bounds[_count] = node->worldAABB();
			++_count;
		}
	}
};

/**
 * Selects all objects that intersect one of the bounding AABBs.
 * The exact intersection-method is specified through TSelectionPolicy,
 * which must implement an evalute() method taking an AABB and the scene::INodePtr.
 */
template<class TSelectionPolicy>
class SelectByBounds : 
	public scene::NodeVisitor
{
	AABB* _aabbs;				// selection aabbs
	std::size_t _count;			// number of aabbs in _aabbs
	TSelectionPolicy policy;	// type that contains a custom intersection method aabb<->aabb

public:
	SelectByBounds(AABB* aabbs, std::size_t count) : 
		_aabbs(aabbs),
        _count(count)
	{}

	bool pre(const scene::INodePtr& node) {
		// Don't traverse hidden nodes
		if (!node->visible()) {
			return false;
		}
		
		SelectablePtr selectable = Node_getSelectable(node);

		// ignore worldspawn
		Entity* entity = Node_getEntity(node);
		if (entity != NULL) {
			if (entity->getKeyValue("classname") == "worldspawn") {
				return true;
			}
		}
    
    	bool selected = false;
    
		if (selectable != NULL && node->getParent() != NULL && !node->isRoot()) {
			for (std::size_t i = 0; i < _count; ++i) {
				// Check if the selectable passes the AABB test
				if (policy.evaluate(_aabbs[i], node)) {
					selectable->setSelected(true);
					selected = true;
					break;
				}
			}
		}
	
		// Only traverse the children of this node, if the node itself couldn't be selected
		return !selected;
	}

	/**
	 * Performs selection operation on the global scenegraph.
	 * If delete_bounds_src is true, then the objects which were
	 * used as source for the selection aabbs will be deleted.
	 */
	static void DoSelection(bool deleteBoundsSrc = true) {
		if (GlobalSelectionSystem().Mode() != SelectionSystem::ePrimitive) {
			// Wrong selection mode
			return;
		}

		// we may not need all AABBs since not all selected objects have to be brushes
		const std::size_t max = GlobalSelectionSystem().countSelected();
		AABB* aabbs = new AABB[max];
            
		CollectSelectedBrushesBounds collector(aabbs, max);
		GlobalSelectionSystem().foreachSelected(collector);

		std::size_t count = collector.getCount();

		// nothing usable in selection
		if (!count) {
			delete[] aabbs;
			return;
		}
      
		// delete selected objects?
		if (deleteBoundsSrc) {
			UndoableCommand undo("deleteSelected");
			selection::algorithm::deleteSelection();
		}

		// Instantiate a "self" object SelectByBounds and use it as visitor
		SelectByBounds<TSelectionPolicy> walker(aabbs, count);
		Node_traverseSubgraph(GlobalSceneGraph().root(), walker);
		
		SceneChangeNotify();
		delete[] aabbs;
	}
};

void selectInside(const cmd::ArgumentList& args) {
	SelectByBounds<SelectionPolicy_Inside>::DoSelection();
}

void selectTouching(const cmd::ArgumentList& args) {
	SelectByBounds<SelectionPolicy_Touching>::DoSelection(false);
}

void selectCompleteTall(const cmd::ArgumentList& args) {
	SelectByBounds<SelectionPolicy_Complete_Tall>::DoSelection();
}

Vector3 getCurrentSelectionCenter() {
	// Construct a walker to traverse the selection
	BoundsAccumulator walker;
	GlobalSelectionSystem().foreachSelected(walker);

	return vector3_snapped(walker.getBounds().getOrigin());
}

AABB getCurrentSelectionBounds() {
	// Construct a walker to traverse the selection
	BoundsAccumulator walker;
	GlobalSelectionSystem().foreachSelected(walker);

	return walker.getBounds();
}

	} // namespace algorithm
} // namespace selection
