#ifndef DOOM3GROUPORIGIN_H_
#define DOOM3GROUPORIGIN_H_

#include "scenelib.h"

namespace entity {

/** greebo: This checks for primitive nodes of types BrushDoom3/PatchDoom3 and passes 
 * the new origin to them so they can take the according actions like
 * TexDef translation in case of an active texture lock, etc.
 */
inline void Primitives_setDoom3GroupOrigin(scene::Node& node, const Vector3& origin) {
	return;
	// Check for BrushDoom3
	BrushDoom3* brush = Node_getBrushDoom3(node);
	if (brush != NULL) {
		brush->setDoom3GroupOrigin(origin);
	}
}

inline void translateDoom3Brush(scene::Node& node, const Vector3& translation) {
	// Check for BrushDoom3
	BrushDoom3* brush = Node_getBrushDoom3(node);
	if (brush != NULL) {
		brush->translateDoom3Brush(translation);
	}
}


class Doom3BrushTranslator : 
	public scene::Traversable::Walker
{
	const Vector3& m_origin;
public:
	Doom3BrushTranslator(const Vector3& origin) : 
		m_origin(origin) 
	{}
	
	bool pre(scene::Node& node) const {
		translateDoom3Brush(node, m_origin);
		return true;
	}
};

class SetDoom3GroupOriginWalker : 
	public scene::Traversable::Walker
{
	const Vector3& m_origin;
public:
	SetDoom3GroupOriginWalker(const Vector3& origin) : 
		m_origin(origin) 
	{}
	
	bool pre(scene::Node& node) const {
		Primitives_setDoom3GroupOrigin(node, m_origin);
		return true;
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
inline void translateBrushes(scene::Node& node, const Vector3& childTranslation) {
	std::cout << "translateBrushes...";
	if (Node_getBrushDoom3(node) != NULL) {
		std::cout << "found BrushDoom3.\n";
		// Try to retrieve a traversable out of the given node
		scene::Traversable* traversable = Node_getTraversable(node);
	
		if (traversable != NULL) {
			std::cout << "Traversable found.\n";
			traversable->traverse(ChildTranslator(childTranslation));
		}
	}
}

/** greebo: This is an Observer helper class that gets notified 
 * on incoming child nodes. These are translated with the given origin
 * on insertion which takes care that the child brushes/patches are
 * measured relatively to the Doom3Group origin key. 
 */
class Doom3GroupOrigin : 
	public scene::Traversable::Observer
{
	scene::Traversable& m_set;
	const Vector3& m_origin;
	bool m_enabled;

public:
	Doom3GroupOrigin(scene::Traversable& set, const Vector3& origin);

	// Enable the automatic translation of child nodes
	void enable();
	void disable();
	
	/** greebo: Translates all child brushes about +/-m_origin to store
	 * their position relatively to the worldspawn/func_static origin.
	 */
	void addOriginToChildren();
	void removeOriginFromChildren();

	void insert(scene::Node& node);
	void erase(scene::Node& node);
};

} // namespace entity

#endif /*DOOM3GROUPORIGIN_H_*/
