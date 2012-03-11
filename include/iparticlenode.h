#pragma once

#include "inode.h"
#include <boost/shared_ptr.hpp>

namespace particles
{

class IRenderableParticle;
typedef boost::shared_ptr<IRenderableParticle> IRenderableParticlePtr;

/// Interface for a Node containing a particle system
class IParticleNode: public virtual scene::INode
{
public:
    // Get the reference to the render particle this node is containing
    virtual IRenderableParticlePtr getParticle() const = 0;
};
typedef boost::shared_ptr<IParticleNode> IParticleNodePtr;

/// Test if a node is a particle node
inline bool isParticleNode(const scene::INodePtr& node)
{
    return (dynamic_cast<IParticleNode*>(node.get()) != NULL);
}

} // namespace
