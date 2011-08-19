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

	// Update the particle system before rendering
	update(volume);

	_renderableParticle->renderSolid(collector, volume, localToWorld(), _renderEntity.get());
}

void ParticleNode::renderWireframe(RenderableCollector& collector, 
								   const VolumeTest& volume) const
{
	if (!_renderableParticle) return;

	// Update the particle system before rendering
	update(volume);

	_renderableParticle->renderWireframe(collector, volume, localToWorld(), _renderEntity.get());
}

void ParticleNode::onInsertIntoScene()
{
	scene::Node::onInsertIntoScene();

	if (_renderEntity)
	{
		_renderEntity->setRequiredShaderFlags(_renderEntity->getRequiredShaderFlags() | RENDER_COLOURARRAY);
	}
}

void ParticleNode::onRemoveFromScene()
{
	scene::Node::onRemoveFromScene();

	if (_renderEntity)
	{
		_renderEntity->setRequiredShaderFlags(_renderEntity->getRequiredShaderFlags() & ~RENDER_COLOURARRAY);
	}
}

void ParticleNode::update(const VolumeTest& viewVolume) const
{
	// Get the view rotation and cancel out the translation part
	Matrix4 viewRotation = viewVolume.GetModelview();
	viewRotation.t() = Vector4(0,0,0,1);

	// Get the main direction of our parent entity
	_renderableParticle->setMainDirection(_renderEntity->getDirection());

	// Set entity colour, might be needed
	_renderableParticle->setEntityColour(Vector3(
		_renderEntity->getShaderParm(0), _renderEntity->getShaderParm(1), _renderEntity->getShaderParm(2)));

	_renderableParticle->update(GlobalRenderSystem().getTime(), 
		GlobalRenderSystem(), viewRotation);
}

} // namespace
