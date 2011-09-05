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
	public scene::Node,
	public IParticleNode,
	public TransformNode // to compensate parent rotations
{
private:
	RenderableParticlePtr _renderableParticle;

	mutable Matrix4 _local2Parent;

public:
	// Construct the node giving a renderable particle 
	ParticleNode(const RenderableParticlePtr& particle);

	IRenderableParticlePtr getParticle() const;
	const AABB& localAABB() const;
	bool isHighlighted(void) const;

	// Add/remove required shader flags to the parent entity on insertion/removal
	virtual void onInsertIntoScene();
	virtual void onRemoveFromScene();

	void renderSolid(RenderableCollector& collector, const VolumeTest& volume) const;
	void renderWireframe(RenderableCollector& collector, const VolumeTest& volume) const;

	void setRenderSystem(const RenderSystemPtr& renderSystem)
	{
		// TODO
	}

	// TransformNode
	const Matrix4& localToParent() const;

private:
	void update(const VolumeTest& viewVolume) const;
};
typedef boost::shared_ptr<ParticleNode> ParticleNodePtr;

} // namespace
