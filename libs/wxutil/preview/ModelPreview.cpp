#include "ModelPreview.h"
#include "../GLWidget.h"

#include "ifilter.h"
#include "iuimanager.h"
#include "imodelcache.h"
#include "i18n.h"
#include "ieclass.h"
#include "math/AABB.h"
#include "modelskin.h"
#include "entitylib.h"
#include "scene/Node.h"
#include "scene/BasicRootNode.h"
#include "wxutil/dialog/MessageBox.h"
#include "string/convert.h"
#include <sstream>

#include "iuimanager.h"

namespace wxutil
{

/* CONSTANTS */

namespace
{
	const char* const FUNC_STATIC_CLASS = "func_static";
}

ModelPreview::ModelPreview(wxWindow* parent) :
    RenderPreview(parent, false),
	_sceneIsReady(false),
	_lastModel(""),
	_defaultCamDistanceFactor(2.8f)
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
	_sceneIsReady = false;

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
	_sceneIsReady = false;

	// Redraw
	queueDraw();
}

void ModelPreview::setDefaultCamDistanceFactor(float factor)
{
	_defaultCamDistanceFactor = factor;
}

void ModelPreview::setupSceneGraph()
{
	RenderPreview::setupSceneGraph();

    try
    {
        _rootNode = std::make_shared<scene::BasicRootNode>();

        _entity = GlobalEntityCreator().createEntity(
            GlobalEntityClassManager().findClass(FUNC_STATIC_CLASS));

        _rootNode->addChildNode(_entity);

        _entity->enable(scene::Node::eHidden);

        // This entity is acting as our root node in the scene
        getScene()->setRoot(_rootNode);

        // Set up the light
        _light = GlobalEntityCreator().createEntity(
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

AABB ModelPreview::getSceneBounds()
{
	if (!_modelNode)
	{
		return RenderPreview::getSceneBounds();
	}

	return _modelNode->localAABB();
}

void ModelPreview::prepareScene()
{
	// Clear the flag
	_sceneIsReady = true;

	// If the model name is empty, release the model
	if (_model.empty())
	{
		if (_modelNode)
		{
			_entity->removeChildNode(_modelNode);
		}

		_modelNode.reset();

		// Emit the signal carrying an empty pointer
		_modelLoadedSignal.emit(model::ModelNodePtr());
		return;
	}

	// Set up the scene
	if (!_entity)
	{
		getScene(); // trigger a setupscenegraph call
	}

	if (_modelNode)
	{
		_entity->removeChildNode(_modelNode);
	}

	_modelNode = GlobalModelCache().getModelNode(_model);

	if (_modelNode)
	{
		_entity->addChildNode(_modelNode);

		// Apply the skin
		model::ModelNodePtr model = Node_getModel(_modelNode);

		if (model)
		{
			ModelSkin& mSkin = GlobalModelSkinCache().capture(_skin);
			model->getIModel().applySkin(mSkin);
		}

		// Trigger an initial update of the subgraph
		GlobalFilterSystem().updateSubgraph(getScene()->root());

		if (_lastModel != _model)
		{
			// Reset the model rotation
			resetModelRotation();

			// Reset the default view, facing down to the model from diagonally above the bounding box
			double distance = _modelNode->localAABB().getRadius() * _defaultCamDistanceFactor;

			setViewOrigin(Vector3(1, 1, 1) * distance);
			setViewAngles(Vector3(34, 135, 0));
		}

		_lastModel = _model;

		// Done loading, emit the signal
		_modelLoadedSignal.emit(model);
	}
}

bool ModelPreview::onPreRender()
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

	return _modelNode != nullptr;
}

void ModelPreview::onModelRotationChanged()
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

RenderStateFlags ModelPreview::getRenderFlagsFill()
{
	return RenderPreview::getRenderFlagsFill() | RENDER_DEPTHWRITE | RENDER_DEPTHTEST;
}

sigc::signal<void, const model::ModelNodePtr&>& ModelPreview::signal_ModelLoaded()
{
	return _modelLoadedSignal;
}

} // namespace ui
