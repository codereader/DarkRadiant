#pragma once

#include <vector>
#include "scene/Node.h"
#include "RenderableModelSurface.h"

namespace model
{
/**
 * Common ModelNode implementation used by various model types,
 * e.g. StaticModelNode and MD5ModelNode
 */
class ModelNodeBase :
    public scene::Node
{
protected:
    // The renderable surfaces attached to the shaders
    std::vector<RenderableModelSurface::Ptr> _renderableSurfaces;

    bool _attachedToShaders;

protected:
    ModelNodeBase();

public:
    void renderHighlights(IRenderableCollector& collector, const VolumeTest& volume) override;

protected:
    void attachToShaders();
    void detachFromShaders();
    void queueRenderableUpdate();
};

}
