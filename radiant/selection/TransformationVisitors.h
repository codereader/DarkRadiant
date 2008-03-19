#ifndef TRANSFORMATIONVISITORS_H_
#define TRANSFORMATIONVISITORS_H_

#include "iselection.h"
#include "scenelib.h"

#include "math/Vector3.h"
#include "math/matrix.h"
#include "math/quaternion.h"

/* greebo: These are the visitor classes that apply the actual transformations to 
 * the visited instance
 * 
 * Included: Translation, Rotation, Scale
 */

// =========== Helper functions ====================================================

/* greebo: This is needed e.g. to calculate the translation vector of a rotation transformation
 * It combines the local and the world pivot point, it seems
 */
Vector3 get_local_pivot(const Vector3& world_pivot, const Matrix4& localToWorld);

/* greebo: This calculates the translation vector of a rotation with a pivot point, 
 * but I'm not sure about this :)
 */
void translation_for_pivoted_rotation(Vector3& parent_translation, const Quaternion& local_rotation, 
										const Vector3& world_pivot,	const Matrix4& localToWorld, 
										const Matrix4& localToParent);

/* greebo: This calculates the translation vector of a scale transformation with a pivot point, 
 */
void translation_for_pivoted_scale(Vector3& parent_translation, const Vector3& local_scale, 
									const Vector3& world_pivot, const Matrix4& localToWorld, 
									const Matrix4& localToParent);

// =========== Translation, Scale & Rotation ==========================================

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
}; // class TranslateSelected

// -------------------------------------------------------------------------------

class RotateSelected : public SelectionSystem::Visitor {
	// The internal transformation vectors
  	const Quaternion& m_rotate;
  	const Vector3& m_world_pivot;
public:
  // Call this constructor with the rotation and pivot vectors
  RotateSelected(const Quaternion& rotation, const Vector3& world_pivot)
  	: m_rotate(rotation), m_world_pivot(world_pivot) {}
  
  // This actually applies the rotation to the node 
  void visit(const scene::INodePtr& node) const;
}; // class rotate_selected

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

// =============================================================================

/* greebo: This is called when a selected item is to be transformed
 * This basically cycles through all selected objects passing the instance to the 
 * visitor class (which derives from SelectionSystem::Visitor)
 */
void Scene_Translate_Selected(scene::Graph& graph, const Vector3& translation);
void Scene_Translate_Component_Selected(scene::Graph& graph, const Vector3& translation);

void Scene_Rotate_Selected(scene::Graph& graph, const Quaternion& rotation, const Vector3& world_pivot);
void Scene_Rotate_Component_Selected(scene::Graph& graph, const Quaternion& rotation, const Vector3& world_pivot);

void Scene_Scale_Selected(scene::Graph& graph, const Vector3& scaling, const Vector3& world_pivot);
void Scene_Scale_Component_Selected(scene::Graph& graph, const Vector3& scaling, const Vector3& world_pivot);

#endif /*TRANSFORMATIONVISITORS_H_*/
