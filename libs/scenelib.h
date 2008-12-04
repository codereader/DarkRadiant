/*
Copyright (C) 2001-2006, William Joseph.
All Rights Reserved.
 
This file is part of GtkRadiant.
 
GtkRadiant is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.
 
GtkRadiant is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
 
You should have received a copy of the GNU General Public License
along with GtkRadiant; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#if !defined (INCLUDED_SCENELIB_H)
#define INCLUDED_SCENELIB_H

#include "Bounded.h"
#include "inode.h"
#include "iscenegraph.h"
#include "iselection.h"
#include "itraversable.h"
#include "itransformnode.h"
#include "ientity.h"
#include "ipatch.h"
#include "ibrush.h"

#include "warnings.h"
#include <cstddef>
#include <string.h>
#include <list>
#include <stack>

#include "debugging/debugging.h"
#include "math/aabb.h"
#include "transformlib.h"
#include "generic/callback.h"
#include <boost/shared_ptr.hpp>

class Selector;
class SelectionTest;
class VolumeTest;
template<typename Element>
class BasicVector4;
typedef BasicVector4<double> Vector4;
class Matrix4;
typedef Vector4 Quaternion;
class AABB;

/** greebo: This is used to identify child brushes of entities.
 */
class BrushDoom3 {
public:
	/** greebo: Translates the brush about the given <translation> vector.
	 */
	virtual void translateDoom3Brush(const Vector3& translation) = 0;
};
typedef boost::shared_ptr<BrushDoom3> BrushDoom3Ptr;

#include "scene/Node.h"

inline void Node_traverseSubgraph(const scene::INodePtr& node, scene::NodeVisitor& visitor) {
	if (node == NULL) return;

	// First, visit the node itself
	if (visitor.pre(node)) {
		// The walker requested to descend the children of this node as well,
		node->traverse(visitor);
	}
	
	visitor.post(node);
}

inline void Path_deleteTop(const scene::Path& path) {
	if (path.size() > 1) {
		path.parent()->removeChildNode(path.top());
	}
}

inline SelectablePtr Node_getSelectable(const scene::INodePtr& node) {
	return boost::dynamic_pointer_cast<Selectable>(node);
}

inline void Node_setSelected(const scene::INodePtr& node, bool selected) {
	SelectablePtr selectable = Node_getSelectable(node);
	if (selectable != NULL) {
		selectable->setSelected(selected);
	}
}

inline bool Node_isSelected(const scene::INodePtr& node) {
	SelectablePtr selectable = Node_getSelectable(node);
	if (selectable != NULL) {
		return selectable->isSelected();
	}
	return false;
}

inline BoundedPtr Node_getBounded(const scene::INodePtr& node) {
	return boost::dynamic_pointer_cast<Bounded>(node);
}

inline bool Node_isPrimitive(scene::INodePtr node) {
	return Node_isBrush(node) || Node_isPatch(node);
}

class ParentBrushes : 
	public scene::NodeVisitor
{
	scene::INodePtr m_parent;
public:
	ParentBrushes(scene::INodePtr parent) : 
		m_parent(parent)
	{}

	virtual bool pre(const scene::INodePtr& node) {
		return false;
	}

	virtual void post(const scene::INodePtr& node) {
		if (Node_isPrimitive(node)) {
			m_parent->addChildNode(node);
		}
	}
};

inline void parentBrushes(const scene::INodePtr& subgraph, const scene::INodePtr& parent) {
	ParentBrushes visitor(parent);
	subgraph->traverse(visitor);
}

class HasBrushes : 
	public scene::NodeVisitor
{
	bool& m_hasBrushes;
public:
	HasBrushes(bool& hasBrushes) : 
		m_hasBrushes(hasBrushes)
	{
		m_hasBrushes = true;
	}
	
	virtual bool pre(const scene::INodePtr& node) {
		if(!Node_isPrimitive(node)) {
			m_hasBrushes = false;
		}
		return false;
	}
};

inline bool node_is_group(scene::INodePtr node) {
	// A node without child nodes is not a group
	if (!node->hasChildNodes()) {
		return false;
	}

	bool hasBrushes = false;
	HasBrushes visitor(hasBrushes);

	node->traverse(visitor);
	return hasBrushes;
}

namespace scene {

/**
 * greebo: This removes the given node from its parent node.
 *         The node is also deselected beforehand.
 */
inline void removeNodeFromParent(const scene::INodePtr& node) {
	// Unselect the node
	Node_setSelected(node, false);

	scene::INodePtr parent = node->getParent();
	assert(parent != NULL);

	parent->removeChildNode(node);
}

/** 
 * greebo: This assigns the given node to the given set of layers. Any previous
 *         assignments of the node get overwritten by this routine.
 */
inline void assignNodeToLayers(const scene::INodePtr& node, const scene::LayerList& layers) {
	if (layers.size() > 0) {
		scene::LayerList::const_iterator i = layers.begin();

		// Move the node to the first layer (so that it gets removed from all others)
		node->moveToLayer(*i);

		// Add the node to all remaining layers
		for (; i != layers.end(); i++) {
			node->addToLayer(*i);
		}
	}
}

class UpdateNodeVisibilityWalker :
	public scene::NodeVisitor
{
	std::stack<bool> _visibilityStack;
public:
	bool pre(const INodePtr& node) {
		// Update the node visibility and store the result
		bool nodeIsVisible = GlobalLayerSystem().updateNodeVisibility(node);

		// Add a new element for this level
		_visibilityStack.push(nodeIsVisible);

		return true;
	}

	void post(const INodePtr& node) {
		// Is this child visible?
		bool childIsVisible = _visibilityStack.top();

		_visibilityStack.pop();

		if (childIsVisible) {
			// Show the node, regardless whether it was hidden before
			// otherwise the parent would hide the visible children as well
			node->disable(Node::eLayered);
		}

		if (!node->visible()) {
			// Node is hidden after update (and no children are visible), de-select
			Node_setSelected(node, false);
		}

		if (childIsVisible && !_visibilityStack.empty()) {
			// The child was visible, set this parent to true
			_visibilityStack.top() = true;
		}
	}
};

/**
 * greebo: This method inserts the given node into the given container
 *         and ensures that the container's layer visibility is updated.
 */
inline void addNodeToContainer(const scene::INodePtr& node, const scene::INodePtr& container) {
	// Insert the child
	container->addChildNode(node);

	// Ensure that worldspawn is visible
	UpdateNodeVisibilityWalker walker;
	Node_traverseSubgraph(container, walker);
}

// This in combination with Instance_getLight can be used to
// cast an instance onto a light and identify it as such.
class SelectableLight {
public:
	/** greebo: Get the AABB of the Light "Diamond" representation.
	 */
	virtual AABB getSelectAABB() = 0;
};
typedef boost::shared_ptr<SelectableLight> SelectableLightPtr;

} // namespace scene

template<typename Functor>
class NodeWalker : 
	public scene::Graph::Walker
{
	const Functor& m_functor;
public:
	NodeWalker(const Functor& functor) : 
		m_functor(functor)
	{}

	bool pre(const scene::Path& path, const scene::INodePtr& node) const {
		m_functor(node);
		return true;
	}
};

template<typename Type, typename Functor>
class InstanceApply : 
	public Functor
{
public:
	InstanceApply(const Functor& functor) : 
		Functor(functor)
	{}
	
	void operator()(const scene::INodePtr& node) const {
		boost::shared_ptr<Type> result = boost::dynamic_pointer_cast<Type>(node);
		if (result != NULL) {
			Functor::operator()(result);
		}
	}
};

inline TransformablePtr Node_getTransformable(const scene::INodePtr& node) {
	return boost::dynamic_pointer_cast<Transformable>(node);
}

inline scene::SelectableLightPtr Node_getLight(const scene::INodePtr& node) {
	return boost::dynamic_pointer_cast<scene::SelectableLight>(node);
}

class NodeSelector : 
	public scene::NodeVisitor
{
public:
	virtual bool pre(const scene::INodePtr& node) {
		Node_setSelected(node, true);
		return false;
	}
};

class InstanceCounter {
public:
	unsigned int m_count;
	InstanceCounter() : 
		m_count(0)
	{}
};

/** greebo: Cast a node onto a BrushDoom3 pointer
 * 
 * @returns: NULL, if failed, the pointer to the class otherwise.
 */
inline BrushDoom3Ptr Node_getBrushDoom3(scene::INodePtr node) {
	return boost::dynamic_pointer_cast<BrushDoom3>(node);
}

class ChildRotator :
	public scene::NodeVisitor
{
	Quaternion _rotation;
public:
	ChildRotator(const Quaternion& rotation) :
		_rotation(rotation)
	{}

	virtual bool pre(const scene::INodePtr& node) {
		TransformablePtr transformable = Node_getTransformable(node);

		if (transformable != NULL) {
			transformable->setType(TRANSFORM_PRIMITIVE);
			transformable->setRotation(_rotation);
		}

		return true;
	}
};

class ChildTransformReverter :
	public scene::NodeVisitor
{
public:
	virtual bool pre(const scene::INodePtr& node) {
		TransformablePtr transformable = Node_getTransformable(node);

		if (transformable != NULL) {
			transformable->revertTransform();
		}
		return true;
	}
};

class ChildTransformFreezer :
	public scene::NodeVisitor
{
public:
	virtual bool pre(const scene::INodePtr& node) {
		TransformablePtr transformable = Node_getTransformable(node);

		if (transformable != NULL) {
			transformable->freezeTransform();
		}
		return true;
	}
};

class ChildTranslator :
	public scene::NodeVisitor
{
	Vector3 _translation;
public:
	ChildTranslator(const Vector3& translation) :
		_translation(translation)
	{}

	virtual bool pre(const scene::INodePtr& node) {
		TransformablePtr transformable = Node_getTransformable(node);

		if (transformable != NULL) {
			transformable->setType(TRANSFORM_PRIMITIVE);
			transformable->setTranslation(_translation);
		}
		return true;
	}
};

inline void translateDoom3Brush(scene::INodePtr node, const Vector3& translation) {
	// Check for BrushDoom3
	BrushDoom3Ptr brush = Node_getBrushDoom3(node);
	if (brush != NULL) {
		brush->translateDoom3Brush(translation);
	}
}

class Doom3BrushTranslator :
	public scene::NodeVisitor
{
	Vector3 m_origin;
public:
	Doom3BrushTranslator(const Vector3& origin) :
		m_origin(origin)
	{}

	virtual bool pre(const scene::INodePtr& node) {
		translateDoom3Brush(node, m_origin);
		return true;
	}
};

// greebo: These tool methods have been moved from map.cpp, they might come in handy
enum ENodeType {
    eNodeUnknown,
    eNodeMap,
    eNodeEntity,
    eNodePrimitive,
};

inline std::string nodetype_get_name(ENodeType type) {
	if (type == eNodeMap)
		return "map";
	if (type == eNodeEntity)
		return "entity";
	if (type == eNodePrimitive)
		return "primitive";
	return "unknown";
}

inline ENodeType node_get_nodetype(scene::INodePtr node) {
	if (Node_isEntity(node)) {
		return eNodeEntity;
	}
	if (Node_isPrimitive(node)) {
		return eNodePrimitive;
	}
	return eNodeUnknown;
}

class SelectedDescendantWalker : 
	public scene::NodeVisitor
{
	bool& m_selected;
public:
	SelectedDescendantWalker(bool& selected) :
		m_selected(selected)
	{
		m_selected = false;
	}

	virtual bool pre(const scene::INodePtr& node) {
		if (node->isRoot()) {
			return false;
		}

		if (Node_isSelected(node)) {
			m_selected = true;
		}

		return true;
	}
};

inline bool Node_selectedDescendant(scene::INodePtr node) {
	bool selected;

	SelectedDescendantWalker visitor(selected);
	Node_traverseSubgraph(node, visitor);

	return selected;
}

class NodePathFinder :
	public scene::Graph::Walker
{
	mutable scene::Path _path;

	// The node to find
	const scene::INodePtr _needle;
public:
	NodePathFinder(const scene::INodePtr& needle) :
		_needle(needle)
	{}

	virtual bool pre(const scene::Path& path, const scene::INodePtr& node) const {
		if (node == _needle) {
			_path = path; // found!
		}
		// Descend deeper if path is still empty
		return _path.empty();
	}

	const scene::Path& getPath() {
		return _path;
	}
};

// greebo: Returns the path for the given node (SLOW, traverses the scenegraph!)
inline scene::Path findPath(const scene::INodePtr& node) {
	NodePathFinder finder(node);
	GlobalSceneGraph().traverse(finder);
	return finder.getPath();
}

#endif
