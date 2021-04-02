#pragma once

#include <sigc++/connection.h>
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
    sigc::connection _materialChanged;

    scene::IMapRootNodePtr _rootNode;

    scene::INodePtr _entity; // The func_static entity
    scene::INodePtr _model; // The textured model
    scene::INodePtr _room; // The textured room
    scene::INodePtr _light; // The light

    std::shared_ptr<TestModelSkin> _testModelSkin;
    std::shared_ptr<TestModelSkin> _testRoomSkin;

    float _defaultCamDistanceFactor;

    wxToolBarToolBase* _testModelCubeButton;
    wxToolBarToolBase* _testModelSphereButton;
    wxToolBarToolBase* _testModelTilesButton;

public:
    MaterialPreview(wxWindow* parent);

    virtual ~MaterialPreview();

    const MaterialPtr& getMaterial();
    void setMaterial(const MaterialPtr& material);

    void enableFrobHighlight(bool enable);

protected:
    bool canDrawGrid() override;
    void setupSceneGraph() override;
    RenderStateFlags getRenderFlagsFill() override;

private:
    void onMaterialChanged();
    bool onPreRender() override;
    void prepareScene();
    void setupToolbar();
    void setupTestModel();
    void setupRoom();
    void updateModelSkin();
    void updateRoomSkin();
    std::string getRoomMaterial();
    void onTestModelSelectionChanged(wxCommandEvent& ev);
};

}
