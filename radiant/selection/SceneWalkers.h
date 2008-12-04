#ifndef SCENEWALKERS_H_
#define SCENEWALKERS_H_

#include "ientity.h"
#include "ieclass.h"
#include "scenelib.h"
#include "selectable.h"
#include "editable.h"
#include "brush/BrushNode.h"

// -------------- Helper functions -------------------------------------

inline AABB Node_getPivotBounds(const scene::INodePtr& node) {
	Entity* entity = Node_getEntity(node);
	if (entity != NULL && (entity->getEntityClass()->isFixedSize() || !node_is_group(node)))
	{
		EditablePtr editable = Node_getEditable(node);
		if (editable != NULL) {
			return AABB(matrix4_multiplied_by_matrix4(node->localToWorld(), editable->getLocalPivot()).t().getVector3(), Vector3(0, 0, 0));
		}
		else {
			return AABB(node->localToWorld().t().getVector3(), Vector3(0, 0, 0));
		}
	}

	return node->worldAABB();
}

// ----------- The Walker Classes ------------------------------------------------

// Sets the visited instance to <select> (true or false), this is used to select all instances in the graph
class SelectAllWalker : public scene::Graph::Walker {
	bool _select;
public:
	SelectAllWalker(bool select) : _select(select) {}
  
	bool pre(const scene::Path& path, const scene::INodePtr& node) const {
		Node_setSelected(node, _select);
		return true;
	}
};

// Selects the visited component instances in the graph, according to the current component mode
class SelectAllComponentWalker : public scene::Graph::Walker {
	bool _select;
	SelectionSystem::EComponentMode _mode;
public:
	SelectAllComponentWalker(bool select, SelectionSystem::EComponentMode mode)
		: _select(select), _mode(mode) {}

  	bool pre(const scene::Path& path, const scene::INodePtr& node) const {
		ComponentSelectionTestablePtr componentSelectionTestable = Node_getComponentSelectionTestable(node);

		if (componentSelectionTestable != NULL) {
			componentSelectionTestable->setSelectedComponents(_select, _mode);
		}
		return true;
	}
};

// Traverses through the scenegraph and removes degenerated brushes from the selected.
// greebo: The actual erasure is performed in the destructor to keep the scenegraph intact during traversal.
class RemoveDegenerateBrushWalker : 
	public SelectionSystem::Visitor
{
	mutable std::list<scene::INodePtr> _eraseList;
public:
	// Destructor removes marked paths
	~RemoveDegenerateBrushWalker() {
		for (std::list<scene::INodePtr>::iterator i = _eraseList.begin(); i != _eraseList.end(); ++i) {
			// Check if the parent has any children left at all
			scene::INodePtr parent = (*i)->getParent();

			// Remove the node from the scene
			scene::removeNodeFromParent(*i);

			if (parent != NULL && !parent->hasChildNodes()) {
				globalErrorStream() << "Warning: removing empty parent entity." << std::endl;
				scene::removeNodeFromParent(parent);
			}
		}
	}

	void visit(const scene::INodePtr& node) const {
		TransformNodePtr transformNode = Node_getTransformNode(node);

		if (transformNode == NULL) return;
		
		Brush* brush = Node_getBrush(node);

		if (brush != NULL && !brush->hasContributingFaces()) {
			// greebo: Mark this path for removal
			_eraseList.push_back(node);

			globalErrorStream() << "Warning: removed degenerate brush!\n";
			return;
		}
	}
};

// As the name states, all visited instances have their transformations freezed
class FreezeTransforms : public scene::Graph::Walker {
public:
	bool pre(const scene::Path& path, const scene::INodePtr& node) const {
		TransformNodePtr transformNode = Node_getTransformNode(node);
		if (transformNode != 0) {
			TransformablePtr transform = Node_getTransformable(node);
			if (transform != 0) {
				transform->freezeTransform(); 
			}
		}
		return true;
	}
};

// As the name states, all visited instances have their transformations reverted
class RevertTransforms : 
	public scene::Graph::Walker 
{
public:
	bool pre(const scene::Path& path, const scene::INodePtr& node) const {
		TransformNodePtr transformNode = Node_getTransformNode(node);
		if (transformNode != 0) {
			TransformablePtr transform = Node_getTransformable(node);
			if (transform != 0) {
				transform->revertTransform(); 
			}
		}
		return true;
	}
};

// As the name states, all visited SELECTED instances have their transformations reverted
class RevertTransformForSelected : 
	public scene::Graph::Walker 
{
public:
	bool pre(const scene::Path& path, const scene::INodePtr& node) const {
		TransformNodePtr transformNode = Node_getTransformNode(node);
		SelectablePtr selectable = Node_getSelectable(node);
				
		if (transformNode != NULL && selectable != NULL && selectable->isSelected()) {
			TransformablePtr transform = Node_getTransformable(node);
			if (transform != NULL) {
				transform->revertTransform(); 
			}
		}
		return true;
	}
};

/**
 * greebo: Calculates the axis-aligned bounding box of the current selection.
 * Use this walker to traverse the current selection and use the getBounds() 
 * method to retrieve the calculated bounds.
 */ 
class BoundsAccumulator : 
	public SelectionSystem::Visitor
{
	mutable AABB _bounds;
public:
	const AABB& getBounds() const {
		return _bounds;
	}

	void visit(const scene::INodePtr& node) const {
		_bounds.includeAABB(Node_getPivotBounds(node));
	}
};

// greebo: Calculates the axis-aligned bounding box of the selection components.
// The constructor is called with a reference to an AABB variable that is updated during the walk
class BoundsSelectedComponent : public scene::Graph::Walker {
	AABB& _bounds;
public:
	BoundsSelectedComponent(AABB& bounds): _bounds(bounds) {
		_bounds = AABB();
	}
  
	bool pre(const scene::Path& path, const scene::INodePtr& node) const {
		SelectablePtr selectable = Node_getSelectable(node);
		// Only update the aabb variable if the instance is selected
		if (selectable != 0 && selectable->isSelected()) {
			ComponentEditablePtr componentEditable = Node_getComponentEditable(node);
			if (componentEditable != NULL) {
				_bounds.includeAABB(
					aabb_for_oriented_aabb_safe(componentEditable->getSelectedComponentsBounds(), 
												node->localToWorld()));
			}
		}
		return true;
	}
};

#endif /*SCENEWALKERS_H_*/
