#include "ModelPreview.h"

#include "gtkutil/glwidget.h"
#include "os/path.h"
#include "referencecache.h"
#include "math/aabb.h"
#include "modelskin.h"

#include <gtk/gtk.h>

namespace ui
{

/* CONSTANTS */

namespace {

	const GLfloat PREVIEW_FOV = 60;

}

// Construct the widgets

ModelPreview::ModelPreview()
: _widget(gtk_frame_new(NULL))
{
	// Create the GL widget
	_glWidget = glwidget_new(TRUE);
	
	// Connect up the signals
	gtk_widget_set_events(_glWidget, GDK_DESTROY | GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_POINTER_MOTION_MASK | GDK_SCROLL_MASK);
	g_signal_connect(G_OBJECT(_glWidget), "expose-event", G_CALLBACK(callbackGLDraw), this);
	g_signal_connect(G_OBJECT(_glWidget), "motion-notify-event", G_CALLBACK(callbackGLMotion), this);
	g_signal_connect(G_OBJECT(_glWidget), "scroll-event", G_CALLBACK(callbackGLScroll), this);
	
	// Pack into a frame and return
	gtk_container_add(GTK_CONTAINER(_widget), _glWidget);
}

// Set the size request for the widget

void ModelPreview::setSize(int size) {
	gtk_widget_set_size_request(_glWidget, size, size);	
}

// Initialise the preview GL stuff

void ModelPreview::initialisePreview() {

	if (glwidget_make_current(_glWidget) != FALSE) {

		// Depth buffer and polygon mode
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LEQUAL);

		// Clear the window
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
		GLfloat l0Amb[] = { 0.3, 0.3, 0.3, 1.0 };
		GLfloat l0Dif[] = { 1.0, 1.0, 1.0, 1.0 };
		GLfloat l0Pos[] = { 1.0, 1.0, 1.0, 0.0 };
		glLightfv(GL_LIGHT0, GL_AMBIENT, l0Amb);
		glLightfv(GL_LIGHT0, GL_DIFFUSE, l0Dif);
		glLightfv(GL_LIGHT0, GL_POSITION, l0Pos);
		
		glEnable(GL_LIGHT1);
		GLfloat l1Dif[] = { 1.0, 1.0, 1.0, 1.0 };
		GLfloat l1Pos[] = { 0.0, 0.0, 1.0, 0.0 };
		glLightfv(GL_LIGHT1, GL_DIFFUSE, l1Dif);
		glLightfv(GL_LIGHT1, GL_POSITION, l1Pos);
	}		

}

// Set the model, this also resets the camera

void ModelPreview::setModel(const std::string& model) {

	// If the model name is empty, release the model
	if (model == "") {
		_model = model::IModelPtr();
		return;
	}

	// Load the model
	ModelLoader* loader = ModelLoader_forType(os::getExtension(model).c_str());
	if (loader != NULL) {
		_model = loader->loadModelFromPath(model);
	}
	else {
		return;
	}

	// Reset camera if the model has changed
	static std::string _lastModel;	

	if (model != _lastModel) {
		// Reset the rotation
		_rotation = Matrix4::getIdentity();
		
		// Calculate camera distance so model is appropriately zoomed
		_camDist = -(_model->getAABB().getRadius() * 2.0); 

		_lastModel = model;
	}

	// Redraw
	gtk_widget_queue_draw(_glWidget);
}

// Set the skin, this does NOT reset the camera

void ModelPreview::setSkin(const std::string& skin) {
	// Load and apply the skin
	ModelSkin& mSkin = GlobalModelSkinCache().capture(skin);
	_model->applySkin(mSkin);
	
	// Redraw
	gtk_widget_queue_draw(_glWidget);
}

/* GTK CALLBACKS */

void ModelPreview::callbackGLDraw(GtkWidget* widget, GdkEventExpose* ev, ModelPreview* self) {
	if (glwidget_make_current(widget) != FALSE) {

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Get the current model if it exists, and if so get its AABB and proceed
		// with rendering, otherwise exit.
		model::IModel* model = self->_model.get();
		AABB aabb;
		if (model == NULL)
			goto swapAndReturn;
			
		aabb = model->getAABB();

		// Premultiply with the translations
		glLoadIdentity();
		glTranslatef(0, 0, self->_camDist); // camera translation
		glMultMatrixf(self->_rotation); // post multiply with rotations
		glTranslatef(-aabb.origin.x(), -aabb.origin.y(), -aabb.origin.z()); // model translation

		// Render the actual model.
		model->render(RENDER_DEFAULT);

swapAndReturn:
		
		// Swap buffers to display the result in GTK
		glwidget_swap_buffers(widget);
	}	
}

void ModelPreview::callbackGLMotion(GtkWidget* widget, GdkEventMotion* ev, ModelPreview* self) {
	if (ev->state & GDK_BUTTON1_MASK) { // dragging with mouse button
		static gdouble _lastX = ev->x;
		static gdouble _lastY = ev->y;

		// Calculate the mouse delta as a vector in the XY plane, and store the
		// current position for the next event.
		Vector3 deltaPos(ev->x - _lastX,
						 _lastY - ev->y,
						 0);
		_lastX = ev->x;
		_lastY = ev->y;
		
		// Calculate the axis of rotation. This is the mouse vector crossed with the Z axis,
		// to give a rotation axis in the XY plane at right-angles to the mouse delta.
		static Vector3 _zAxis(0, 0, 1);
		Vector3 axisRot = deltaPos.crossProduct(_zAxis);
		
		// Grab the GL widget, and update the modelview matrix with the additional
		// rotation (TODO: may not be the best way to do this).
		if (glwidget_make_current(widget) != FALSE) {

			// Premultiply the current modelview matrix with the rotation,
			// in order to achieve rotations in eye space rather than object
			// space. At this stage we are only calculating and storing the
			// matrix for the GLDraw callback to use.
			glLoadIdentity();
			glRotatef(-2, axisRot.x(), axisRot.y(), axisRot.z());
			glMultMatrixf(self->_rotation);

			// Save the new GL matrix for GL draw
			glGetFloatv(GL_MODELVIEW_MATRIX, self->_rotation);

			gtk_widget_queue_draw(widget); // trigger the GLDraw method to draw the actual model
		}
		
	}
}

void ModelPreview::callbackGLScroll(GtkWidget* widget, GdkEventScroll* ev, ModelPreview* self) {
	float inc = self->_model->getAABB().getRadius() * 0.1; // Scroll increment is a fraction of the AABB radius
	if (ev->direction == GDK_SCROLL_UP)
		self->_camDist += inc;
	else if (ev->direction == GDK_SCROLL_DOWN)
		self->_camDist -= inc;
	gtk_widget_queue_draw(widget);
}



}
