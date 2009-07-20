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

#if !defined (INCLUDED_TRANSFORMLIB_H)
#define INCLUDED_TRANSFORMLIB_H

#include "itransformable.h"
#include "itransformnode.h"
#include "math/matrix.h"
#include "math/quaternion.h"

/// \brief A transform node which has no effect.
class IdentityTransform : public TransformNode
{
public:
  /// \brief Returns the identity matrix.
  const Matrix4& localToParent() const
  {
    return Matrix4::getIdentity();
  }
};

/// \brief A transform node which stores a generic transformation matrix.
class MatrixTransform : public TransformNode
{
  Matrix4 m_localToParent;
public:
  MatrixTransform() : m_localToParent(Matrix4::getIdentity())
  {
  }

  Matrix4& localToParent()
  {
    return m_localToParent;
  }
  /// \brief Returns the stored local->parent transform.
  const Matrix4& localToParent() const
  {
    return m_localToParent;
  }
};


#include "generic/callback.h"

inline Matrix4 matrix4_transform_for_components(const Vector3& translation, const Quaternion& rotation, const Vector3& scale)
{
  Matrix4 result(matrix4_rotation_for_quaternion_quantised(rotation));
  result.x().getVector3() *= scale.x();
  result.y().getVector3() *= scale.y();
  result.z().getVector3() *= scale.z();
  result.tx() = translation.x();
  result.ty() = translation.y();
  result.tz() = translation.z();
  return result;
}

const Vector3 c_translation_identity(0, 0, 0);
const Quaternion c_rotation_identity(c_quaternion_identity);
const Vector3 c_scale_identity(1, 1, 1);

class Transformable : public ITransformable
{
	Vector3 m_translation;
	Quaternion m_rotation;
	Vector3 m_scale;

	Callback m_changed;
	Callback m_apply;

	TransformModifierType m_type;
public:

  Transformable(const Callback& changed, const Callback& apply) :
    m_translation(c_translation_identity),
    m_rotation(c_quaternion_identity),
    m_scale(c_scale_identity),
    m_changed(changed),
    m_apply(apply),
    m_type(TRANSFORM_PRIMITIVE)
  {
  }
    
  void setType(TransformModifierType type)
  {
    m_type = type;
  }
  TransformModifierType getType() const
  {
    return m_type;
  }
  void setTranslation(const Vector3& value)
  {
    m_translation = value;
    m_changed();
  }
  void setRotation(const Quaternion& value)
  {
    m_rotation = value;
    m_changed();
  }
  void setScale(const Vector3& value)
  {
    m_scale = value;
    m_changed();
  }
  void freezeTransform()
  {
    if(m_translation != c_translation_identity
      || m_rotation != c_rotation_identity
      || m_scale != c_scale_identity)
    {
      m_apply();
      m_translation = c_translation_identity;
      m_rotation = c_rotation_identity;
      m_scale = c_scale_identity;
      m_changed();
    }
  }
  
  /* greebo: This reverts the currently active transformation
   * by setting the scale/rotation/translation to identity and
   * calling apply()
   */
  void revertTransform()
  {
    m_translation = c_translation_identity;
    m_rotation = c_rotation_identity;
    m_scale = c_scale_identity;
    m_apply();
    m_changed();
  }
  const Vector3& getTranslation() const
  {
    return m_translation;
  }
  const Quaternion& getRotation() const
  {
    return m_rotation;
  }
  const Vector3& getScale() const
  {
    return m_scale;
  }
  Matrix4 calculateTransform() const
  {
    return matrix4_transform_for_components(getTranslation(), getRotation(), getScale());
  }
};


#endif
