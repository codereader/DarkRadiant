#pragma once

#include "scene/Node.h"
#include "iparticlenode.h"
#include "iparticles.h"
#include "itransformnode.h"

#include "RenderableParticle.h"

namespace particles
{

/**
 * Like the model nodes this ParticleNode encapsulates a renderable particle
 * such that it can be inserted into the scenegraph as child node of an entity.
 */
class ParticleNode :
	public IParticleNode,
    public scene::Node,
	public ITransformNode // to compensate parent rotations
{
    // The actual particle system that will be rendered
	RenderableParticlePtr _renderableParticle;

	mutable Matrix4 _local2Parent;

public:
	// Construct the node giving a renderable particle 
	ParticleNode(const RenderableParticlePtr& particle);

	std::string name() const;
	Type getNodeType() const;

	IRenderableParticlePtr getParticle() const;
	const AABB& localAABB() const;
	bool isHighlighted(void) const;

	void renderSolid(RenderableCollector& collector, const VolumeTest& volume) const;
	void renderWireframe(RenderableCollector& collector, const VolumeTest& volume) const;

	void setRenderSystem(const RenderSystemPtr& renderSystem);

	// ITransformNode
	const Matrix4& localToParent() const;

private:
	void update(const VolumeTest& viewVolume) const;
};
typedef boost::shared_ptr<ParticleNode> ParticleNodePtr;

} // namespace
