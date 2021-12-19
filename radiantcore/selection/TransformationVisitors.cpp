#include "TransformationVisitors.h"

#include "editable.h"
#include "manipulators/ManipulatorComponents.h"
#include "transformlib.h"
#include "registry/registry.h"
#include "selection/algorithm/General.h"

// greebo: This is needed e.g. to calculate the translation vector of a rotation transformation
Vector3 get_local_pivot(const Vector3& world_pivot, const Matrix4& localToWorld)
{
	return localToWorld.getFullInverse().transformPoint(world_pivot);
}

// greebo: Calculates the translation vector of a rotation about a pivot point,
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

  selection::translation_local2object(parent_translation, translation, localToParent);

  //rMessage() << "parent_translation: " << parent_translation << "\n";
}

// greebo: Calculates the translation vector of a scale transformation based on a pivot point,
void translation_for_pivoted_scale(Vector3& parent_translation, const Vector3& local_scale,
									const Vector3& world_pivot, const Matrix4& localToWorld,
									const Matrix4& localToParent)
{
  Vector3 local_pivot(get_local_pivot(world_pivot, localToWorld));

  Vector3 translation(
      local_pivot +
      (-local_pivot) * local_scale
  );

  selection::translation_local2object(parent_translation, translation, localToParent);
}

// ===================================================================================

void TranslateSelected::visit(const scene::INodePtr& node) const {
	ITransformablePtr transform = scene::node_cast<ITransformable>(node);
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
    ITransformNodePtr transformNode = scene::node_cast<ITransformNode>(node);

    if (transformNode)
    {
        // Upcast the instance onto a Transformable
        ITransformablePtr transformable = scene::node_cast<ITransformable>(node);

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
    ITransformNodePtr transformNode = scene::node_cast<ITransformNode>(node);
    if(transformNode != 0)
    {
      ITransformablePtr transform = scene::node_cast<ITransformable>(node);
      if(transform != 0)
      {
        transform->setType(TRANSFORM_PRIMITIVE);
        transform->setScale(c_scale_identity);
        transform->setTranslation(c_translation_identity);

        transform->setType(TRANSFORM_PRIMITIVE);
        transform->setScale(m_scale);
        {
          Vector3 parent_translation;
          translation_for_pivoted_scale(
            parent_translation,
            m_scale,
            m_world_pivot,
			node->localToWorld(),
			transformNode->localToParent()
          );

          transform->setTranslation(parent_translation);
        }
      }
    }
}

// ====== Component Visitors ==========================================================

void TranslateComponentSelected::visit(const scene::INodePtr& node) const {
    ITransformablePtr transform = scene::node_cast<ITransformable>(node);
    if(transform != 0)
    {
      transform->setType(TRANSFORM_COMPONENT);
      transform->setTranslation(m_translate);
    }
}

void RotateComponentSelected::visit(const scene::INodePtr& node) const {
    ITransformablePtr transform = scene::node_cast<ITransformable>(node);
    if(transform != 0) {
      Vector3 parent_translation;
      translation_for_pivoted_rotation(parent_translation, m_rotate, m_world_pivot,
						node->localToWorld(),
      					scene::node_cast<ITransformNode>(node)->localToParent());

      transform->setType(TRANSFORM_COMPONENT);
      transform->setRotation(m_rotate);
      transform->setTranslation(parent_translation);
    }
}

void ScaleComponentSelected::visit(const scene::INodePtr& node) const {
    ITransformablePtr transform = scene::node_cast<ITransformable>(node);
    if(transform != 0) {
      Vector3 parent_translation;
	  translation_for_pivoted_scale(parent_translation, m_scale, m_world_pivot, node->localToWorld(), scene::node_cast<ITransformNode>(node)->localToParent());

      transform->setType(TRANSFORM_COMPONENT);
      transform->setScale(m_scale);
      transform->setTranslation(parent_translation);
    }
}
