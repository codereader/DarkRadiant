#pragma once

#include <memory>
#include "inode.h"
#include "math/AABB.h"

/**
 * Speaker entity node interface, allowing to 
 * get the bounds without the speaker radius part.
 */
class ISpeakerNode
{
public:
    virtual ~ISpeakerNode() {}

    /**
     * Get the AABB of the speaker box, without sound radius representation.
     * (Use worldAABB() to get the full bounds.)
     */
    virtual AABB getSpeakerAABB() const = 0;
};

inline std::shared_ptr<ISpeakerNode> Node_getSpeakerNode(const scene::INodePtr& node)
{
    return std::dynamic_pointer_cast<ISpeakerNode>(node);
}
