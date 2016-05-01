#pragma once

#include "scene/Node.h"
#include "RenderableTargetLines.h"

namespace entity
{

class EntityNode;

/**
 * Non-selectable node representing the connection line between
 * two entities, displayed as a line with an arrow in the middle. 
 * It is owned and managed by the EntityNode hosting the corresponding TargetKey.
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
    bool isHighlighted() const override;

private:
    const Vector3& getWorldPosition() const;
};

}
