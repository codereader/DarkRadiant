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
	Matrix4 localToParent() const
	{
		return Matrix4::getIdentity();
	}
};

namespace scene
{

/**
 * @brief Visit each Transformable child node
 *
 * @tparam Func
 * Functor object accepting a single parameter consisting of an ITransformable&.
 *
 * @param node
 * Node to start traversal from.
 *
 * @param functor
 * Functor to invoke.
 */
template<typename Func>
void forEachTransformable(const INode& node, Func functor)
{
    node.foreachNode(
        [&](const scene::INodePtr& child) -> bool
        {
            ITransformablePtr transformable = scene::node_cast<ITransformable>(child);
            if (transformable)
                functor(*transformable);

            return true;
        }
    );
}
}
