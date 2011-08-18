#pragma once

#include "scene/Node.h"
#include "iparticlenode.h"
#include "iparticles.h"

namespace particles
{

class ParticleNode :
	public scene::Node,
	public IParticleNode
{
private:
	IRenderableParticlePtr _renderableParticle;

public:
	// Construct the node giving a renderable particle 
	ParticleNode(const IRenderableParticlePtr& particle);

	// Construct the node using a particle def name
	ParticleNode(const std::string& particleName);

	const IRenderableParticlePtr& getParticle() const
	{
		return _renderableParticle;
	}

	const AABB& localAABB() const
	{
		return _renderableParticle->getBounds();
	}

	bool isHighlighted(void) const
	{
		return false;
	}

	void renderSolid(RenderableCollector& collector, const VolumeTest& volume) const;

	void renderWireframe(RenderableCollector& collector, const VolumeTest& volume) const;
};
typedef boost::shared_ptr<ParticleNode> ParticleNodePtr;

} // namespace
