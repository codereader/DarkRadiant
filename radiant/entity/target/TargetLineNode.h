#pragma once

#include "scene/Node.h"
#include "RenderableTargetLines.h"

namespace entity
{

class EntityNode;

/**
 * Non-selectable node representing one ore more connection lines between
 * entities, displayed as a line with an arrow in the middle. 
 * It is owned and managed by the EntityNode hosting the corresponding TargetKey.
 * The rationale of inserting these lines as separate node type is to prevent
 * the lines from disappearing from the view when the targeting/targeted entities
 * get culled by the space partitioning system during rendering.
 */
class TargetLineNode :
    public scene::Node
{
private:
    AABB _aabb;

    EntityNode& _owner;

    mutable RenderableTargetLines _targetLines;

public:
    TargetLineNode(EntityNode& owner);

    TargetLineNode(TargetLineNode& other);

    // Returns the type of this node
	Type getNodeType() const override;

    const AABB& localAABB() const override;

    void renderSolid(RenderableCollector& collector, const VolumeTest& volumeTest) const override;
    void renderWireframe(RenderableCollector& collector, const VolumeTest& volumeTest) const override;
	std::size_t getHighlightFlags() override;

private:
    const Vector3& getOwnerPosition() const;
};

}
