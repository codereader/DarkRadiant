#include "ParticleNode.h"

#include "ivolumetest.h"
#include "itextstream.h"

namespace particles
{

ParticleNode::ParticleNode(const RenderableParticlePtr& particle) :
	_renderableParticle(particle),
	_local2Parent(Matrix4::getIdentity())
{}

std::string ParticleNode::name() const
{
	return "particle";
}

scene::INode::Type ParticleNode::getNodeType() const
{
	return Type::Particle;
}

IRenderableParticlePtr ParticleNode::getParticle() const
{
	return _renderableParticle;
}

const AABB& ParticleNode::localAABB() const
{
	return _renderableParticle->getBounds();
}

std::size_t ParticleNode::getHighlightFlags()
{
	return Highlight::NoHighlight;
}

Matrix4 ParticleNode::localToParent() const
{
	scene::INodePtr parent = getParent();

	if (parent == NULL)
	{
		_local2Parent = Matrix4::getIdentity();
	}
	else
	{
		_local2Parent = parent->localToWorld();

		// compensate the parent rotation only
		_local2Parent.tx() = 0;
		_local2Parent.ty() = 0;
		_local2Parent.tz() = 0;

		_local2Parent.invert();
	}

	return _local2Parent;
}

void ParticleNode::onPreRender(const VolumeTest& volume)
{
    if (!_renderableParticle) return;

    // Update the particle system before rendering
    update(volume);
}

void ParticleNode::renderSolid(IRenderableCollector& collector,
							   const VolumeTest& volume) const
{
#if 0
	if (!_renderableParticle) return;

	// Update the particle system before rendering
	update(volume);

	_renderableParticle->renderSolid(collector, volume, localToWorld(), _renderEntity);
#endif
}

void ParticleNode::renderWireframe(IRenderableCollector& collector,
								   const VolumeTest& volume) const
{
	// greebo: For now, don't draw particles in ortho views they are too distracting
#if 0
	if (!_renderableParticle) return;

	// Update the particle system before rendering
	update(volume);

	_renderableParticle->renderWireframe(collector, volume, localToWorld(), _renderEntity.get());
#endif
}

void ParticleNode::renderHighlights(IRenderableCollector& collector, const VolumeTest& volume)
{
#if 0
    if (!collector.supportsFullMaterials()) return;

    renderSolid(collector, volume);
#endif
}

void ParticleNode::setRenderSystem(const RenderSystemPtr& renderSystem)
{
	Node::setRenderSystem(renderSystem);

	_renderableParticle->setRenderSystem(renderSystem);
}

void ParticleNode::update(const VolumeTest& viewVolume) const
{
	// Get the view rotation and cancel out the translation part
	Matrix4 viewRotation = viewVolume.GetModelview();
	viewRotation.tx() = 0;
	viewRotation.ty() = 0;
	viewRotation.tz() = 0;
	viewRotation.tw() = 1;

	// Get the main direction of our parent entity
	_renderableParticle->setMainDirection(_renderEntity->getDirection());

	// Set entity colour, might be needed
	_renderableParticle->setEntityColour(Vector3(
		_renderEntity->getShaderParm(0), _renderEntity->getShaderParm(1), _renderEntity->getShaderParm(2)));

	_renderableParticle->update(viewRotation, localToWorld());
}

} // namespace
