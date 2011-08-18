#include "ParticleNode.h"

#include "itextstream.h"

namespace particles
{

ParticleNode::ParticleNode(const IRenderableParticlePtr& particle) :
	_renderableParticle(particle)
{}

ParticleNode::ParticleNode(const std::string& particleName) :
	_renderableParticle(GlobalParticlesManager().getRenderableParticle(particleName))
{}

void ParticleNode::renderSolid(RenderableCollector& collector, 
							   const VolumeTest& volume) const
{
	globalOutputStream() << "ParticleNode::renderSolid" << std::endl;
}

void ParticleNode::renderWireframe(RenderableCollector& collector, 
								   const VolumeTest& volume) const
{
	globalOutputStream() << "ParticleNode::renderWireframe" << std::endl;
}

} // namespace
