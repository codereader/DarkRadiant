#pragma once

#include "ishaders.h"
#include "modelskin.h"
#include "scene/BasicRootNode.h"
#include "TestModelSkin.h"
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
    scene::INodePtr _model; // The textured model
    scene::INodePtr _light; // The light

    std::shared_ptr<TestModelSkin> _testModelSkin;

    float _defaultCamDistanceFactor;

    wxToolBarToolBase* _testModelCubeButton;
    wxToolBarToolBase* _testModelSphereButton;

public:
    MaterialPreview(wxWindow* parent);

    virtual ~MaterialPreview();

    const MaterialPtr& getMaterial();
    void setMaterial(const MaterialPtr& material);

protected:
    bool canDrawGrid() override;
    void setupSceneGraph() override;

private:
    bool onPreRender() override;
    void prepareScene();
    void setupToolbar();
    void setupTestModel();
    void updateModelSkin();
    void onTestModelSelectionChanged(wxCommandEvent& ev);
};

}
