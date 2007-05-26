#ifndef SCENEWALKERS_H_
#define SCENEWALKERS_H_

#include "ientity.h"
#include "ieclass.h"
#include "scenelib.h"
#include "selectable.h"
#include "editable.h"
#include "brush/BrushNode.h"

// -------------- Helper functions -------------------------------------

inline AABB Instance_getPivotBounds(scene::Instance& instance) {
	Entity* entity = Node_getEntity(instance.path().top());
	if (entity != 0
		&& (entity->getEntityClass()->isFixedSize() 
			|| !node_is_group(instance.path().top())))
	{
		EditablePtr editable = Node_getEditable(instance.path().top());
		if (editable != NULL) {
			return AABB(matrix4_multiplied_by_matrix4(instance.localToWorld(), editable->getLocalPivot()).t().getVector3(), Vector3(0, 0, 0));
		}
		else {
			return AABB(instance.localToWorld().t().getVector3(), Vector3(0, 0, 0));
		}
	}

	return instance.worldAABB();
}

// ----------- The Walker Classes ------------------------------------------------

// Sets the visited instance to <select> (true or false), this is used to select all instances in the graph
class SelectAllWalker : public scene::Graph::Walker {
	bool _select;
public:
	SelectAllWalker(bool select) : _select(select) {}
  
	bool pre(const scene::Path& path, scene::Instance& instance) const {
		Selectable* selectable = Instance_getSelectable(instance);
		if (selectable != 0) {
			selectable->setSelected(_select);
		}
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

  	bool pre(const scene::Path& path, scene::Instance& instance) const {
		ComponentSelectionTestable* componentSelectionTestable = Instance_getComponentSelectionTestable(instance);
		if (componentSelectionTestable) {
			componentSelectionTestable->setSelectedComponents(_select, _mode);
		}
		return true;
	}
};

// Traverses through the scenegraph and removes degenerated brushes from the selected.
class RemoveDegenerateBrushWalker : 
	public scene::Graph::Walker 
{
public:
	bool pre(const scene::Path& path, scene::Instance& instance) const {
		TransformNodePtr transformNode = Node_getTransformNode(path.top());
		if (transformNode != 0) {
			Brush* brush = Node_getBrush(path.top());
			if (brush != NULL) {
				if (!brush->hasContributingFaces()) {
					// Remove the degenerate brush
					Path_deleteTop(path);
					globalErrorStream() << "Warning: removed degenerate brush!\n";
					return false;
				}
			}
		}
		return true;
	}
};

// As the name states, all visited instances have their transformations freezed
class FreezeTransforms : public scene::Graph::Walker {
public:
	bool pre(const scene::Path& path, scene::Instance& instance) const {
		TransformNodePtr transformNode = Node_getTransformNode(path.top());
		if (transformNode != 0) {
			Transformable* transform = Instance_getTransformable(instance);
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
	bool pre(const scene::Path& path, scene::Instance& instance) const {
		TransformNodePtr transformNode = Node_getTransformNode(path.top());
		if (transformNode != 0) {
			Transformable* transform = Instance_getTransformable(instance);
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
	bool pre(const scene::Path& path, scene::Instance& instance) const {
		TransformNodePtr transformNode = Node_getTransformNode(path.top());
		Selectable* selectable = Instance_getSelectable(instance);
				
		if (transformNode != NULL && selectable != NULL && selectable->isSelected()) {
			Transformable* transform = Instance_getTransformable(instance);
			if (transform != NULL) {
				transform->revertTransform(); 
			}
		}
		return true;
	}
};

// greebo: Calculates the axis-aligned bounding box of the selection.
// The constructor is called with a reference to an AABB variable that is updated during the walk
class BoundsSelected : public scene::Graph::Walker {
	AABB& _bounds;
public:
	BoundsSelected(AABB& bounds): _bounds(bounds) {
		_bounds = AABB();
	}
	
	bool pre(const scene::Path& path, scene::Instance& instance) const {
		Selectable* selectable = Instance_getSelectable(instance);
		// Only update the aabb variable if the instance is selected 
		if (selectable != 0 && selectable->isSelected()) {
			_bounds.includeAABB(Instance_getPivotBounds(instance));
		}
		return true;
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
  
	bool pre(const scene::Path& path, scene::Instance& instance) const {
		Selectable* selectable = Instance_getSelectable(instance);
		// Only update the aabb variable if the instance is selected
		if (selectable != 0 && selectable->isSelected()) {
			ComponentEditable* componentEditable = Instance_getComponentEditable(instance);
			if (componentEditable) {
				_bounds.includeAABB(
					aabb_for_oriented_aabb_safe(componentEditable->getSelectedComponentsBounds(), 
												instance.localToWorld()));
			}
		}
		return true;
	}
};

#endif /*SCENEWALKERS_H_*/
