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
class IdentityTransform : 
	public TransformNode
{
public:
	/// \brief Returns the identity matrix.
	const Matrix4& localToParent() const
	{
		return Matrix4::getIdentity();
	}
};

/// \brief A transform node which stores a generic transformation matrix.
class MatrixTransform : 
	public TransformNode
{
	Matrix4 _localToParent;
public:
	MatrixTransform() : 
		_localToParent(Matrix4::getIdentity())
	{}

	Matrix4& localToParent()
	{
		return _localToParent;
	}

	/// \brief Returns the stored local->parent transform.
	const Matrix4& localToParent() const
	{
		return _localToParent;
	}
};

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

class Transformable : 
	public ITransformable
{
	Vector3 _translation;
	Quaternion _rotation;
	Vector3 _scale;

	TransformModifierType _type;
public:

	Transformable() :
		_translation(c_translation_identity),
		_rotation(c_quaternion_identity),
		_scale(c_scale_identity),
		_type(TRANSFORM_PRIMITIVE)
	{}
    
	void setType(TransformModifierType type)
	{
		_type = type;
	}

	TransformModifierType getType() const
	{
		return _type;
	}

	void setTranslation(const Vector3& value)
	{
		_translation = value;

		_onTransformationChanged();
	}

	void setRotation(const Quaternion& value)
	{
		_rotation = value;

		_onTransformationChanged();
	}

	void setScale(const Vector3& value)
	{
		_scale = value;

		_onTransformationChanged();
	}

	void freezeTransform()
	{
		if (_translation != c_translation_identity || 
			_rotation != c_rotation_identity || 
			_scale != c_scale_identity)
		{
			_applyTransformation();

			_translation = c_translation_identity;
			_rotation = c_rotation_identity;
			_scale = c_scale_identity;

			_onTransformationChanged();
		}
	}
  
	/* greebo: This reverts the currently active transformation
	* by setting the scale/rotation/translation to identity and
	* calling apply()
	*/
	void revertTransform()
	{
		_translation = c_translation_identity;
		_rotation = c_rotation_identity;
		_scale = c_scale_identity;

		_applyTransformation();

		_onTransformationChanged();
	}
  
	const Vector3& getTranslation() const
	{
		return _translation;
	}

	const Quaternion& getRotation() const
	{
		return _rotation;
	}

	const Vector3& getScale() const
	{
		return _scale;
	}

	Matrix4 calculateTransform() const
	{
		return matrix4_transform_for_components(getTranslation(), getRotation(), getScale());
	}

protected:
	/**
	 * greebo: Signal method for subclasses. This gets called
	 * as soon as anything (translation, scale, rotation) is changed.
	 *
	 * To be implemented by subclasses
	 */
	virtual void _onTransformationChanged()
	{}

	/**
	 * greebo: Signal method to be implemented by subclasses.
	 * Is invoked whenever the transformation is reverted or frozen.
	 */
	virtual void _applyTransformation()
	{}
};


#endif
