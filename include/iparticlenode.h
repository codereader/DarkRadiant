#pragma once

#include "inode.h"
#include <boost/shared_ptr.hpp>

namespace particles
{

class IRenderableParticle;
typedef boost::shared_ptr<IRenderableParticle> IRenderableParticlePtr;

class IParticleNode
{
public:
	// Get the reference to the render particle this node is containing
	virtual const IRenderableParticlePtr& getParticle() const = 0;
};
typedef boost::shared_ptr<IParticleNode> IParticleNodePtr;

} // namespace
