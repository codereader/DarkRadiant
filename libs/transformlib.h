#pragma once

#include "Transformable.h"

#include "itransformnode.h"
#include "math/Matrix4.h"

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
