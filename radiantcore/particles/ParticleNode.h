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

	std::string name() const override;
	Type getNodeType() const override;

	IRenderableParticlePtr getParticle() const override;
	const AABB& localAABB() const override;
	std::size_t getHighlightFlags() override;

	void onPreRender(const VolumeTest& volume) override;
	void renderHighlights(IRenderableCollector& collector, const VolumeTest& volume) override;

	void setRenderSystem(const RenderSystemPtr& renderSystem) override;

	// ITransformNode
	Matrix4 localToParent() const override;

    void onRemoveFromScene(scene::IMapRootNode& root) override;

private:
	void update(const VolumeTest& viewVolume) const;
};
typedef std::shared_ptr<ParticleNode> ParticleNodePtr;

} // namespace
