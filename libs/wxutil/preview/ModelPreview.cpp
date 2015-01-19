#include "ModelPreview.h"
#include "../GLWidget.h"

#include "ifilter.h"
#include "iuimanager.h"
#include "imodelcache.h"
#include "i18n.h"
#include "ieclass.h"
#include "os/path.h"
#include "math/AABB.h"
#include "modelskin.h"
#include "entitylib.h"
#include "scene/Node.h"
#include "scene/BasicRootNode.h"
#include "wxutil/dialog/MessageBox.h"
#include "string/convert.h"
#include <sstream>

#include "iuimanager.h"

#include <boost/algorithm/string/case_conv.hpp>

namespace wxutil
{

/* CONSTANTS */

namespace
{
	const char* const FUNC_STATIC_CLASS = "func_static";
}

// Construct the widgets

ModelPreview::ModelPreview(wxWindow* parent) :
    RenderPreview(parent, false),
	_lastModel(""),
	_defaultCamDistanceFactor(2.8f)
{}

// Set the model, this also resets the camera
void ModelPreview::setModel(const std::string& model)
{
	// If the model name is empty, release the model
	if (model.empty())
	{
		if (_modelNode)
		{
			_entity->removeChildNode(_modelNode);
		}

		_modelNode.reset();

		stopPlayback();
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

	_modelNode = GlobalModelCache().getModelNode(model);

	if (_modelNode)
	{
		_entity->addChildNode(_modelNode);

		// Trigger an initial update of the subgraph
		GlobalFilterSystem().updateSubgraph(getScene()->root());

		// Reset camera if the model has changed
		if (model != _lastModel)
		{
			// Reset preview time
			stopPlayback();

			// Reset the model rotation
            resetModelRotation();

            // Reset the default view, facing down to the model from diagonally above the bounding box
            double distance = _modelNode->localAABB().getRadius() * _defaultCamDistanceFactor;

            setViewOrigin(Vector3(1, 1, 1) * distance);
            setViewAngles(Vector3(34, 135, 0));
		}

		_lastModel = model;
	}

	// Redraw
	queueDraw();
}

// Set the skin, this does NOT reset the camera

void ModelPreview::setSkin(const std::string& skin) {

	// Load and apply the skin, checking first to make sure the model is valid
	// and not null
	if (_modelNode != NULL)
	{
		model::ModelNodePtr model = Node_getModel(_modelNode);

		if (model)
		{
			ModelSkin& mSkin = GlobalModelSkinCache().capture(skin);
			model->getIModel().applySkin(mSkin);
		}
	}

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
        _rootNode.reset(new scene::BasicRootNode);

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
        wxutil::Messagebox::ShowError(
            (boost::format(_("Unable to setup the preview,\n"
            "could not find the entity class %s")) % FUNC_STATIC_CLASS).str());
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

bool ModelPreview::onPreRender()
{
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

} // namespace ui
