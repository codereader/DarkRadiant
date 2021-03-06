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

    inline scene::INodePtr createCubicBrush(const scene::INodePtr& parent,
        const Vector3& origin = Vector3(0, 0, 0),
        const std::string& material = "_default")
    {
        auto brushNode = GlobalBrushCreator().createBrush();
        parent->addChildNode(brushNode);

        auto& brush = *Node_getIBrush(brushNode);

        auto translation = Matrix4::getTranslation(origin);
        brush.addFace(Plane3(+1, 0, 0, 64).transform(translation));
        brush.addFace(Plane3(-1, 0, 0, 64).transform(translation));
        brush.addFace(Plane3(0, +1, 0, 64).transform(translation));
        brush.addFace(Plane3(0, -1, 0, 64).transform(translation));
        brush.addFace(Plane3(0, 0, +1, 64).transform(translation));
        brush.addFace(Plane3(0, 0, -1, 64).transform(translation));

        brush.setShader(material);

        brush.evaluateBRep();

        return brushNode;
    }
}

MaterialPreview::MaterialPreview(wxWindow* parent) :
    RenderPreview(parent, true),
    _sceneIsReady(false),
    _defaultCamDistanceFactor(1.5f)
{}

const MaterialPtr& MaterialPreview::getMaterial()
{
    return _material;
}

void MaterialPreview::setMaterial(const MaterialPtr& material)
{
    _material = material;
    _sceneIsReady = false;

    if (_brush)
    {
        auto& brush = *Node_getIBrush(_brush);
        for (auto i = 0; i < brush.getNumFaces(); ++i)
        {
            brush.getFace(i).setShader(_material ? _material->getName() : "");
            brush.getFace(i).fitTexture(1, 1);
        }
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
    if (_brush)
    {
        // angle change is constant over time, one full rotation per 10 seconds
        auto newAngle = 2 * c_pi * MSEC_PER_FRAME / 10000;
        auto rotation = Quaternion::createForAxisAngle(Vector3(0, 0, 1), newAngle);

        auto transformable = Node_getTransformable(_brush);

        transformable->setRotation(rotation);
        transformable->freezeTransform();
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

        // Set up a brush
        _brush = createCubicBrush(_entity);

        getScene()->setRoot(_rootNode);

        // Set up the light
        _light = GlobalEntityModule().createEntity(
            GlobalEntityClassManager().findClass("light"));

        Node_getEntity(_light)->setKeyValue("light_radius", "600 600 600");
        Node_getEntity(_light)->setKeyValue("origin", "250 250 250");

        _rootNode->addChildNode(_light);

        // Reset the default view, facing down to the model from diagonally above the bounding box
        double distance = _brush->localAABB().getRadius() * _defaultCamDistanceFactor;

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
