#pragma once

#include "inode.h"
#include "math/AABB.h"

namespace scene
{

/**
 * greebo: This walker is used to traverse the children of
 * a given node and accumulate their values of worldAABB().
 *
 * Note: This walker's pre() method always returns false, so only
 * the first order children are visited.
 */
class AABBAccumulateWalker :
    public NodeVisitor
{
private:
    AABB& _aabb;

public:
    AABBAccumulateWalker(AABB& aabb) :
        _aabb(aabb)
    {}

    virtual bool pre(const INodePtr& node) override
    {
        _aabb.includeAABB(node->worldAABB());

        // Don't traverse the children
        return false;
    }
};

}
