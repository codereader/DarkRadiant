#include "ParticleNode.h"

#include "ivolumetest.h"

namespace particles
{

ParticleNode::ParticleNode(const RenderableParticlePtr& particle) :
	_renderableParticle(particle)
{}

IRenderableParticlePtr ParticleNode::getParticle() const
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

	Matrix4 modelView = volume.GetModelview();
	modelView.t() = Vector4(0,0,0,1);

	_renderableParticle->update(GlobalRenderSystem().getTime(), 
		GlobalRenderSystem(), modelView);

	_renderableParticle->renderSolid(collector, volume, localToWorld());
}

void ParticleNode::renderWireframe(RenderableCollector& collector, 
								   const VolumeTest& volume) const
{
	if (!_renderableParticle) return;

	Matrix4 modelView = volume.GetModelview();
	modelView.t() = Vector4(0,0,0,1);

	_renderableParticle->update(GlobalRenderSystem().getTime(), 
		GlobalRenderSystem(), modelView);

	_renderableParticle->renderWireframe(collector, volume, localToWorld());
}

} // namespace
