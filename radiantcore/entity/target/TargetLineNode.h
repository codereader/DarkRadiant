#pragma once

#include <sigc++/trackable.h>
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
class TargetLineNode final :
    public scene::Node,
    public sigc::trackable
{
private:
    EntityNode& _owner;

    RenderableTargetLines _targetLines;

public:
    TargetLineNode(EntityNode& owner);

    TargetLineNode(TargetLineNode& other);

    // Returns the type of this node
	Type getNodeType() const override;

    const AABB& localAABB() const override;

    void onPreRender(const VolumeTest& volume) override;
    void renderHighlights(IRenderableCollector& collector, const VolumeTest& volumeTest) override;
	std::size_t getHighlightFlags() override;

    void onRenderSystemChanged();

    void onInsertIntoScene(scene::IMapRootNode& root) override;
    void onRemoveFromScene(scene::IMapRootNode& root) override;

    void queueRenderableUpdate();

protected:
    void onVisibilityChanged(bool isVisibleNow) override;
    void onRenderStateChanged() override;

private:
    Vector3 getOwnerPosition() const;
};

}
