#pragma once

#include "ishaders.h"
#include "scene/BasicRootNode.h"
#include "wxutil/preview/RenderPreview.h"

namespace ui
{

class MaterialPreview :
    public wxutil::RenderPreview
{
private:
    bool _sceneIsReady;

    MaterialPtr _material;

    scene::IMapRootNodePtr _rootNode;

    scene::INodePtr _entity; // The func_static entity
    scene::INodePtr _brush; // The textured brush
    scene::INodePtr _light; // The light

    float _defaultCamDistanceFactor;

public:
    MaterialPreview(wxWindow* parent);

    const MaterialPtr& getMaterial();
    void setMaterial(const MaterialPtr& material);

protected:
    bool canDrawGrid() override;
    void setupSceneGraph() override;

private:
    bool onPreRender() override;
    void prepareScene();
};

}
