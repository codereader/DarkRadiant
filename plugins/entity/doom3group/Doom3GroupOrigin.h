#ifndef DOOM3GROUPORIGIN_H_
#define DOOM3GROUPORIGIN_H_

#include "scenelib.h"

namespace entity {

class Doom3GroupOrigin : 
	public scene::Traversable::Observer
{
	scene::Traversable& m_set;
	const Vector3& m_origin;
	bool m_enabled;

public:
	Doom3GroupOrigin(scene::Traversable& set, const Vector3& origin);

	// Enable the callbacks, set the bool to TRUE if a callback should 
	// be triggered immediately.
	void enable(bool triggerOriginChange = true);
	void disable();

	// Triggers a setDoom3GroupOrigin call on all the child brushes
	// (which basically moves them according to the origin).
	void originChanged();

	void insert(scene::Node& node);
	void erase(scene::Node& node);
};

/** greebo: Helper classes that facilitate the transformation
 * of child nodes. TODO: Move this into transformlib when done
 */
 
class InstanceFunctor
{
public:
	virtual void operator() (scene::Instance& instance) const = 0;
};

/** greebo: This cycles through all the Instances of a given
 * Instantiable scene::Node, calling an InstanceFunctor on visit.
 */
class InstanceVisitor :
	public scene::Instantiable::Visitor
{
	const InstanceFunctor& _functor;
public:
	InstanceVisitor(const InstanceFunctor& functor) :
		_functor(functor)
	{}

	void visit(scene::Instance& instance) const {
		_functor(instance);
	}
};

class ChildTranslator : 
	public scene::Traversable::Walker,
	public InstanceFunctor
{
	const Vector3& _translation;
public:
	ChildTranslator(const Vector3& translation) :
		_translation(translation)
	{}

	bool pre(scene::Node& node) const {
		scene::Instantiable* instantiable = Node_getInstantiable(node);
		
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

/** greebo: Tries to translate the given node.
 * 
 * The path is as follows: 
 * 
 * scene::Node >> scene::Traversable >> WALK >> scene::Instantiable >>
 * 			   >> VISIT >> Transformable >> setTranslation() 
 */
inline void translateNode(scene::Node& node, const Vector3& childTranslation) {
	// Try to retrieve a traversable out of the given node
	scene::Traversable* traversable = Node_getTraversable(node);
	
	if (traversable != NULL) {
		traversable->traverse(ChildTranslator(childTranslation));
	}
}

class ChildRotator : 
	public scene::Traversable::Walker,
	public InstanceFunctor
{
	const Quaternion& _rotation;
public:
	ChildRotator(const Quaternion& rotation) :
		_rotation(rotation)
	{}

	bool pre(scene::Node& node) const {
		scene::Instantiable* instantiable = Node_getInstantiable(node);
		
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
	public InstanceFunctor
{
public:
	bool pre(scene::Node& node) const {
		scene::Instantiable* instantiable = Node_getInstantiable(node);
		
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
	public InstanceFunctor
{
public:
	bool pre(scene::Node& node) const {
		scene::Instantiable* instantiable = Node_getInstantiable(node);
		
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

} // namespace entity

#endif /*DOOM3GROUPORIGIN_H_*/
