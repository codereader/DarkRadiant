#include "ModelPreview.h"

#include "ifilter.h"
#include "iuimanager.h"
#include "imodelcache.h"
#include "ieclass.h"
#include "os/path.h"
#include "math/AABB.h"
#include "modelskin.h"
#include "entitylib.h"

#include "iuimanager.h"

#include <gtkmm/box.h>
#include <gtkmm/toolbar.h>
#include <gtkmm/image.h>
#include <gtkmm/toggletoolbutton.h>

#include <boost/algorithm/string/case_conv.hpp>

namespace ui
{

/* CONSTANTS */

namespace
{
	const char* const FUNC_STATIC_CLASS = "func_static";
}

// Construct the widgets

ModelPreview::ModelPreview() :
	gtkutil::RenderPreview(),
	_lastModel("")
{
	// Create the toolbar
	Gtk::Toolbar* toolbar = Gtk::manage(new Gtk::Toolbar);
	toolbar->set_toolbar_style(Gtk::TOOLBAR_ICONS);

	// Draw bounding box toolbar button
	_drawBBox = Gtk::manage(new Gtk::ToggleToolButton);
	_drawBBox->signal_toggled().connect(sigc::mem_fun(*this, &ModelPreview::callbackToggleBBox));

	_drawBBox->set_icon_widget(*Gtk::manage(new Gtk::Image(
		GlobalUIManager().getLocalPixbuf("iconDrawBBox.png"))));
	toolbar->insert(*_drawBBox, 0);

	addToolbar(*toolbar);
}

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

			// Reset the rotation to the default one
			_rotation = Matrix4::getRotationAboutZDegrees(45);
			_rotation = _rotation.getMultipliedBy(Matrix4::getRotation(Vector3(1,-1,0), 45));

			// Calculate camera distance so model is appropriately zoomed
			_camDist = -(_modelNode->localAABB().getRadius() * 5.0f);
		}

		_lastModel = model;
	}

	// Redraw
	_glWidget->queueDraw();
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
	_glWidget->queueDraw();
}

Gtk::Widget* ModelPreview::getWidget()
{
	return this;
}

void ModelPreview::setupSceneGraph()
{
	RenderPreview::setupSceneGraph();

	_entity = GlobalEntityCreator().createEntity(
		GlobalEntityClassManager().findClass(FUNC_STATIC_CLASS));

	_entity->enable(scene::Node::eHidden);

	// This entity is acting as our root node in the scene
	getScene()->setRoot(_entity);
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
	return _modelNode != NULL;
}

void ModelPreview::onPostRender()
{
	// Render the bounding box if the toggle is active
	if (_drawBBox->get_active())
	{
		// Render as fullbright wireframe
		glDisable(GL_LIGHTING);
		glDisable(GL_TEXTURE_2D);
		glColor3f(0, 1, 1);

		aabb_draw_wire(_modelNode->localAABB());
	}
}

RenderStateFlags ModelPreview::getRenderFlagsFill()
{
	return RenderPreview::getRenderFlagsFill() | RENDER_DEPTHWRITE | RENDER_DEPTHTEST;
}

void ModelPreview::callbackToggleBBox()
{
	_glWidget->queueDraw();
}

} // namespace ui
