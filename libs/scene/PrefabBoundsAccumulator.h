#pragma once

#include "ilightnode.h"
#include "ispeakernode.h"
#include "iselection.h"

namespace scene
{

/**
 * Special AABB accumulation walker used to calculate the bounds of a
 * scene below a root node. It will ignore light and speaker volumes
 * since they are needlessly inflating the bounds.
 */
class PrefabBoundsAccumulator final :
    public scene::NodeVisitor,
    public SelectionSystem::Visitor
{
private:
    mutable AABB _bounds;

public:
    PrefabBoundsAccumulator()
    {}

    const AABB& getBounds() const
    {
        return _bounds;
    }

    bool pre(const INodePtr& node) override
    {
        _bounds.includeAABB(GetNodeBounds(node));
        return false;
    }

    void visit(const scene::INodePtr& node) const override
    {
        _bounds.includeAABB(GetNodeBounds(node));
    }

    static AABB GetNodeBounds(const INodePtr& node)
    {
        // For lights we'll only sum up the small diamond AABB
        auto lightNode = Node_getLightNode(node);

        if (lightNode)
        {
            return lightNode->getSelectAABB();
        }

        // Speakers without radius
        auto speakerNode = Node_getSpeakerNode(node);

        if (speakerNode)
        {
            return speakerNode->getSpeakerAABB();
        }

        return node->worldAABB();
    }
};

}
