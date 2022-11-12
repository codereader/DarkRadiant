#include "ModelPreview.h"
#include "../GLWidget.h"

#include "ifilter.h"
#include "imodelcache.h"
#include "i18n.h"
#include "ieclass.h"
#include "math/AABB.h"
#include "modelskin.h"
#include "entitylib.h"
#include "scenelib.h"
#include "scene/Node.h"
#include "scene/BasicRootNode.h"
#include "wxutil/dialog/MessageBox.h"
#include "string/convert.h"
#include <sstream>

namespace wxutil
{

/* CONSTANTS */

namespace
{
	constexpr const char* const FUNC_STATIC_CLASS = "func_static";
}

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
        _rootNode->removeChildNode(_entity);
    }

    _entity = entity;

    if (_entity)
    {
        _rootNode->addChildNode(_entity);
    }
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
        wxutil::Messagebox::ShowError(fmt::format(_("Unable to setup the preview,\n"
			"could not find the entity class {0}"), FUNC_STATIC_CLASS));
    }
}

AABB EntityPreview::getSceneBounds()
{
	if (!_entity)
	{
		return RenderPreview::getSceneBounds();
	}

	return _entity->localAABB();
}

void EntityPreview::prepareScene()
{
	// Clear the flag
	_sceneIsReady = true;
}

void EntityPreview::queueSceneUpdate()
{
    _sceneIsReady = false;
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

ModelPreview::ModelPreview(wxWindow* parent) :
    EntityPreview(parent)
{}

const std::string& ModelPreview::getModel() const
{
    return _model;
}

const std::string& ModelPreview::getSkin() const
{
    return _skin;
}

void ModelPreview::setModel(const std::string& model)
{
    // Remember the name and mark the scene as "not ready"
    _model = model;
    queueSceneUpdate();

    if (!_model.empty())
    {
        // Reset time if the model has changed
        if (_model != _lastModel)
        {
            // Reset preview time
            stopPlayback();
        }

        // Redraw
        queueDraw();
    }
    else
    {
        stopPlayback();
    }
}

void ModelPreview::setSkin(const std::string& skin) {

    _skin = skin;
    queueSceneUpdate();

    // Redraw
    queueDraw();
}

void ModelPreview::setupSceneGraph()
{
    EntityPreview::setupSceneGraph();

    // Add a hidden func_static as preview entity
    auto entity = GlobalEntityModule().createEntity(
        GlobalEntityClassManager().findClass(FUNC_STATIC_CLASS));

    setEntity(entity);

    entity->enable(scene::Node::eHidden);
}

void ModelPreview::prepareScene()
{
    EntityPreview::prepareScene();

    // If the model name is empty, release the model
    if (_model.empty())
    {
        if (_modelNode)
        {
            getEntity()->removeChildNode(_modelNode);
        }

        _modelNode.reset();

        // Emit the signal carrying an empty pointer
        _modelLoadedSignal.emit(model::ModelNodePtr());
        return;
    }

    if (_modelNode)
    {
        getEntity()->removeChildNode(_modelNode);
    }

    // Check if the model key is pointing to a def
    auto modelDef = GlobalEntityClassManager().findModel(_model);

    _modelNode = GlobalModelCache().getModelNode(modelDef ? modelDef->getMesh() : _model);

    if (_modelNode)
    {
        getEntity()->addChildNode(_modelNode);

        // Apply the skin
        model::ModelNodePtr model = Node_getModel(_modelNode);

        if (model)
        {
            auto skin = GlobalModelSkinCache().findSkin(_skin);
            model->getIModel().applySkin(skin);
        }

        // Apply the idle pose if possible
        if (modelDef)
        {
            scene::applyIdlePose(_modelNode, modelDef);
        }

        // Trigger an initial update of the subgraph
        GlobalFilterSystem().updateSubgraph(getScene()->root());

        if (_lastModel != _model)
        {
            // Reset the model rotation
            resetModelRotation();

            // Reset the default view, facing down to the model from diagonally above the bounding box
            double distance = getSceneBounds().getRadius() * _defaultCamDistanceFactor;

            setViewOrigin(Vector3(1, 1, 1) * distance);
            setViewAngles(Vector3(34, 135, 0));
        }

        _lastModel = _model;

        // Done loading, emit the signal
        _modelLoadedSignal.emit(model);
    }
}

AABB ModelPreview::getSceneBounds()
{
    if (!_modelNode)
    {
        return EntityPreview::getSceneBounds();
    }

    return _modelNode->localAABB();
}

sigc::signal<void, const model::ModelNodePtr&>& ModelPreview::signal_ModelLoaded()
{
    return _modelLoadedSignal;
}

} // namespace ui
