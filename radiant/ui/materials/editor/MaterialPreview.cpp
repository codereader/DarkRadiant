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
}

MaterialPreview::MaterialPreview(wxWindow* parent) :
    RenderPreview(parent, true),
    _sceneIsReady(false),
    _defaultCamDistanceFactor(2.0f)
{
    _testModelSkin.reset(new TestModelSkin("model"));
    GlobalModelSkinCache().addNamedSkin(_testModelSkin);

    _testRoomSkin.reset(new TestModelSkin("room"));
    GlobalModelSkinCache().addNamedSkin(_testRoomSkin);

    setupToolbar();

    setViewOrigin(Vector3(1, 1, 1) * 100);
    setViewAngles(Vector3(37, 135, 0));
}

MaterialPreview::~MaterialPreview()
{
    if (_testModelSkin)
    {
        GlobalModelSkinCache().removeSkin(_testModelSkin->getName());
        _testModelSkin.reset();
    }

    if (_testRoomSkin)
    {
        GlobalModelSkinCache().removeSkin(_testRoomSkin->getName());
        _testRoomSkin.reset();
    }
}

void MaterialPreview::setupToolbar()
{
    // Add one additional toolbar for particle-related stuff
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

    toolbar->Realize();

    addToolbar(toolbar);
}

const MaterialPtr& MaterialPreview::getMaterial()
{
    return _material;
}

void MaterialPreview::updateModelSkin()
{
    // Hide the model if there's no material to preview
    _model->setFiltered(_testModelSkin->isEmpty());

    // Let the model update its remaps
    auto skinnedModel = std::dynamic_pointer_cast<SkinnedModel>(_model);

    if (skinnedModel)
    {
        skinnedModel->skinChanged(_testModelSkin->getName());
    }
}

std::string MaterialPreview::getRoomMaterial()
{
    return game::current::getValue<std::string>(GKEY_DEFAULT_ROOM_MATERIAL);
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

void MaterialPreview::updateRoomSkin()
{
    auto roomMaterial = getRoomMaterial();
    _testRoomSkin->setRemapMaterial(GlobalMaterialManager().getMaterial(roomMaterial));

    // Let the model update its remaps
    auto skinnedRoom = std::dynamic_pointer_cast<SkinnedModel>(_room);

    if (skinnedRoom)
    {
        skinnedRoom->skinChanged(_testRoomSkin->getName());
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

    if (_model)
    {
        // Assign the material to the temporary skin
        _testModelSkin->setRemapMaterial(_material);

        updateModelSkin();
    }

    if (!hadMaterial && _material)
    {
        setLightingModeEnabled(true);
        startPlayback();
    }
    else if (hadMaterial && !_material)
    {
        stopPlayback();
    }

    queueDraw();
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
        _light = GlobalEntityModule().createEntity(
            GlobalEntityClassManager().findClass(getDefaultLightDef()));

        Node_getEntity(_light)->setKeyValue("light_radius", "750 750 750");
        Node_getEntity(_light)->setKeyValue("origin", "150 150 150");

        scene::addNodeToContainer(_light, _rootNode);

        // Reset the default view, facing down to the model from diagonally above the bounding box
        double distance = _model->localAABB().getRadius() * _defaultCamDistanceFactor;

        setViewOrigin(Vector3(1, 1, 1) * distance);
        setViewAngles(Vector3(37, 135, 0));

        _sigLightChanged.emit();
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
    
    updateRoomSkin();
}

void MaterialPreview::setupTestModel()
{
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

    updateModelSkin();
}

void MaterialPreview::onTestModelSelectionChanged(wxCommandEvent& ev)
{
    setupTestModel();
    queueDraw();
}

sigc::signal<void>& MaterialPreview::signal_LightChanged()
{
    return _sigLightChanged;
}

std::string MaterialPreview::getLightClassname()
{
    return _light ? Node_getEntity(_light)->getEntityClass()->getName() : "";
}

void MaterialPreview::setLightClassname(const std::string& className)
{
    if (!_light || className.empty()) return;

    _light = changeEntityClassname(_light, className);
    _sigLightChanged.emit();
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
    _sigLightChanged.emit();
}

void MaterialPreview::resetLightColour()
{
    if (!_light) return;

    Node_getEntity(_light)->setKeyValue("_color", "");
    _sigLightChanged.emit();
}

}
