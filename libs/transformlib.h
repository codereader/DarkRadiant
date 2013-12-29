#pragma once

#include "Transformable.h"

#include "itransformnode.h"
#include "math/Matrix4.h"

/// \brief A transform node which has no effect.
class IdentityTransform :
	public ITransformNode
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
	public ITransformNode
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

namespace scene
{

/**
 * Visit each transformable that is child of the given node with the given functor.
 */
inline void foreachTransformable(const scene::INodePtr& node, const std::function<void(ITransformable&)>& functor)
{
	if (!node) return;

	node->foreachNode([&] (const scene::INodePtr& child)->bool
	{
		ITransformablePtr transformable = Node_getTransformable(child);

		if (transformable)
		{
			functor(*transformable);
		}

		return true;
	});
}

}
