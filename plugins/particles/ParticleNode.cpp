#include "ParticleNode.h"

namespace particles
{

ParticleNode::ParticleNode(const RenderableParticlePtr& particle) :
	_renderableParticle(particle)
{}

const IRenderableParticlePtr& ParticleNode::getParticle() const
{
	return _renderableParticle;
}

const AABB& ParticleNode::localAABB() const
{
	return _renderableParticle->getBounds();
}

bool ParticleNode::isHighlighted(void) const
{
	return false;
}

void ParticleNode::renderSolid(RenderableCollector& collector, 
							   const VolumeTest& volume) const
{
	if (!_renderableParticle) return;

	_renderableParticle->update(GlobalRenderSystem().getTime(), 
		GlobalRenderSystem(), Matrix4::getIdentity());

	_renderableParticle->renderSolid(collector, volume, localToWorld());
}

void ParticleNode::renderWireframe(RenderableCollector& collector, 
								   const VolumeTest& volume) const
{
	if (!_renderableParticle) return;

	_renderableParticle->update(GlobalRenderSystem().getTime(), 
		GlobalRenderSystem(), Matrix4::getIdentity());

	_renderableParticle->renderWireframe(collector, volume, localToWorld());
}

} // namespace
