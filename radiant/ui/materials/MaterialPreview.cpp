#include "MaterialPreview.h"

#include "ibrush.h"
#include "ientity.h"
#include "ieclass.h"
#include "wxutil/dialog/MessageBox.h"

namespace ui
{

namespace
{
    const char* const FUNC_STATIC_CLASS = "func_static";
}

MaterialPreview::MaterialPreview(wxWindow* parent) :
    RenderPreview(parent, true),
    _sceneIsReady(false),
    _defaultCamDistanceFactor(1.5f)
{
    _testModelSkin.reset(new TestModelSkin);
    GlobalModelSkinCache().addNamedSkin(_testModelSkin);
}

MaterialPreview::~MaterialPreview()
{
    if (_testModelSkin)
    {
        GlobalModelSkinCache().removeSkin(_testModelSkin->getName());
        _testModelSkin.reset();
    }
}

const MaterialPtr& MaterialPreview::getMaterial()
{
    return _material;
}

void MaterialPreview::setMaterial(const MaterialPtr& material)
{
    bool hadMaterial = _material != nullptr;

    _material = material;
    _sceneIsReady = false;

    if (_model)
    {
        // Assign the material to the temporary skin
        _testModelSkin->setRemapMaterial(_material);
        
        // Let the model update its remaps
        auto skinnedModel = std::dynamic_pointer_cast<SkinnedModel>(_model);

        if (skinnedModel)
        {
            skinnedModel->skinChanged(_testModelSkin->getName());
        }
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

bool MaterialPreview::onPreRender()
{
    if (!_sceneIsReady)
    {
        prepareScene();
    }

    // Update the rotation of the func_static
    if (_model)
    {
#if 0
        auto time = _renderSystem->getTime();

        // one full rotation per 10 seconds
        auto newAngle = 2 * c_pi * time / 10000;
        auto rotation = Quaternion::createForAxisAngle(Vector3(0, 0, 1), newAngle);

        const auto& planes =
        {
            Plane3(+1, 0, 0, 64),
            Plane3(-1, 0, 0, 64),
            Plane3(0, +1, 0, 64),
            Plane3(0, -1, 0, 64),
            Plane3(0, 0, +1, 64),
            Plane3(0, 0, -1, 64),
        };

        auto& brush = *Node_getIBrush(_brush);
        brush.clear();

        for (const auto& plane : planes)
        {
            brush.addFace(plane);
        }

        brush.evaluateBRep();

        for (auto i = 0; i < brush.getNumFaces(); ++i)
        {
            brush.getFace(i).setShader(_material ? _material->getName() : "");
            brush.getFace(i).fitTexture(1, 1);
        }

        auto transformable = Node_getTransformable(_brush);

        transformable->setRotation(rotation);
        transformable->freezeTransform();
#endif
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

void MaterialPreview::setupSceneGraph()
{
    RenderPreview::setupSceneGraph();

    try
    {
        _rootNode = std::make_shared<scene::BasicRootNode>();

        _entity = GlobalEntityModule().createEntity(
            GlobalEntityClassManager().findClass(FUNC_STATIC_CLASS));

        _rootNode->addChildNode(_entity);

        // Load the pre-defined model from the resources path
        _model = GlobalModelCache().getModelNodeForStaticResource("preview/sphere.ase");
        
        // The test model is a child of this entity
        _entity->addChildNode(_model);

        getScene()->setRoot(_rootNode);

        // Set up the light
        _light = GlobalEntityModule().createEntity(
            GlobalEntityClassManager().findClass("light"));

        Node_getEntity(_light)->setKeyValue("light_radius", "600 600 600");
        Node_getEntity(_light)->setKeyValue("origin", "250 250 250");

        _rootNode->addChildNode(_light);

        // Reset the default view, facing down to the model from diagonally above the bounding box
        double distance = _model->localAABB().getRadius() * _defaultCamDistanceFactor;

        setViewOrigin(Vector3(1, 1, 1) * distance);
        setViewAngles(Vector3(34, 135, 0));
    }
    catch (std::runtime_error&)
    {
        wxutil::Messagebox::ShowError(fmt::format(_("Unable to setup the preview,\n"
            "could not find the entity class {0}"), FUNC_STATIC_CLASS));
    }
}

}
