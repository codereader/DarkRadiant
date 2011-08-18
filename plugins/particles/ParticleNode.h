#pragma once

#include "scene/Node.h"
#include "iparticlenode.h"
#include "iparticles.h"

#include "RenderableParticle.h"

namespace particles
{

class ParticleNode :
	public scene::Node,
	public IParticleNode
{
private:
	RenderableParticlePtr _renderableParticle;

public:
	// Construct the node giving a renderable particle 
	ParticleNode(const RenderableParticlePtr& particle);

	IRenderableParticlePtr getParticle() const;
	const AABB& localAABB() const;
	bool isHighlighted(void) const;

	void renderSolid(RenderableCollector& collector, const VolumeTest& volume) const;

	void renderWireframe(RenderableCollector& collector, const VolumeTest& volume) const;
};
typedef boost::shared_ptr<ParticleNode> ParticleNodePtr;

} // namespace
