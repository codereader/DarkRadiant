#include "ModelPreview.h"

#include "gtkutil/GLWidgetSentry.h"
#include "iuimanager.h"
#include "imodelcache.h"
#include "ieclass.h"
#include "os/path.h"
#include "math/AABB.h"
#include "math/frustum.h"
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

namespace {

	const GLfloat PREVIEW_FOV = 60;

}

// Construct the widgets

ModelPreview::ModelPreview() :
	Gtk::Frame(),
	_glWidget(Gtk::manage(new gtkutil::GLWidget(true, "ModelPreview"))),
	_lastModel(""),
	_filtersMenu(GlobalUIManager().createFilterMenu()),
	_previewWidth(0),
	_previewHeight(0)
{
	// Main vbox - above is the GL widget, below is the toolbar
	Gtk::VBox* vbx = Gtk::manage(new Gtk::VBox(false, 0));

	// Cast the GLWidget object to GtkWidget for further use
	vbx->pack_start(*_glWidget, true, true, 0);

	// Connect up the signals
	_glWidget->set_events(Gdk::EXPOSURE_MASK | Gdk::BUTTON_PRESS_MASK | Gdk::BUTTON_RELEASE_MASK |
						 Gdk::POINTER_MOTION_MASK | Gdk::SCROLL_MASK);

	_glWidget->signal_expose_event().connect(sigc::mem_fun(*this, &ModelPreview::callbackGLDraw));
	_glWidget->signal_motion_notify_event().connect(sigc::mem_fun(*this, &ModelPreview::callbackGLMotion));
	_glWidget->signal_scroll_event().connect(sigc::mem_fun(*this, &ModelPreview::callbackGLScroll));
	_glWidget->signal_size_allocate().connect(sigc::mem_fun(*this, &ModelPreview::onSizeAllocate));

	// The HBox containing the toolbar and the menubar
	Gtk::HBox* toolHBox = Gtk::manage(new Gtk::HBox(false, 0));
	vbx->pack_end(*toolHBox, false, false, 0);

	// Create the toolbar
	Gtk::Toolbar* toolbar = Gtk::manage(new Gtk::Toolbar);
	toolbar->set_toolbar_style(Gtk::TOOLBAR_ICONS);
	toolHBox->pack_end(*toolbar, true, true, 0);

	// Draw bounding box toolbar button
	_drawBBox = Gtk::manage(new Gtk::ToggleToolButton);
	_drawBBox->signal_toggled().connect(sigc::mem_fun(*this, &ModelPreview::callbackToggleBBox));

	_drawBBox->set_icon_widget(*Gtk::manage(new Gtk::Image(
		GlobalUIManager().getLocalPixbuf("iconDrawBBox.png"))));
	toolbar->insert(*_drawBBox, 0);

	// Create the menu
	toolHBox->pack_end(*_filtersMenu->getMenuBarWidget(), false, false, 0);

	// Pack into a frame and return
	add(*vbx);
}

// Set the size request for the widget

void ModelPreview::setSize(int xsize, int ysize)
{
	_glWidget->set_size_request(xsize, ysize);
}

void ModelPreview::clear()
{
	_modelCache.clear();
	_model = model::IModelPtr();
}

// Initialise the preview GL stuff

void ModelPreview::initialisePreview()
{
	// Grab the GL widget with sentry object
	gtkutil::GLWidgetSentry sentry(*_glWidget);

	// Clear the window
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glClearColor(0.0, 0.0, 0.0, 0);
	glClearDepth(100.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Set up the camera
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(PREVIEW_FOV, 1, 0.1, 10000);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	// Set up the lights
	glEnable(GL_LIGHTING);

	glEnable(GL_LIGHT0);
	GLfloat l0Amb[] = { 0.3f, 0.3f, 0.3f, 1.0f };
	GLfloat l0Dif[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	GLfloat l0Pos[] = { 1.0f, 1.0f, 1.0f, 0.0f };
	glLightfv(GL_LIGHT0, GL_AMBIENT, l0Amb);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, l0Dif);
	glLightfv(GL_LIGHT0, GL_POSITION, l0Pos);

	glEnable(GL_LIGHT1);
	GLfloat l1Dif[] = { 1.0, 1.0, 1.0, 1.0 };
	GLfloat l1Pos[] = { 0.0, 0.0, 1.0, 0.0 };
	glLightfv(GL_LIGHT1, GL_DIFFUSE, l1Dif);
	glLightfv(GL_LIGHT1, GL_POSITION, l1Pos);

}

// Set the model, this also resets the camera

void ModelPreview::setModel(const std::string& model)
{
	// If the model name is empty, release the model
	if (model == "") {
		_model = model::IModelPtr();
		return;
	}

	// Copy the model string to a local variable
	std::string modelToLoad = model;

	// Check the model cache if the model is already there
	ModelMap::iterator foundModel = _modelCache.find(model);

	if (foundModel != _modelCache.end()) {
		// Use cached model
		_model = foundModel->second;
	}
	else {
		// Load the model, using a lowercase version of the file extension to
		// identify the loader module to use
		std::string ldrName = os::getExtension(modelToLoad);
		boost::algorithm::to_lower(ldrName);

		// greebo: If the extension is empty, this might be a modeldef
		IModelDefPtr modelDef = GlobalEntityClassManager().findModel(modelToLoad);

		if (modelDef != NULL) {
			// Model def found, let's try to get the extension of the "mesh" key
			ldrName = os::getExtension(modelDef->mesh);
			modelToLoad = modelDef->mesh;
			boost::algorithm::to_lower(ldrName);
		}

		ModelLoaderPtr loader = GlobalModelCache().getModelLoaderForType(ldrName);

		if (loader != NULL) {
			_model = loader->loadModelFromPath(modelToLoad);
		}
		else {
			_model = model::IModelPtr();
			return;
		}

		if (_model != NULL) {
			// Insert model into cache on successful load
			// use the unmodified model name as key
			_modelCache.insert(ModelMap::value_type(model, _model));
		}
	}

	// Reset camera if the model has changed
	if (_model && modelToLoad != _lastModel) {
		// Reset the rotation
		_rotation = Matrix4::getIdentity();

		// Calculate camera distance so model is appropriately zoomed
		_camDist = -(_model->localAABB().getRadius() * 2.7f);

		_lastModel = modelToLoad;
	}

	// Redraw
	_glWidget->queueDraw();
}

// Set the skin, this does NOT reset the camera

void ModelPreview::setSkin(const std::string& skin) {

	// Load and apply the skin, checking first to make sure the model is valid
	// and not null
	if (_model != NULL) {
		ModelSkin& mSkin = GlobalModelSkinCache().capture(skin);
		_model->applySkin(mSkin);
	}

	// Redraw
	_glWidget->queueDraw();
}

Gtk::Widget* ModelPreview::getWidget()
{
	return this;
}

void ModelPreview::onSizeAllocate(Gtk::Allocation& allocation)
{
	_previewWidth = allocation.get_width();
	_previewHeight = allocation.get_height();
}

Matrix4 ModelPreview::getProjectionMatrix(float near_z, float far_z, float fieldOfView, int width, int height)
{
	const float half_width = near_z * tan(degrees_to_radians(fieldOfView * 0.5f));
	const float half_height = half_width * (static_cast<float>(height) / static_cast<float>(width));

	return Matrix4::getProjectionForFrustum(
		-half_width,
		half_width,
		-half_height,
		half_height,
		near_z,
		far_z
	);
}

bool ModelPreview::callbackGLDraw(GdkEventExpose* ev)
{
	// Create scoped sentry object to swap the GLWidget's buffers
	gtkutil::GLWidgetSentry sentry(*_glWidget);

	glViewport(0, 0, _previewWidth, _previewHeight);

	// Set up the render
	glClearColor(0.2f, 0.2f, 0.2f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Get the current model if it exists, and if so get its AABB and proceed
	// with rendering, otherwise exit.
	model::IModelPtr model = _model;
	if (!model)
		return false;

	AABB aabb(model->localAABB());

	// Set up the camera
	Matrix4 projection = getProjectionMatrix(0.1f, 10000, PREVIEW_FOV, _previewWidth, _previewHeight);

	glMatrixMode(GL_PROJECTION);
	glLoadMatrixf(projection);

	// Premultiply with the translations
	glMatrixMode(GL_MODELVIEW);
	
	// Premultiply with the translations
	glLoadIdentity();
	glTranslatef(0, 0, _camDist); // camera translation
	glMultMatrixf(_rotation); // post multiply with rotations
	glRotatef(90, -1, 0, 0); // axis rotation (y-up (GL) -> z-up (model))

	// Render the bounding box if the toggle is active
	if (_drawBBox->get_active())
	{
		// Render as fullbright wireframe
		glDisable(GL_LIGHTING);
		glDisable(GL_TEXTURE_2D);
		glColor3f(0, 1, 1);

		aabb_draw_wire(aabb); // TODO: This seems to have broken (was RenderableAABB before)
	}

	// Render the actual model.
	glEnable(GL_LIGHTING);
	glTranslatef(-aabb.origin.x(), -aabb.origin.y(), -aabb.origin.z()); // model translation
	model->render(RENDER_TEXTURE_2D);

	return false;
}

bool ModelPreview::callbackGLMotion(GdkEventMotion* ev)
{
	if (ev->state & GDK_BUTTON1_MASK) // dragging with mouse button
	{
		static gdouble _lastX = ev->x;
		static gdouble _lastY = ev->y;

		// Calculate the mouse delta as a vector in the XY plane, and store the
		// current position for the next event.
		Vector3 deltaPos(static_cast<float>(ev->x - _lastX),
						 static_cast<float>(_lastY - ev->y),
						 0);
		_lastX = ev->x;
		_lastY = ev->y;

		// Calculate the axis of rotation. This is the mouse vector crossed with the Z axis,
		// to give a rotation axis in the XY plane at right-angles to the mouse delta.
		static Vector3 _zAxis(0, 0, 1);
		Vector3 axisRot = deltaPos.crossProduct(_zAxis);

		// Grab the GL widget, and update the modelview matrix with the
		// additional rotation
		if (gtkutil::GLWidget::makeCurrent(*_glWidget))
		{
			// Premultiply the current modelview matrix with the rotation,
			// in order to achieve rotations in eye space rather than object
			// space. At this stage we are only calculating and storing the
			// matrix for the GLDraw callback to use.
			glLoadIdentity();
			glRotatef(-2, axisRot.x(), axisRot.y(), axisRot.z());
			glMultMatrixf(_rotation);

			// Save the new GL matrix for GL draw
			glGetFloatv(GL_MODELVIEW_MATRIX, _rotation);

			_glWidget->queueDraw(); // trigger the GLDraw method to draw the actual model
		}
	}

	return false;
}

bool ModelPreview::callbackGLScroll(GdkEventScroll* ev)
{
	if (_model == NULL) return false;

	float inc = _model->localAABB().getRadius() * 0.1f; // Scroll increment is a fraction of the AABB radius
	if (ev->direction == GDK_SCROLL_UP)
		_camDist += inc;
	else if (ev->direction == GDK_SCROLL_DOWN)
		_camDist -= inc;

	_glWidget->queueDraw();

	return false;
}

void ModelPreview::callbackToggleBBox()
{
	_glWidget->queueDraw();
}

} // namespace ui
