#include "ParticlePreview.h"

#include "gtkutil/GLWidgetSentry.h"
#include "iuimanager.h"
#include "iparticles.h"

#include "math/aabb.h"
#include "entitylib.h"

#include <gtkmm/box.h>
#include <gtkmm/toolbar.h>
#include <gtkmm/image.h>
#include <gtkmm/toggletoolbutton.h>

#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/case_conv.hpp>

namespace ui
{

/* CONSTANTS */

namespace {

	const GLfloat PREVIEW_FOV = 60;
	const unsigned int MSEC_PER_FRAME = 16;

	// Set the bool to true for the lifetime of this object
	class ScopedBoolLock
	{
	private:
		bool& _target;

	public:
		ScopedBoolLock(bool& target) :
			_target(target)
		{
			_target = true;
		}

		~ScopedBoolLock()
		{
			_target = false;
		}
	};
}

// Construct the widgets

ParticlePreview::ParticlePreview() :
	Gtk::Frame(),
	_glWidget(Gtk::manage(new gtkutil::GLWidget(true, "ParticlePreview"))),
	_renderSystem(GlobalRenderSystemFactory().createRenderSystem()),
	_renderer(_renderSystem),
	_previewTimeMsec(0),
	_renderingInProgress(false),
	_timer(MSEC_PER_FRAME, _onFrame, this)
{
	// Main vbox - above is the GL widget, below is the toolbar
	Gtk::VBox* vbx = Gtk::manage(new Gtk::VBox(false, 0));
	
	// Cast the GLWidget object to GtkWidget for further use
	vbx->pack_start(*_glWidget, true, true, 0);
	
	// Connect up the signals
	_glWidget->set_events(Gdk::EXPOSURE_MASK | Gdk::BUTTON_PRESS_MASK | Gdk::BUTTON_RELEASE_MASK | 
						 Gdk::POINTER_MOTION_MASK | Gdk::SCROLL_MASK);
	
	_glWidget->signal_expose_event().connect(sigc::mem_fun(*this, &ParticlePreview::callbackGLDraw));
	_glWidget->signal_motion_notify_event().connect(sigc::mem_fun(*this, &ParticlePreview::callbackGLMotion));
	_glWidget->signal_scroll_event().connect(sigc::mem_fun(*this, &ParticlePreview::callbackGLScroll));
	
	// The HBox containing the toolbar and the menubar
	Gtk::HBox* toolHBox = Gtk::manage(new Gtk::HBox(false, 0));
	vbx->pack_end(*toolHBox, false, false, 0);

	// Create the toolbar
	Gtk::Toolbar* toolbar = Gtk::manage(new Gtk::Toolbar);
	toolbar->set_toolbar_style(Gtk::TOOLBAR_ICONS);
	toolHBox->pack_end(*toolbar, true, true, 0);
		
	// Pack into a frame and return
	add(*vbx);
}

ParticlePreview::~ParticlePreview()
{
	if (_timer.isEnabled())
	{
		_timer.disable();
	}
}

// Set the size request for the widget

void ParticlePreview::setSize(int size)
{
	_glWidget->set_size_request(size, size);
}

// Initialise the preview GL stuff

void ParticlePreview::initialisePreview()
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


	_renderSystem->setLightingEnabled(true);
}

// Set the model, this also resets the camera

void ParticlePreview::setParticle(const std::string& name)
{
	std::string nameClean = name;

	if (boost::algorithm::ends_with(nameClean, ".prt")) 
	{
		nameClean = nameClean.substr(0, nameClean.length() - 4);
	}

	// If the model name is empty, release the model
	if (nameClean.empty())
	{
		_particle.reset();
		_timer.disable();
		return;
	}

	_particle = GlobalParticlesManager().getRenderableParticle(nameClean);
			
	if (_particle != NULL && _lastParticle != nameClean)
	{
		// Reset preview time
		_previewTimeMsec = 0;

		_timer.enable();

		// Reset the rotation
		_rotation = Matrix4::getIdentity();
		
		// TODO: Use particle AABB
		_camDist = -(20 * 2.0);
		_rotation = Matrix4::getIdentity();

		_lastParticle = nameClean;

		// Update the particle
		_particle->update(_previewTimeMsec, *_renderSystem);
	}

	// Redraw
	_glWidget->queueDraw();
}

Gtk::Widget* ParticlePreview::getWidget()
{
	return this;
}

bool ParticlePreview::callbackGLDraw(GdkEventExpose* ev) 
{
	if (_renderingInProgress) return false; // avoid double-entering this method

	ScopedBoolLock lock(_renderingInProgress); // will be set to false on method exit

	// Create scoped sentry object to swap the GLWidget's buffers
	gtkutil::GLWidgetSentry sentry(*_glWidget);

	// Set up the render and clear the drawing area in any case
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	if (_particle == NULL) return false; // nothing to do

	// Update the particle
	_particle->update(_previewTimeMsec, *_renderSystem);

	// Front-end render phase, collect OpenGLRenderable objects from the
	// particle system
	_particle->renderSolid(_renderer, _volumeTest);

	RenderStateFlags flags = RENDER_DEPTHTEST
                             | RENDER_COLOURWRITE
                             //| RENDER_DEPTHWRITE
                             | RENDER_ALPHATEST
                             | RENDER_BLEND
                             | RENDER_CULLFACE
                             | RENDER_COLOURARRAY
                             | RENDER_OFFSETLINE
                             | RENDER_POLYGONSMOOTH
                             | RENDER_LINESMOOTH
                             | RENDER_COLOURCHANGE
							 | RENDER_FILL
							 | RENDER_LIGHTING
							 | RENDER_TEXTURE_2D
							 | RENDER_SMOOTH
							 | RENDER_SCALED;

	flags |= RENDER_FILL
           | RENDER_LIGHTING
           | RENDER_TEXTURE_2D
           | RENDER_TEXTURE_CUBEMAP
           | RENDER_SMOOTH
           | RENDER_SCALED
           | RENDER_BUMP
           | RENDER_PROGRAM
           | RENDER_MATERIAL_VCOL
           | RENDER_VCOL_INVERT
           | RENDER_SCREEN;

	// Set up the camera
	Matrix4 projection;
	{
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		gluPerspective(PREVIEW_FOV, 1, 0.1, 10000);

		// Load the matrix from openGL
		glGetDoublev(GL_PROJECTION_MATRIX, projection);
	}

	// Premultiply with the translations
	Matrix4 modelview;
	{
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();

		glTranslatef(0, 0, _camDist); // camera translation
		glMultMatrixd(_rotation); // post multiply with rotations

		// Load the matrix from openGL
		glGetDoublev(GL_MODELVIEW_MATRIX, modelview);
	}

	// Launch the back end rendering
	_renderSystem->render(flags, modelview, projection);

	// Draw coordinate axes for better orientation
	drawAxes();

	return false;
}

gboolean ParticlePreview::_onFrame(gpointer data)
{
	ParticlePreview* self = reinterpret_cast<ParticlePreview*>(data);

	if (!self->_renderingInProgress)
	{
		self->_previewTimeMsec += MSEC_PER_FRAME;
		self->_glWidget->queue_draw();
	}

	// Return true, so that the timer gets called again
	return TRUE;
}

void ParticlePreview::drawAxes()
{
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_LIGHTING);

	glLineWidth(2);
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

	glBegin(GL_LINES);

	glColor4f(1,0,0,0.6f);
	glVertex3f(0,0,0);
	glVertex3f(5,0,0);

	glColor4f(0,1,0,0.6f);
	glVertex3f(0,0,0);
	glVertex3f(0,5,0);

	glColor4f(0,0,1,0.6f);
	glVertex3f(0,0,0);
	glVertex3f(0,0,5);

	glEnd();
}

bool ParticlePreview::callbackGLMotion(GdkEventMotion* ev)
{
	if (ev->state & GDK_BUTTON1_MASK) // dragging with mouse button
	{
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
		
		// Grab the GL widget, and update the modelview matrix with the 
		// additional rotation
		if (gtkutil::GLWidget::makeCurrent(*_glWidget))
		{
			// Premultiply the current modelview matrix with the rotation,
			// in order to achieve rotations in eye space rather than object
			// space. At this stage we are only calculating and storing the
			// matrix for the GLDraw callback to use.
			glLoadIdentity();
			glRotated(-2, axisRot.x(), axisRot.y(), axisRot.z());
			glMultMatrixd(_rotation);

			// Save the new GL matrix for GL draw
			glGetDoublev(GL_MODELVIEW_MATRIX, _rotation);

			_glWidget->queueDraw(); // trigger the GLDraw method to draw the actual model
		}
	}

	return false;
}

bool ParticlePreview::callbackGLScroll(GdkEventScroll* ev)
{
	float inc = 40 * 0.1; // Scroll increment is a fraction of the AABB radius

	if (ev->direction == GDK_SCROLL_UP)
		_camDist += inc;
	else if (ev->direction == GDK_SCROLL_DOWN)
		_camDist -= inc;

	if (!_renderingInProgress)
	{
		_glWidget->queueDraw();
	}

	return false;
}

} // namespace ui
