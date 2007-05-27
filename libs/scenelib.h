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

#include "math/aabb.h"
#include "transformlib.h"
#include "generic/callback.h"
#include <boost/shared_ptr.hpp>

class Selector;
class SelectionTest;
class VolumeTest;
template<typename Element>
class BasicVector3;
typedef BasicVector3<double> Vector3;
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
	STRING_CONSTANT(Name, "BrushDoom3");

	/** greebo: Translates the brush about the given <translation> vector.
	 */
	virtual void translateDoom3Brush(const Vector3& translation) = 0;
};
typedef boost::shared_ptr<BrushDoom3> BrushDoom3Ptr;

namespace scene {

// Auto-incrementing ID (contains the largest ID in use)
static unsigned long _maxNodeId;

class Node :
	public INode
{
public:
	enum { 
		eVisible = 0,
		eHidden = 1 << 0,
		eFiltered = 1 << 1,
		eExcluded = 1 << 2
	};

private:
	unsigned int _state;
	bool _isRoot;
	unsigned long _id;
	
public:
	Node() :
		_state(eVisible),
		_isRoot(false),
		_id(getNewId()) // Get new auto-incremented ID
	{
		//std::cout << "Node #" << _id << " constructed.\n";
	}
	
	Node(const Node& other) :
		INode(other),
		_state(other._state),
		_isRoot(other._isRoot),
		_id(getNewId())	// ID is incremented on copy
	{
		//std::cout << "Node #" << _id << " constructed by copying.\n";
	}
	
	virtual ~Node() {
		//std::cout << "Node #" << _id << " destructed.\n";
	}
	
	static void resetIds() {
		_maxNodeId = 0;
	}
	
	static unsigned long getNewId() {
		return ++_maxNodeId;
	}
	
	bool isRoot() const {
		return _isRoot;
	}
	
	void setIsRoot(bool isRoot) {
		_isRoot = isRoot;
	}
	
	void enable(unsigned int state) {
		_state |= state;
	}

	void disable(unsigned int state) {
		_state &= ~state;
	}

	bool visible() const {
		return _state == eVisible;
	}

	bool excluded() const {
		return (_state & eExcluded) != 0;
	}
};

} // namespace scene

inline scene::InstantiablePtr Node_getInstantiable(scene::INodePtr node) {
	return boost::dynamic_pointer_cast<scene::Instantiable>(node);
}

inline void Node_traverseSubgraph(scene::INodePtr node, const scene::Traversable::Walker& walker) {
	// First, visit the node itself
	if (walker.pre(node)) {
		// The walker requested to descend the children of this node as well,
		// check if this node has any children (i.e. is Traversable)  
		scene::TraversablePtr traversable = Node_getTraversable(node);
		if (traversable != NULL) {
			traversable->traverse(walker);
		}
	}
	walker.post(node);
}

inline scene::INodePtr NewNullNode() {
	return scene::INodePtr(new scene::Node);
}

inline void Path_deleteTop(const scene::Path& path) {
	scene::TraversablePtr traversable = Node_getTraversable(path.parent());
	if (traversable != NULL) {
		traversable->erase(path.top());
	}
}


class delete_all : 
	public scene::Traversable::Walker
{
	scene::INodePtr m_parent;
public:
	delete_all(scene::INodePtr parent) : m_parent(parent) {}
	bool pre(scene::INodePtr node) const {
		return false;
	}
	void post(scene::INodePtr node) const {
		Node_getTraversable(m_parent)->erase(node);
	}
};

inline void DeleteSubgraph(scene::INodePtr subgraph) {
	Node_getTraversable(subgraph)->traverse(delete_all(subgraph));
}

template<typename Functor>
class EntityWalker : public scene::Graph::Walker {
	const Functor& functor;
public:
	EntityWalker(const Functor& functor) : functor(functor) {}
	bool pre(const scene::Path& path, scene::Instance& instance) const {
		if(Node_isEntity(path.top())) {
			functor(instance);
			return false;
		}
		return true;
	}
};

template<typename Functor>
inline const Functor& Scene_forEachEntity(const Functor& functor) {
	GlobalSceneGraph().traverse(EntityWalker<Functor>(functor));
	return functor;
}

inline bool Node_isPrimitive(scene::INodePtr node) {
#if 1
	return Node_isBrush(node) || Node_isPatch(node);
#else

	return !node.isRoot();
#endif
}

class ParentBrushes : 
	public scene::Traversable::Walker
{
	scene::INodePtr m_parent;
public:
	ParentBrushes(scene::INodePtr parent) : 
		m_parent(parent)
	{}
	
	bool pre(scene::INodePtr node) const {
		return false;
	}
	
	void post(scene::INodePtr node) const {
		if(Node_isPrimitive(node)) {
			Node_getTraversable(m_parent)->insert(node);
		}
	}
};

inline void parentBrushes(scene::INodePtr subgraph, scene::INodePtr parent) {
	Node_getTraversable(subgraph)->traverse(ParentBrushes(parent));
}

class HasBrushes : 
	public scene::Traversable::Walker
{
	bool& m_hasBrushes;
public:
	HasBrushes(bool& hasBrushes) : 
		m_hasBrushes(hasBrushes)
	{
		m_hasBrushes = true;
	}
	
	bool pre(scene::INodePtr node) const {
		if(!Node_isPrimitive(node)) {
			m_hasBrushes = false;
		}
		return false;
	}
};

inline bool node_is_group(scene::INodePtr node) {
	scene::TraversablePtr traversable = Node_getTraversable(node);
	if (traversable != NULL) {
		bool hasBrushes = false;
		traversable->traverse(HasBrushes(hasBrushes));
		return hasBrushes;
	}
	return false;
}

inline Selectable* Instance_getSelectable(scene::Instance& instance);
inline const Selectable* Instance_getSelectable(const scene::Instance& instance);

inline Bounded* Instance_getBounded(scene::Instance& instance);
inline const Bounded* Instance_getBounded(const scene::Instance& instance);

namespace scene {

class Instance
{
	class AABBAccumulateWalker : 
		public scene::Graph::Walker
	{
		AABB& m_aabb;
		mutable std::size_t m_depth;
	public:
		AABBAccumulateWalker(AABB& aabb) : 
			m_aabb(aabb), 
			m_depth(0)
		{}
		
		bool pre(const scene::Path& path, scene::Instance& instance) const {
			if(m_depth == 1) {
				m_aabb.includeAABB(instance.worldAABB());
			}
			return ++m_depth != 2;
		}
		
		void post(const scene::Path& path, scene::Instance& instance) const {
			--m_depth;
		}
	};
	
	
	class TransformChangedWalker : 
		public scene::Graph::Walker
	{
	public:
		bool pre(const scene::Path& path, scene::Instance& instance) const {
			instance.transformChangedLocal();
			return true;
		}
	};
	
	class ParentSelectedChangedWalker : 
		public scene::Graph::Walker
	{
	public:
		bool pre(const scene::Path& path, scene::Instance& instance) const {
			instance.parentSelectedChanged();
			return true;
		}
	};
	
	class ChildSelectedWalker : 
		public scene::Graph::Walker
	{
		bool& m_childSelected;
		mutable std::size_t m_depth;
	public:
		ChildSelectedWalker(bool& childSelected) : m_childSelected(childSelected), m_depth(0) {
			m_childSelected = false;
		}
		bool pre(const scene::Path& path, scene::Instance& instance) const {
			if(m_depth == 1 && !m_childSelected) {
				m_childSelected = instance.isSelected() || instance.childSelected();
			}
			return ++m_depth != 2;
		}
		void post(const scene::Path& path, scene::Instance& instance) const {
			--m_depth;
		}
	};

	Path m_path;
	Instance* m_parent;

	mutable Matrix4 m_local2world;
	mutable AABB m_bounds;
	mutable AABB m_childBounds;
	mutable bool m_transformChanged;
	mutable bool m_transformMutex;
	mutable bool m_boundsChanged;
	mutable bool m_boundsMutex;
	mutable bool m_childBoundsChanged;
	mutable bool m_childBoundsMutex;
	mutable bool m_isSelected;
	mutable bool m_isSelectedChanged;
	mutable bool m_childSelected;
	mutable bool m_childSelectedChanged;
	mutable bool m_parentSelected;
	mutable bool m_parentSelectedChanged;
	Callback m_childSelectedChangedCallback;
	Callback m_transformChangedCallback;


	void evaluateTransform() const {
		if(m_transformChanged) {
			ASSERT_MESSAGE(!m_transformMutex, "re-entering transform evaluation");
			m_transformMutex = true;

			m_local2world = (m_parent != 0) ? m_parent->localToWorld() : g_matrix4_identity;
			TransformNodePtr transformNode = Node_getTransformNode(m_path.top());
			if (transformNode != NULL) {
				matrix4_multiply_by_matrix4(m_local2world, transformNode->localToParent());
			}

			m_transformMutex = false;
			m_transformChanged = false;
		}
	}
	void evaluateChildBounds() const {
		if(m_childBoundsChanged) {
			ASSERT_MESSAGE(!m_childBoundsMutex, "re-entering bounds evaluation");
			m_childBoundsMutex = true;

			m_childBounds = AABB();

			GlobalSceneGraph().traverse_subgraph(AABBAccumulateWalker(m_childBounds), m_path);

			m_childBoundsMutex = false;
			m_childBoundsChanged = false;
		}
	}
	void evaluateBounds() const {
		if(m_boundsChanged) {
			ASSERT_MESSAGE(!m_boundsMutex, "re-entering bounds evaluation");
			m_boundsMutex = true;

			m_bounds = childBounds();

			const Bounded* bounded = Instance_getBounded(*this);
			if(bounded != 0) {
				m_bounds.includeAABB(
				    aabb_for_oriented_aabb_safe(bounded->localAABB(), localToWorld())
				);
			}

			m_boundsMutex = false;
			m_boundsChanged = false;
		}
	}

	Instance(const scene::Instance& other);
	Instance& operator=(const scene::Instance& other);
public:

	Instance(const scene::Path& path, Instance* parent) :
		m_path(path),
		m_parent(parent),
		m_local2world(g_matrix4_identity),
		m_transformChanged(true),
		m_transformMutex(false),
		m_boundsChanged(true),
		m_boundsMutex(false),
		m_childBoundsChanged(true),
		m_childBoundsMutex(false),
		m_isSelectedChanged(true),
		m_childSelectedChanged(true),
		m_parentSelectedChanged(true)
	{
		ASSERT_MESSAGE((parent == 0) == (path.size() == 1), "instance has invalid parent");
	}
	
	virtual ~Instance() {}

	const scene::Path& path() const {
		return m_path;
	}

	const Matrix4& localToWorld() const {
		evaluateTransform();
		return m_local2world;
	}
	
	void transformChangedLocal() {
		ASSERT_NOTNULL(m_parent);
		m_transformChanged = true;
		m_boundsChanged = true;
		m_childBoundsChanged = true;
		m_transformChangedCallback();
	}
	
	void transformChanged() {
		GlobalSceneGraph().traverse_subgraph(TransformChangedWalker(), m_path);
		boundsChanged();
	}
	
	void setTransformChangedCallback(const Callback& callback) {
		m_transformChangedCallback = callback;
	}

	const AABB& worldAABB() const {
		evaluateBounds();
		return m_bounds;
	}
	
	const AABB& childBounds() const {
		evaluateChildBounds();
		return m_childBounds;
	}
	
	void boundsChanged() {
		m_boundsChanged = true;
		m_childBoundsChanged = true;
		if(m_parent != 0) {
			m_parent->boundsChanged();
		}
		GlobalSceneGraph().boundsChanged();
	}

	void childSelectedChanged() {
		m_childSelectedChanged = true;
		m_childSelectedChangedCallback();
		if(m_parent != 0) {
			m_parent->childSelectedChanged();
		}
	}
	
	bool childSelected() const {
		if(m_childSelectedChanged) {
			m_childSelectedChanged = false;
			GlobalSceneGraph().traverse_subgraph(ChildSelectedWalker(m_childSelected), m_path);
		}
		return m_childSelected;
	}

	void setChildSelectedChangedCallback(const Callback& callback) {
		m_childSelectedChangedCallback = callback;
	}
	
	void selectedChanged() {
		m_isSelectedChanged = true;
		if(m_parent != 0) {
			m_parent->childSelectedChanged();
		}
		GlobalSceneGraph().traverse_subgraph(ParentSelectedChangedWalker(), m_path);
	}
	
	bool isSelected() const {
		if(m_isSelectedChanged) {
			m_isSelectedChanged = false;
			const Selectable* selectable = Instance_getSelectable(*this);
			m_isSelected = selectable != 0 && selectable->isSelected();
		}
		return m_isSelected;
	}

	void parentSelectedChanged() {
		m_parentSelectedChanged = true;
	}
	
	bool parentSelected() const {
		if(m_parentSelectedChanged) {
			m_parentSelectedChanged = false;
			m_parentSelected = m_parent != 0 && (m_parent->isSelected() || m_parent->parentSelected());
		}
		return m_parentSelected;
	}
};

// This in combination with Instance_getLight can be used to
// cast an instance onto a light and identify it as such.
class LightInstance {
public:
	STRING_CONSTANT(Name, "SceneLightInstance");

	/** greebo: Get the AABB of the Light "Diamand" representation.
	 */
	virtual AABB getSelectAABB() = 0;
};

} // namespace scene

template<typename Functor>
class InstanceWalker : public scene::Graph::Walker {
	const Functor& m_functor;
public:
	InstanceWalker(const Functor& functor) : m_functor(functor) {}
	bool pre(const scene::Path& path, scene::Instance& instance) const {
		m_functor(instance);
		return true;
	}
};

template<typename Functor>
class ChildInstanceWalker : public scene::Graph::Walker {
	const Functor& m_functor;
	mutable std::size_t m_depth;
public:
	ChildInstanceWalker(const Functor& functor) : m_functor(functor), m_depth(0) {}
	bool pre(const scene::Path& path, scene::Instance& instance) const {
		if(m_depth == 1) {
			m_functor(instance);
		}
		return ++m_depth != 2;
	}
	void post(const scene::Path& path, scene::Instance& instance) const {
		--m_depth;
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
	
	void operator()(scene::Instance& instance) const {
		Type* result = dynamic_cast<Type*>(&instance);
		if (result != NULL) {
			Functor::operator()(*result);
		}
	}
};

inline Selectable* Instance_getSelectable(scene::Instance& instance) {
	return dynamic_cast<Selectable*>(&instance);
}
inline const Selectable* Instance_getSelectable(const scene::Instance& instance) {
	return dynamic_cast<const Selectable*>(&instance);
}

template<typename Functor>
inline void Scene_forEachChildSelectable(const Functor& functor, const scene::Path& path) {
	GlobalSceneGraph().traverse_subgraph(ChildInstanceWalker< InstanceApply<Selectable, Functor> >(functor), path);
}

class SelectableSetSelected {
	bool m_selected;
public:
	SelectableSetSelected(bool selected) : m_selected(selected) {}
	void operator()(Selectable& selectable) const {
		selectable.setSelected(m_selected);
	}
};

inline Bounded* Instance_getBounded(scene::Instance& instance) {
	return dynamic_cast<Bounded*>(&instance);
}
inline const Bounded* Instance_getBounded(const scene::Instance& instance) {
	return dynamic_cast<const Bounded*>(&instance);
}

inline Transformable* Instance_getTransformable(scene::Instance& instance) {
	return dynamic_cast<Transformable*>(&instance);
}
inline const Transformable* Instance_getTransformable(const scene::Instance& instance) {
	return dynamic_cast<const Transformable*>(&instance);
}

inline scene::LightInstance* Instance_getLight(scene::Instance& instance) {
	return dynamic_cast<scene::LightInstance*>(&instance);
}

inline void Instance_setSelected(scene::Instance& instance, bool selected) {
	Selectable* selectable = Instance_getSelectable(instance);
	if(selectable != 0) {
		selectable->setSelected(selected);
	}
}

inline bool Instance_isSelected(scene::Instance& instance) {
	Selectable* selectable = Instance_getSelectable(instance);
	if(selectable != 0) {
		return selectable->isSelected();
	}
	return false;
}

inline scene::Instance& findInstance(const scene::Path& path) {
	scene::Instance* instance = GlobalSceneGraph().find(path);
	ASSERT_MESSAGE(instance != 0, "findInstance: path not found in scene-graph");
	return *instance;
}

inline void selectPath(const scene::Path& path, bool selected) {
	Instance_setSelected(findInstance(path), selected);
}

class SelectChildren : public scene::Traversable::Walker {
	mutable scene::Path m_path;
public:
	SelectChildren(const scene::Path& root)
			: m_path(root) {}
	bool pre(scene::INodePtr node) const {
		m_path.push(node);
		selectPath(m_path, true);
		return false;
	}
	void post(scene::INodePtr node) const {
		m_path.pop();
	}
};

inline void Entity_setSelected(scene::Instance& entity, bool selected) {
	scene::INodePtr node = entity.path().top();
	if(node_is_group(node)) {
		Node_getTraversable(node)->traverse(SelectChildren(entity.path()));
	}
	else {
		Instance_setSelected(entity, selected);
	}
}

inline bool Entity_isSelected(scene::Instance& entity) {
	if(node_is_group(entity.path().top())) {
		return entity.childSelected();
	}
	return Instance_isSelected(entity);
}

class InstanceCounter {
public:
	unsigned int m_count;
	InstanceCounter() : 
		m_count(0)
	{}
};

class Counter {
public:
	virtual void increment() = 0;
	virtual void decrement() = 0;
	virtual std::size_t get() const = 0;
};

/** greebo: Cast a node onto a BrushDoom3 pointer
 * 
 * @returns: NULL, if failed, the pointer to the class otherwise.
 */
inline BrushDoom3Ptr Node_getBrushDoom3(scene::INodePtr node) {
	return boost::dynamic_pointer_cast<BrushDoom3>(node);
}

// Helper class
class InstanceFunctor {
public:
	virtual void operator() (scene::Instance& instance) const = 0;
};

/** greebo: This cycles through all the Instances of a given
 * Instantiable scene::Node, calling an InstanceFunctor on visit.
 */
class InstanceVisitor :
	public scene::Instantiable::Visitor {
	const InstanceFunctor& _functor;
public:
	InstanceVisitor(const InstanceFunctor& functor) :
	_functor(functor) {}

	void visit(scene::Instance& instance) const {
		_functor(instance);
	}
};

class ChildRotator :
			public scene::Traversable::Walker,
	public InstanceFunctor {
	const Quaternion& _rotation;
public:
	ChildRotator(const Quaternion& rotation) :
	_rotation(rotation) {}

	bool pre(scene::INodePtr node) const {
		scene::InstantiablePtr instantiable = Node_getInstantiable(node);

		if (instantiable != NULL) {
			instantiable->forEachInstance(InstanceVisitor(*this));
		}
		return true;
	}

	void operator() (scene::Instance& instance) const {
		Transformable* transformable = Instance_getTransformable(instance);

		if (transformable != NULL) {
			transformable->setRotation(_rotation);
		}
	}
};

class ChildTransformReverter :
			public scene::Traversable::Walker,
	public InstanceFunctor {
public:
	bool pre(scene::INodePtr node) const {
		scene::InstantiablePtr instantiable = Node_getInstantiable(node);

		if (instantiable != NULL) {
			instantiable->forEachInstance(InstanceVisitor(*this));
		}
		return true;
	}

	void operator() (scene::Instance& instance) const {
		Transformable* transformable = Instance_getTransformable(instance);

		if (transformable != NULL) {
			transformable->revertTransform();
		}
	}
};

class ChildTransformFreezer :
			public scene::Traversable::Walker,
	public InstanceFunctor {
public:
	bool pre(scene::INodePtr node) const {
		scene::InstantiablePtr instantiable = Node_getInstantiable(node);

		if (instantiable != NULL) {
			instantiable->forEachInstance(InstanceVisitor(*this));
		}
		return true;
	}

	void operator() (scene::Instance& instance) const {
		Transformable* transformable = Instance_getTransformable(instance);

		if (transformable != NULL) {
			transformable->freezeTransform();
		}
	}
};

class ChildTranslator :
			public scene::Traversable::Walker,
	public InstanceFunctor {
	const Vector3& _translation;
public:
	ChildTranslator(const Vector3& translation) :
	_translation(translation) {}

	bool pre(scene::INodePtr node) const {
		scene::InstantiablePtr instantiable = Node_getInstantiable(node);

		if (instantiable != NULL) {
			instantiable->forEachInstance(InstanceVisitor(*this));
		}
		return true;
	}

	void operator() (scene::Instance& instance) const {
		Transformable* transformable = Instance_getTransformable(instance);

		if (transformable != NULL) {
			transformable->setTranslation(_translation);
		}
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
	public scene::Traversable::Walker {
	const Vector3& m_origin;
public:
	Doom3BrushTranslator(const Vector3& origin) :
	m_origin(origin) {}

	bool pre(scene::INodePtr node) const {
		translateDoom3Brush(node, m_origin);
		return true;
	}
};

template<typename Contained>
class ConstReference;
typedef ConstReference<scene::Path> PathConstReference;

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

#endif
