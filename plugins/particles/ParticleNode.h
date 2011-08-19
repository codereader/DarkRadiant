#pragma once

#include "scene/Node.h"
#include "iparticlenode.h"
#include "iparticles.h"

#include "RenderableParticle.h"

namespace particles
{

/**
 * Like the model nodes this ParticleNode encapsulates a renderable particle
 * such that it can be inserted into the scenegraph as child node of an entity.
 */
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

	// Add/remove required shader flags to the parent entity on insertion/removal
	virtual void onInsertIntoScene();
	virtual void onRemoveFromScene();

	void renderSolid(RenderableCollector& collector, const VolumeTest& volume) const;
	void renderWireframe(RenderableCollector& collector, const VolumeTest& volume) const;

private:
	void update(const VolumeTest& viewVolume) const;
};
typedef boost::shared_ptr<ParticleNode> ParticleNodePtr;

} // namespace
