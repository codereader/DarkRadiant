#include "EntityPreview.h"

#include "i18n.h"
#include "ieclass.h"
#include "ifilter.h"
#include "scene/BasicRootNode.h"

#include "../dialog/MessageBox.h"
#include "scene/PrefabBoundsAccumulator.h"
#include "string/convert.h"

namespace wxutil
{

EntityPreview::EntityPreview(wxWindow* parent) :
    RenderPreview(parent, false),
    _sceneIsReady(false),
    _defaultCamDistanceFactor(2.8f)
{}

void EntityPreview::setDefaultCamDistanceFactor(float factor)
{
    _defaultCamDistanceFactor = factor;
}

const IEntityNodePtr& EntityPreview::getEntity()
{
    return _entity;
}

void EntityPreview::setEntity(const IEntityNodePtr& entity)
{
    if (_entity == entity) return;

    if (_entity)
    {
        // Use getScene to access the root node to ensure it's created
        getScene()->root()->removeChildNode(_entity);
    }

    _entity = entity;

    if (_entity)
    {
        getScene()->root()->addChildNode(_entity);

        // Remember the entity bounds including children, but without light or speaker radii
        scene::PrefabBoundsAccumulator accumulator;
        accumulator.visit(_entity);

        _entity->foreachNode([&](const scene::INodePtr& node)
        {
            accumulator.visit(node);
            return true;
        });

        _untransformedEntityBounds = accumulator.getBounds();
    }
    else
    {
        _untransformedEntityBounds = AABB({ 0,0,0 }, { 64,64,64 });
    }

    queueSceneUpdate();
}

void EntityPreview::setupSceneGraph()
{
    RenderPreview::setupSceneGraph();

    try
    {
        _rootNode = std::make_shared<scene::BasicRootNode>();

        // This entity is acting as our root node in the scene
        getScene()->setRoot(_rootNode);

        // Set up the light
        _light = GlobalEntityModule().createEntity(
            GlobalEntityClassManager().findClass("light"));

        Node_getEntity(_light)->setKeyValue("light_radius", "600 600 600");
        Node_getEntity(_light)->setKeyValue("origin", "0 0 300");

        _rootNode->addChildNode(_light);
    }
    catch (std::runtime_error&)
    {
        Messagebox::ShowError(_("Unable to setup the preview,\ncould not find the entity class 'light'"));
    }
}

AABB EntityPreview::getSceneBounds()
{
    return _untransformedEntityBounds;
}

void EntityPreview::prepareScene()
{
    if (_sceneIsReady) return;

    // Clear the flag
    _sceneIsReady = true;

    // Reset the model rotation
    resetModelRotation();

    if (_entity)
    {
        // Reset the default view, facing down to the model from diagonally above the bounding box
        double distance = getSceneBounds().getRadius() * _defaultCamDistanceFactor;

        setViewOrigin(Vector3(1, 1, 1) * distance);
        setViewAngles(Vector3(34, 135, 0));
    }

    // Trigger an initial update of the subgraph
    GlobalFilterSystem().updateSubgraph(getScene()->root());
}

void EntityPreview::queueSceneUpdate()
{
    _sceneIsReady = false;
    queueDraw();
}

bool EntityPreview::onPreRender()
{
    if (!_sceneIsReady)
    {
        prepareScene();
    }

    if (_light)
    {
        Vector3 lightOrigin = _viewOrigin + Vector3(0, 0, 20);

        // Position the light just above the camera
        Node_getEntity(_light)->setKeyValue("origin", string::to_string(lightOrigin));

        // Let the light encompass the object
        float radius = (getSceneBounds().getOrigin() - lightOrigin).getLength() * 2.0f;
        radius = std::max(radius, 200.f);

        std::ostringstream value;
        value << radius << ' ' << radius << ' ' << radius;

        Node_getEntity(_light)->setKeyValue("light_radius", value.str());

        Node_getEntity(_light)->setKeyValue("_color", "0.6 0.6 0.6");
    }

    return _entity != nullptr;
}

void EntityPreview::onModelRotationChanged()
{
    if (_entity)
    {
        // Update the model rotation on the entity
        std::ostringstream value;
        value << _modelRotation.xx() << ' '
            << _modelRotation.xy() << ' '
            << _modelRotation.xz() << ' '
            << _modelRotation.yx() << ' '
            << _modelRotation.yy() << ' '
            << _modelRotation.yz() << ' '
            << _modelRotation.zx() << ' '
            << _modelRotation.zy() << ' '
            << _modelRotation.zz();

        Node_getEntity(_entity)->setKeyValue("rotation", value.str());
    }
}

RenderStateFlags EntityPreview::getRenderFlagsFill()
{
    return RenderPreview::getRenderFlagsFill() | RENDER_DEPTHWRITE | RENDER_DEPTHTEST;
}

}
