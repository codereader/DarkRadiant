#include "MaterialPreview.h"

#include "i18n.h"
#include "ibrush.h"
#include "ientity.h"
#include "imodelcache.h"
#include "ieclass.h"
#include "ishaders.h"
#include "string/convert.h"
#include "math/pi.h"
#include "wxutil/dialog/MessageBox.h"
#include "wxutil/Bitmap.h"
#include <wx/toolbar.h>
#include "entitylib.h"
#include "gamelib.h"
#include "scenelib.h"

namespace ui
{

namespace
{
    const char* const FUNC_STATIC_CLASS = "func_static";
    const char* const GKEY_DEFAULT_ROOM_MATERIAL = "/materialPreview/defaultRoomMaterial";
    const char* const GKEY_DEFAULT_LIGHT_DEF = "/materialPreview/defaultLightDef";

    inline bool isSkyboxMaterial(const MaterialPtr& material)
    {
        if (!material) return false;

        auto result = false;

        material->foreachLayer([&](const IShaderLayer::Ptr& layer)
        {
            if (layer->getMapType() == IShaderLayer::MapType::CameraCubeMap)
            {
                result = true;
                return false;
            }

            return true;
        });

        return result;
    }
}

MaterialPreview::MaterialPreview(wxWindow* parent) :
    RenderPreview(parent, true),
    _sceneIsReady(false),
    _roomMaterial(game::current::getValue<std::string>(GKEY_DEFAULT_ROOM_MATERIAL)),
    _defaultCamDistanceFactor(2.0f),
    _lightClassname(getDefaultLightDef())
{
    _testModelSkin = std::make_unique<TestModelSkin>("model");
    _testRoomSkin = std::make_unique<TestModelSkin>("room");

    setupToolbar();

    setStartupLightingMode(true);
    setViewOrigin(Vector3(1, 1, 1) * 100);
    setViewAngles(Vector3(37, 135, 0));
}

MaterialPreview::~MaterialPreview()
{
    _testModelSkin.reset();
    _testRoomSkin.reset();
}

void MaterialPreview::setupToolbar()
{
    // Add one additional toolbar
    wxToolBar* toolbar = new wxToolBar(_mainPanel, wxID_ANY);
    toolbar->SetToolBitmapSize(wxSize(16, 16));

    _testModelCubeButton = toolbar->AddRadioTool(wxID_ANY, "", wxutil::GetLocalBitmap("cube.png", wxART_TOOLBAR));
    _testModelCubeButton->SetShortHelp(_("Show Cube"));
    toolbar->ToggleTool(_testModelCubeButton->GetId(), true);
    
    _testModelSphereButton = toolbar->AddRadioTool(wxID_ANY, "", wxutil::GetLocalBitmap("sphere.png", wxART_TOOLBAR));
    _testModelSphereButton->SetShortHelp(_("Show Sphere"));

    _testModelTilesButton = toolbar->AddRadioTool(wxID_ANY, "", wxutil::GetLocalBitmap("tiles.png", wxART_TOOLBAR));
    _testModelTilesButton->SetShortHelp(_("Show Tiles"));

    toolbar->Bind(wxEVT_TOOL, &MaterialPreview::onTestModelSelectionChanged, this, _testModelCubeButton->GetId());
    toolbar->Bind(wxEVT_TOOL, &MaterialPreview::onTestModelSelectionChanged, this, _testModelSphereButton->GetId());
    toolbar->Bind(wxEVT_TOOL, &MaterialPreview::onTestModelSelectionChanged, this, _testModelTilesButton->GetId());

    toolbar->AddSeparator();

    _swapBackgroundMaterialButton = toolbar->AddTool(wxID_ANY, "Swap", wxutil::GetLocalBitmap("swap_background.png", wxART_TOOLBAR));
    _swapBackgroundMaterialButton->SetToggle(true);
    toolbar->ToggleTool(_swapBackgroundMaterialButton->GetId(), false);
    _swapBackgroundMaterialButton->SetShortHelp(_("Swap Background and Foreground Materials"));

    toolbar->Bind(wxEVT_TOOL, &MaterialPreview::onSwapBackgroundMaterialChanged, this, _swapBackgroundMaterialButton->GetId());

    toolbar->Realize();

    addToolbar(toolbar);
}

const MaterialPtr& MaterialPreview::getMaterial()
{
    return _material;
}

void MaterialPreview::updateModelSkin(const std::string& material)
{
    // Assign the material to the temporary skin
    _testModelSkin->setRemapMaterial(GlobalMaterialManager().getMaterial(material));

    if (!_model) return;

    // Hide the model if there's no material to preview
    _model->setFiltered(_testModelSkin->isEmpty());

    // Let the model update its remaps
    auto skinnedModel = std::dynamic_pointer_cast<SkinnedModel>(_model);

    if (skinnedModel)
    {
        skinnedModel->skinChanged(_testModelSkin->getSkinName());
    }
}

const std::string& MaterialPreview::getRoomMaterial()
{
    return _roomMaterial;
}

void MaterialPreview::setRoomMaterial(const std::string& material)
{
    _roomMaterial = material;
    updateSceneMaterials();

    signal_SceneChanged().emit();
}

MaterialPreview::TestModel MaterialPreview::getTestModelType()
{
    if (_testModelCubeButton->IsToggled()) return TestModel::Cube;
    if (_testModelSphereButton->IsToggled()) return TestModel::Sphere;
    if (_testModelTilesButton->IsToggled()) return TestModel::Tiles;

    return TestModel::Cube;
}

void MaterialPreview::setTestModelType(TestModel type)
{
    _testModelCubeButton->GetToolBar()->ToggleTool(_testModelCubeButton->GetId(), type == TestModel::Cube);
    _testModelSphereButton->GetToolBar()->ToggleTool(_testModelSphereButton->GetId(), type == TestModel::Sphere);
    _testModelTilesButton->GetToolBar()->ToggleTool(_testModelTilesButton->GetId(), type == TestModel::Tiles);

    setupTestModel();
}

std::string MaterialPreview::GetTestModelTypeName(TestModel type)
{
    switch (type)
    {
    case TestModel::Cube: return "Cube";
    case TestModel::Sphere: return "Sphere";
    case TestModel::Tiles: return "Tiles";
    default: throw std::invalid_argument("Unknown preview model type");
    }
}

MaterialPreview::TestModel MaterialPreview::GetTestModelType(const std::string& typeName)
{
    if (typeName == "Cube") return TestModel::Cube;
    if (typeName == "Sphere") return TestModel::Sphere;
    if (typeName == "Tiles") return TestModel::Tiles;

    return TestModel::Cube; // in case bad serialization data reaches this point
}

std::string MaterialPreview::getDefaultLightDef()
{
    auto className = game::current::getValue<std::string>(GKEY_DEFAULT_LIGHT_DEF);

    if (className.empty() || !GlobalEntityClassManager().findClass(className))
    {
        className = "light";
    }

    return className;
}

void MaterialPreview::updateRoomSkin(const std::string& roomMaterial)
{
    _testRoomSkin->setRemapMaterial(GlobalMaterialManager().getMaterial(roomMaterial));

    // Let the model update its remaps
    auto skinnedRoom = std::dynamic_pointer_cast<SkinnedModel>(_room);

    if (skinnedRoom)
    {
        skinnedRoom->skinChanged(_testRoomSkin->getSkinName());
    }
}

void MaterialPreview::setMaterial(const MaterialPtr& material)
{
    bool hadMaterial = _material != nullptr;

    _materialChanged.disconnect();

    _material = material;

    if (_material)
    {
        _materialChanged = _material->sig_materialChanged().connect(
            sigc::mem_fun(this, &MaterialPreview::onMaterialChanged));
    }

    _sceneIsReady = false;

    // Detect whether we should apply this material to the background
    _swapBackgroundMaterial = isSkyboxMaterial(_material);
    _swapBackgroundMaterialButton->GetToolBar()->ToggleTool(_swapBackgroundMaterialButton->GetId(), _swapBackgroundMaterial);

    updateSceneMaterials();

    if (!hadMaterial && _material)
    {
        startPlayback();
    }
    else if (hadMaterial && !_material)
    {
        stopPlayback();
    }

    queueDraw();
}

void MaterialPreview::updateSceneMaterials()
{
    auto foregroundMaterial = _material ? _material->getName() : _roomMaterial;
    auto backgroundMaterial = _roomMaterial;

    if (_swapBackgroundMaterial)
    {
        std::swap(foregroundMaterial, backgroundMaterial);
    }

    updateModelSkin(foregroundMaterial);
    updateRoomSkin(backgroundMaterial);
}

void MaterialPreview::enableFrobHighlight(bool enable)
{
    if (!_entity) return;

    Node_getEntity(_entity)->setKeyValue("shaderParm11", enable ? "1" : "0");
    queueDraw();
}

void MaterialPreview::onMaterialChanged()
{
    queueDraw();
}

bool MaterialPreview::onPreRender()
{
    if (!_sceneIsReady)
    {
        prepareScene();
    }

    // Update the rotation of the func_static
    if (_model)
    {
        auto time = _renderSystem->getTime();

        // one full rotation per 10 seconds
        auto newAngle = 2 * math::PI * time / 10000;

        Node_getEntity(_entity)->setKeyValue("angle", string::to_string(radians_to_degrees(newAngle)));
    }

    return RenderPreview::onPreRender();
}

void MaterialPreview::prepareScene()
{
    _sceneIsReady = true;
}

bool MaterialPreview::canDrawGrid()
{
    return false;
}

RenderStateFlags MaterialPreview::getRenderFlagsFill()
{
    return RenderPreview::getRenderFlagsFill() | RENDER_DEPTHWRITE | RENDER_DEPTHTEST;
}

void MaterialPreview::setupSceneGraph()
{
    RenderPreview::setupSceneGraph();

    try
    {
        _rootNode = std::make_shared<scene::BasicRootNode>();

        setupRoom();

        _entity = GlobalEntityModule().createEntity(
            GlobalEntityClassManager().findClass(FUNC_STATIC_CLASS));

        // Make sure the shaderParm11 spawnarg is present
        Node_getEntity(_entity)->setKeyValue("shaderParm11", "0");

        _rootNode->addChildNode(_entity);

        setupTestModel();

        getScene()->setRoot(_rootNode);

        // Set up the light
        _light = GlobalEntityModule().createEntity(GlobalEntityClassManager().findClass(_lightClassname));

        Node_getEntity(_light)->setKeyValue("light_radius", "750 750 750");
        Node_getEntity(_light)->setKeyValue("origin", "150 150 150");

        scene::addNodeToContainer(_light, _rootNode);

        // Initialise the view origin and angles, if thery're still empty
        if (getViewOrigin() == Vector3(0,0,0))
        {
            // Reset the default view, facing down to the model from diagonally above the bounding box
            double distance = _model->localAABB().getRadius() * _defaultCamDistanceFactor;

            setViewOrigin(Vector3(1, 1, 1) * distance);
            setViewAngles(Vector3(37, 135, 0));
        }

        signal_SceneChanged().emit();
    }
    catch (std::runtime_error&)
    {
        wxutil::Messagebox::ShowError(fmt::format(_("Unable to setup the preview,\n"
            "could not find the entity class {0}"), FUNC_STATIC_CLASS));
    }
}

void MaterialPreview::setupRoom()
{
    _room = GlobalModelCache().getModelNodeForStaticResource("preview/room_cuboid.ase");

    auto roomEntity = GlobalEntityModule().createEntity(
        GlobalEntityClassManager().findClass(FUNC_STATIC_CLASS));

    _rootNode->addChildNode(roomEntity);

    roomEntity->addChildNode(_room);
    
    updateRoomSkin(getRoomMaterial());
}

void MaterialPreview::setupTestModel()
{
    if (!_entity) return; // too early

    if (_entity && _model)
    {
        scene::removeNodeFromParent(_model);
        _model.reset();
    }

    // Load the pre-defined model from the resources path
    if (_testModelCubeButton->IsToggled())
    {
        _model = GlobalModelCache().getModelNodeForStaticResource("preview/cube.ase");
    }
    else if (_testModelSphereButton->IsToggled()) // sphere
    {
        _model = GlobalModelCache().getModelNodeForStaticResource("preview/sphere.ase");
    }
    else // Tiles
    {
        _model = GlobalModelCache().getModelNodeForStaticResource("preview/tiles.ase");
    }

    // The test model is a child of this entity
    scene::addNodeToContainer(_model, _entity);

    updateSceneMaterials();
}

void MaterialPreview::onTestModelSelectionChanged(wxCommandEvent& ev)
{
    if (ev.GetId() == _testModelCubeButton->GetId())
    {
        setTestModelType(TestModel::Cube);
    }
    else if (ev.GetId() == _testModelSphereButton->GetId())
    {
        setTestModelType(TestModel::Sphere);
    }
    else if (ev.GetId() == _testModelTilesButton->GetId())
    {
        setTestModelType(TestModel::Tiles);
    }

    queueDraw();
}

void MaterialPreview::onSwapBackgroundMaterialChanged(wxCommandEvent& ev)
{
    _swapBackgroundMaterial = ev.IsChecked();
    updateSceneMaterials();
    queueDraw();
}

sigc::signal<void>& MaterialPreview::signal_SceneChanged()
{
    return _sigSceneChanged;
}

std::string MaterialPreview::getLightClassname()
{
    return _lightClassname;
}

void MaterialPreview::setLightClassname(const std::string& className)
{
    // Store this value locally, even if we don't have any light to adjust yet
    _lightClassname = className;

    if (!_light || _lightClassname.empty()) return;

    _light = changeEntityClassname(_light, _lightClassname);
    signal_SceneChanged().emit();
}

Vector3 MaterialPreview::getLightColour()
{
    if (!_light) return Vector3(0,0,0);
    
    auto value = Node_getEntity(_light)->getKeyValue("_color");
    
    if (value.empty())
    {
        value = "1 1 1";
    }

    return string::convert<Vector3>(value, Vector3(0,0,0));
}

void MaterialPreview::setLightColour(const Vector3& colour)
{
    if (!_light) return;

    Node_getEntity(_light)->setKeyValue("_color", string::to_string(colour));
    signal_SceneChanged().emit();
}

void MaterialPreview::resetLightColour()
{
    if (!_light) return;

    Node_getEntity(_light)->setKeyValue("_color", "");
    signal_SceneChanged().emit();
}

}
