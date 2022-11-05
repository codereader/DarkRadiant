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
private:
    // The renderable surfaces attached to the shaders
    std::vector<RenderableModelSurface::Ptr> _renderableSurfaces;

    bool _attachedToShaders;

    ShaderPtr _inactiveShader;

protected:
    ModelNodeBase();

public:
    scene::INode::Type getNodeType() const override;

    void onPreRender(const VolumeTest& volume) override;
    void renderHighlights(IRenderableCollector& collector, const VolumeTest& volume) override;
    std::size_t getHighlightFlags() override;

    void onInsertIntoScene(scene::IMapRootNode& root) override;
    void onRemoveFromScene(scene::IMapRootNode& root) override;

    void transformChangedLocal() override;

    void onFiltersChanged() override;

    void setRenderSystem(const RenderSystemPtr& renderSystem) override;

protected:
    // To be implemented by subclasses, this should populate the _renderableSurfaces collection
    virtual void createRenderableSurfaces() = 0;

    void emplaceRenderableSurface(RenderableModelSurface::Ptr&& surface);

    // Detaches all surfaces from their shaders and clears the _renderableSurfaces collection
    virtual void destroyRenderableSurfaces();

    void onVisibilityChanged(bool isVisibleNow) override;
    void onRenderStateChanged() override;

    void attachToShaders();
    void detachFromShaders();
    void queueRenderableUpdate();
};

}
