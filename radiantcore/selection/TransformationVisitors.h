#pragma once

#include "iselection.h"
#include "iscenegraph.h"

#include "math/Vector3.h"
#include "math/Matrix4.h"
#include "math/Quaternion.h"

// Visitor classes applying the actual transformation to the visited node.

/* greebo: Visitor classes that apply a transformation to the instance passed to visit()
 *
 * The constructor expects the according transformation vectors to be passed.
 * The visit function is called with the scene::instance to be modified.
 */
class TranslateSelected :
	public SelectionSystem::Visitor
{
	// The translation vector3 (initialised in the constructor)
  	const Vector3& m_translate;

public:
	// The constructor. Instantiate this class with the translation vector3
  	TranslateSelected(const Vector3& translate): m_translate(translate) {}

	// The visitor function that applies the actual transformation to the instance
	void visit(const scene::INodePtr& node) const;
};

// -------------------------------------------------------------------------------

class RotateSelected : 
    public SelectionSystem::Visitor
{
	// The internal transformation vectors
  	const Quaternion& _rotation;
  	const Vector3& _worldPivot;
    bool _freeObjectRotation;

public:
    // Call this constructor with the rotation and pivot vectors
    RotateSelected(const Quaternion& rotation, const Vector3& world_pivot);

    // This actually applies the rotation to the node
    void visit(const scene::INodePtr& node) const override;
};

// -------------------------------------------------------------------------------

class ScaleSelected : public SelectionSystem::Visitor
{
  // The internal vectors of the transformation to be applied
  const Vector3& m_scale;
  const Vector3& m_world_pivot;
public:
  ScaleSelected(const Vector3& scaling, const Vector3& world_pivot)
    : m_scale(scaling), m_world_pivot(world_pivot) {}

  // This actually applies the scale to the node
  void visit(const scene::INodePtr& node) const;
};

// =========== Translate, Rotate, Scale Component =====================================

/* greebo: Same as above, just that components are transformed.
 * Note: This probably could be merged into the other three visitor classes, passing a bool
 * which tells the visitor if components or primitives are to be transformed
 */
class TranslateComponentSelected : public SelectionSystem::Visitor {
	// Internally stored translation vector
	const Vector3& m_translate;
public:
	// Constructor
	TranslateComponentSelected(const Vector3& translate): m_translate(translate) {}

	// This actually applies the change to the node
	void visit(const scene::INodePtr& node) const;
};

// -------------------------------------------------------------------------------

class RotateComponentSelected : public SelectionSystem::Visitor {
	// The internal transformation vectors
	const Quaternion& m_rotate;
	const Vector3& m_world_pivot;
public:
	// Constructor
	RotateComponentSelected(const Quaternion& rotation, const Vector3& world_pivot)
		: m_rotate(rotation), m_world_pivot(world_pivot) {}

    // This actually applies the change to the node
	void visit(const scene::INodePtr& node) const;
};

// -------------------------------------------------------------------------------

class ScaleComponentSelected : public SelectionSystem::Visitor {
	// The internal vectors of the transformation to be applied
	const Vector3& m_scale;
	const Vector3& m_world_pivot;
public:
	// Constructor
	ScaleComponentSelected(const Vector3& scaling, const Vector3& world_pivot)
    	: m_scale(scaling), m_world_pivot(world_pivot) {}

	// This actually applies the change to the node
  	void visit(const scene::INodePtr& node) const;
};
