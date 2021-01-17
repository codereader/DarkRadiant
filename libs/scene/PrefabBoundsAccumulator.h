#pragma once

#include "AABBAccumulateWalker.h"
#include "ilightnode.h"

namespace scene
{

/**
 * Special AABB accumulation walker used to calculate the bounds of a
 * scene below a root node. It will ignore light and speaker volumes
 * since they are needlessly inflating the bounds.
 */
class PrefabBoundsAggregator final :
    public AABBAccumulateWalker
{
private:
    AABB _bounds;

public:
    PrefabBoundsAggregator() :
        AABBAccumulateWalker(_bounds) // point the base to the local member
    {}

    const AABB& getBounds() const
    {
        return _bounds;
    }

    bool pre(const INodePtr& node) override
    {
        // For lights we'll only sum up the small diamond AABB
        auto lightNode = Node_getLightNode(node);

        if (lightNode)
        {
            _bounds.includeAABB(lightNode->getSelectAABB());
            return false;
        }

        // TODO: Speakers

        return AABBAccumulateWalker::pre(node);
    }
};

}
