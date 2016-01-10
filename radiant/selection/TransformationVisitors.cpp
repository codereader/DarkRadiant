#include "TransformationVisitors.h"

#include "editable.h"
#include "Manipulatables.h"
#include "transformlib.h"
#include "registry/registry.h"
#include "selection/algorithm/General.h"

// greebo: The implementation of those geometric helper functions

Vector3 get_local_pivot(const Vector3& world_pivot, const Matrix4& localToWorld)
{
	return localToWorld.getFullInverse().transformPoint(world_pivot);
}

void translation_for_pivoted_rotation(Vector3& parent_translation, const Quaternion& local_rotation,
									  const Vector3& world_pivot, const Matrix4& localToWorld,
									  const Matrix4& localToParent)
{
  Vector3 local_pivot(get_local_pivot(world_pivot, localToWorld));

  Vector3 translation(
      local_pivot +
      Matrix4::getRotationQuantised(local_rotation).transformPoint(-local_pivot)
  );

  //rMessage() << "translation: " << translation << "\n";

  translation_local2object(parent_translation, translation, localToParent);

  //rMessage() << "parent_translation: " << parent_translation << "\n";
}

void translation_for_pivoted_scale(Vector3& parent_translation, const Vector3& local_scale,
									const Vector3& world_pivot, const Matrix4& localToWorld,
									const Matrix4& localToParent)
{
  Vector3 local_pivot(get_local_pivot(world_pivot, localToWorld));

  Vector3 translation(
      local_pivot +
      (-local_pivot) * local_scale
  );

  translation_local2object(parent_translation, translation, localToParent);
}

// ===================================================================================

void TranslateSelected::visit(const scene::INodePtr& node) const {
	ITransformablePtr transform = Node_getTransformable(node);
    if(transform != 0) {
    	transform->setType(TRANSFORM_PRIMITIVE);
    	transform->setTranslation(m_translate);
    }
}

// ===================================================================================

RotateSelected::RotateSelected(const Quaternion& rotation, const Vector3& world_pivot) : 
    _rotation(rotation), 
    _worldPivot(world_pivot),
    _freeObjectRotation(registry::getValue<bool>(selection::algorithm::RKEY_FREE_OBJECT_ROTATION))
{}

void RotateSelected::visit(const scene::INodePtr& node) const
{
    ITransformNodePtr transformNode = Node_getTransformNode(node);

    if (transformNode) 
    {
        // Upcast the instance onto a Transformable
        ITransformablePtr transformable = Node_getTransformable(node);

        if (transformable)
        {
            // The object is not scaled or translated explicitly
            // A translation might occur due to the rotation around a pivot point
            transformable->setType(TRANSFORM_PRIMITIVE);
            transformable->setScale(c_scale_identity);
            transformable->setTranslation(c_translation_identity);

            // Pass the rotation quaternion and the world pivot, 
            // unless we're rotating each object around their own center
            transformable->setRotation(_rotation, 
                _freeObjectRotation ? transformable->getUntransformedOrigin() : _worldPivot, 
                node->localToWorld());
        }
    }
}

// ===================================================================================

void ScaleSelected::visit(const scene::INodePtr& node) const {
    ITransformNodePtr transformNode = Node_getTransformNode(node);
    if(transformNode != 0)
    {
      ITransformablePtr transform = Node_getTransformable(node);
      if(transform != 0)
      {
        transform->setType(TRANSFORM_PRIMITIVE);
        transform->setScale(c_scale_identity);
        transform->setTranslation(c_translation_identity);

        transform->setType(TRANSFORM_PRIMITIVE);
        transform->setScale(m_scale);
        {
          EditablePtr editable = Node_getEditable(node);
          const Matrix4& localPivot = editable != 0 ? editable->getLocalPivot() : Matrix4::getIdentity();

          Vector3 parent_translation;
          translation_for_pivoted_scale(
            parent_translation,
            m_scale,
            m_world_pivot,
			node->localToWorld().getMultipliedBy(localPivot),
			transformNode->localToParent().getMultipliedBy(localPivot)
          );

          transform->setTranslation(parent_translation);
        }
      }
    }
}

// ====== Component Visitors ==========================================================

void TranslateComponentSelected::visit(const scene::INodePtr& node) const {
    ITransformablePtr transform = Node_getTransformable(node);
    if(transform != 0)
    {
      transform->setType(TRANSFORM_COMPONENT);
      transform->setTranslation(m_translate);
    }
}

void RotateComponentSelected::visit(const scene::INodePtr& node) const {
    ITransformablePtr transform = Node_getTransformable(node);
    if(transform != 0) {
      Vector3 parent_translation;
      translation_for_pivoted_rotation(parent_translation, m_rotate, m_world_pivot,
						node->localToWorld(),
      					Node_getTransformNode(node)->localToParent());

      transform->setType(TRANSFORM_COMPONENT);
      transform->setRotation(m_rotate);
      transform->setTranslation(parent_translation);
    }
}

void ScaleComponentSelected::visit(const scene::INodePtr& node) const {
    ITransformablePtr transform = Node_getTransformable(node);
    if(transform != 0) {
      Vector3 parent_translation;
	  translation_for_pivoted_scale(parent_translation, m_scale, m_world_pivot, node->localToWorld(), Node_getTransformNode(node)->localToParent());

      transform->setType(TRANSFORM_COMPONENT);
      transform->setScale(m_scale);
      transform->setTranslation(parent_translation);
    }
}

// ============ Scene Traversors ================================

// greebo: these could be implemented into the RadiantSelectionSystem class as well, if you ask me

/* greebo: This is called when a selected item is to be translated
 * This basically cycles through all selected objects with a translation
 * visitor class (which derives from SelectionSystem::Visitor)
 */
void Scene_Translate_Selected(scene::Graph& graph, const Vector3& translation) {
  // Check if there is anything to do
  if(GlobalSelectionSystem().countSelected() != 0) {
  	// Cycle through the selected items and apply the translation
  	GlobalSelectionSystem().foreachSelected(TranslateSelected(translation));
  }
}

// The same as Scene_Translate_Selected, just that components are translated
void Scene_Translate_Component_Selected(scene::Graph& graph, const Vector3& translation) {
  if(GlobalSelectionSystem().countSelected() != 0)
  {
    GlobalSelectionSystem().foreachSelectedComponent(TranslateComponentSelected(translation));
  }
}

// The same as Scene_Rotate_Selected, just that components are rotated
void Scene_Rotate_Component_Selected(scene::Graph& graph, const Quaternion& rotation, const Vector3& world_pivot) {
  if(GlobalSelectionSystem().countSelectedComponents() != 0)
  {
    GlobalSelectionSystem().foreachSelectedComponent(RotateComponentSelected(rotation, world_pivot));
  }
}
/* greebo: This is called when a selected item is to be scaled
 * This basically cycles through all selected objects with a scale
 * visitor class (which derives from SelectionSystem::Visitor)
 */
void Scene_Scale_Selected(scene::Graph& graph, const Vector3& scaling, const Vector3& world_pivot) {
  if(GlobalSelectionSystem().countSelected() != 0)
  {
    GlobalSelectionSystem().foreachSelected(ScaleSelected(scaling, world_pivot));
  }
}

// The same as Scene_Scale_Selected, just that components are scaled
void Scene_Scale_Component_Selected(scene::Graph& graph, const Vector3& scaling, const Vector3& world_pivot) {
  if(GlobalSelectionSystem().countSelectedComponents() != 0)
  {
    GlobalSelectionSystem().foreachSelectedComponent(ScaleComponentSelected(scaling, world_pivot));
  }
}
