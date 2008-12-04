#include "General.h"

#include <stack>
#include "iselection.h"
#include "iundo.h"
#include "scenelib.h"
#include "selectable.h"

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

bool EntitySelectByClassnameWalker::pre(
	const scene::Path& path, const scene::INodePtr& node) const
{
	Entity* entity = Node_getEntity(node);
	
	if (entity != NULL && entityMatches(entity)) {
		Node_setSelected(node, true);
	}

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

void selectAllOfType() {
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
			GlobalSceneGraph().traverse(classnameSelector);
		}
		else {
			Scene_BrushSelectByShader(GlobalSceneGraph(), GlobalTextureBrowser().getSelectedShader());
			Scene_PatchSelectByShader(GlobalSceneGraph(), GlobalTextureBrowser().getSelectedShader());
		}
	}

	SceneChangeNotify();
}

inline void hideNode(scene::INodePtr node, bool hide) {
	if (hide) {
		node->enable(scene::Node::eHidden);
	}
	else {
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

	void visit(const scene::INodePtr& node) const {
		hideNode(node, _hide);
	}
};

void hideSelected() {
	// Traverse the selection, hiding all nodes
	GlobalSelectionSystem().foreachSelected(HideSelectedWalker(true));

	// Then de-select the hidden nodes
	GlobalSelectionSystem().setSelectedAll(false);

	SceneChangeNotify();
}

// Hides all nodes that are not selected
class HideDeselectedWalker : 
	public scene::Graph::Walker
{
	bool _hide;

	mutable std::stack<bool> _stack;
public:
	HideDeselectedWalker(bool hide) : 
		_hide(hide)
	{}

	bool pre(const scene::Path& path, const scene::INodePtr& node) const {
		// Check the selection status
		bool isSelected = Node_isSelected(node);

		// greebo: Don't check root nodes for selected state
		if (!node->isRoot() && isSelected) {
			// We have a selected instance, "remember" this by setting the parent 
			// stack element to TRUE
			if (_stack.size() > 0) {
				_stack.top() = true;
			}
		}

		// We are going one level deeper, add a new stack element for this subtree
		_stack.push(false);

		// Try to go deeper, but don't do this for deselected instances
		return !isSelected;
	}

	virtual void post(const scene::Path& path, const scene::INodePtr& node) const {
		
		// greebo: We've traversed this subtree, now check if we had selected children
		if (!node->isRoot() && 
			_stack.size() > 0 && _stack.top() == false && 
			!Node_isSelected(node))
		{
			// No selected child instances, hide this node
			hideNode(node, _hide);
		}

		// Go upwards again, one level
		_stack.pop();
	}
};

void hideDeselected() {
	GlobalSceneGraph().traverse(HideDeselectedWalker(true));
	// Hide all components, there might be faces selected
	GlobalSelectionSystem().setSelectedAllComponents(false);
	SceneChangeNotify();
}

class HideAllWalker : 
	public scene::Graph::Walker
{
	bool _hide;
public:
	HideAllWalker(bool hide) : 
		_hide(hide)
	{}

	bool pre(const scene::Path& path, const scene::INodePtr& node) const {
		hideNode(node, _hide);
		return true;
	}
};

void showAllHidden() {
	GlobalSceneGraph().traverse(HideAllWalker(false));
	SceneChangeNotify();
}

class InvertSelectionWalker : 
	public scene::Graph::Walker
{
	SelectionSystem::EMode _mode;
	mutable SelectablePtr _selectable;
public:
	InvertSelectionWalker(SelectionSystem::EMode mode) : 
		_mode(mode)
	{}
	
	bool pre(const scene::Path& path, const scene::INodePtr& node) const {
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
	
	void post(const scene::Path& path, const scene::INodePtr& node) const {
		if (_selectable != NULL) {
			_selectable->invertSelected();
			_selectable = SelectablePtr();
		}
	}
};

void invertSelection() {
	GlobalSceneGraph().traverse(
		InvertSelectionWalker(GlobalSelectionSystem().Mode())
	);
}

class DeleteSelected : 
	public scene::Graph::Walker
{
	mutable bool _remove;
	mutable bool _removedChild;

	mutable std::set<scene::INodePtr> _eraseList;
public:
	DeleteSelected() : 
		_remove(false), 
		_removedChild(false)
	{}

	// Actually destroy the nodes
	~DeleteSelected() {
		for (std::set<scene::INodePtr>::iterator i = _eraseList.begin(); i != _eraseList.end(); i++) {
			scene::removeNodeFromParent(*i);
		}
	}

	bool pre(const scene::Path& path, const scene::INodePtr& node) const {
		_removedChild = false;

		if (Node_isSelected(node) && path.size() > 1 && !node->isRoot()) {
			_remove = true;
			return false;// dont traverse into child elements of deletion candidates
		}

		return true;
	}

	void post(const scene::Path& path, const scene::INodePtr& node) const {
		if (_removedChild) {
			// A child node has been removed, check if we have an empty entity
			_removedChild = false;

			// Delete empty entities, but leave the worldspawn alone
			Entity* entity = Node_getEntity(node);

			if (entity != NULL && 
				node != GlobalMap().findWorldspawn() && 
				!node->hasChildNodes())
			{
				_eraseList.insert(node);
			}
		}

		// node should be removed
		if (_remove) {
			if (Node_isEntity(path.parent())) {
				// The parent node is an entity, set the bool to indicate
				// the child removal. The entity is then checked for emptiness.
				_removedChild = true;
			}

			_remove = false;
			_eraseList.insert(node);
		}
	}
};

void deleteSelection() {
	// Traverse the scene, deleting all selected nodes
	GlobalSceneGraph().traverse(DeleteSelected());

	SceneChangeNotify();
}

void deleteSelectionCmd() {
	UndoableCommand undo("deleteSelected");

	deleteSelection();

	GlobalShaderClipboard().clear();
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
	public scene::Graph::Walker
{
	AABB* _aabbs;				// selection aabbs
	std::size_t _count;			// number of aabbs in _aabbs
	TSelectionPolicy policy;	// type that contains a custom intersection method aabb<->aabb

public:
	SelectByBounds(AABB* aabbs, std::size_t count) : 
		_aabbs(aabbs),
        _count(count)
	{}

	bool pre(const scene::Path& path, const scene::INodePtr& node) const {
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
    
		if (path.size() > 1 && !node->isRoot() && selectable != NULL) {
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
		GlobalSceneGraph().traverse(
			SelectByBounds<TSelectionPolicy>(aabbs, count)
		);
      
		SceneChangeNotify();
		delete[] aabbs;
	}
};

void selectInside() {
	SelectByBounds<SelectionPolicy_Inside>::DoSelection();
}

void selectTouching() {
	SelectByBounds<SelectionPolicy_Touching>::DoSelection(false);
}

void selectCompleteTall() {
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
