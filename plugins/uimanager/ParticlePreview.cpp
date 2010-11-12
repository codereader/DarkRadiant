#include "ParticlePreview.h"

#include "gtkutil/GLWidgetSentry.h"
#include "iuimanager.h"
#include "ieventmanager.h"
#include "iparticles.h"
#include "i18n.h"

#include "math/aabb.h"
#include "math/frustum.h"
#include "entitylib.h"

#include "string/string.h"

#include <gtkmm/box.h>
#include <gtkmm/toolbar.h>
#include <gtkmm/image.h>
#include <gtkmm/toggletoolbutton.h>
#include <gtkmm/toolbutton.h>
#include <gtkmm/stock.h>

#include <boost/format.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>

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
	_startButton(NULL),
	_pauseButton(NULL),
	_stopButton(NULL),
	_renderSystem(GlobalRenderSystemFactory().createRenderSystem()),
	_renderer(_renderSystem),
	_previewTimeMsec(0),
	_renderingInProgress(false),
	_timer(MSEC_PER_FRAME, _onFrame, this),
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

	_glWidget->signal_expose_event().connect(sigc::mem_fun(*this, &ParticlePreview::callbackGLDraw));
	_glWidget->signal_motion_notify_event().connect(sigc::mem_fun(*this, &ParticlePreview::callbackGLMotion));
	_glWidget->signal_scroll_event().connect(sigc::mem_fun(*this, &ParticlePreview::callbackGLScroll));
	_glWidget->signal_size_allocate().connect(sigc::mem_fun(*this, &ParticlePreview::onSizeAllocate));

	// The HBox containing the toolbar and the menubar
	Gtk::HBox* toolHBox = Gtk::manage(new Gtk::HBox(false, 0));
	vbx->pack_end(*toolHBox, false, false, 0);

	// Create the toolbar
	Gtk::Toolbar* toolbar = Gtk::manage(new Gtk::Toolbar);
	toolbar->set_toolbar_style(Gtk::TOOLBAR_ICONS);

	_startButton = Gtk::manage(new Gtk::ToolButton);
	_startButton->signal_clicked().connect(sigc::mem_fun(*this, &ParticlePreview::callbackStart));
	_startButton->set_icon_widget(*Gtk::manage(new Gtk::Image(Gtk::Stock::MEDIA_PLAY, Gtk::ICON_SIZE_MENU)));
	_startButton->set_tooltip_text(_("Start time"));

	_pauseButton = Gtk::manage(new Gtk::ToolButton);
	_pauseButton->signal_clicked().connect(sigc::mem_fun(*this, &ParticlePreview::callbackPause));
	_pauseButton->set_icon_widget(*Gtk::manage(new Gtk::Image(Gtk::Stock::MEDIA_PAUSE, Gtk::ICON_SIZE_MENU)));
	_pauseButton->set_tooltip_text(_("Pause time"));

	_stopButton = Gtk::manage(new Gtk::ToolButton);
	_stopButton->signal_clicked().connect(sigc::mem_fun(*this, &ParticlePreview::callbackStop));
	_stopButton->set_icon_widget(*Gtk::manage(new Gtk::Image(Gtk::Stock::MEDIA_STOP, Gtk::ICON_SIZE_MENU)));
	_stopButton->set_tooltip_text(_("Stop time"));

	Gtk::ToolButton* nextButton = Gtk::manage(new Gtk::ToolButton);
	nextButton->signal_clicked().connect(sigc::mem_fun(*this, &ParticlePreview::callbackStepForward));
	nextButton->set_icon_widget(*Gtk::manage(new Gtk::Image(Gtk::Stock::MEDIA_NEXT, Gtk::ICON_SIZE_MENU)));
	nextButton->set_tooltip_text(_("Next frame"));

	Gtk::ToolButton* prevButton = Gtk::manage(new Gtk::ToolButton);
	prevButton->signal_clicked().connect(sigc::mem_fun(*this, &ParticlePreview::callbackStepBack));
	prevButton->set_icon_widget(*Gtk::manage(new Gtk::Image(Gtk::Stock::MEDIA_PREVIOUS, Gtk::ICON_SIZE_MENU)));
	prevButton->set_tooltip_text(_("Previous frame"));

	toolbar->insert(*_startButton, 0);
	toolbar->insert(*_pauseButton, 0);
	toolbar->insert(*nextButton, 0);
	toolbar->insert(*prevButton, 0);
	toolbar->insert(*_stopButton, 0);

	Gtk::Toolbar* toolbar2 = Gtk::manage(new Gtk::Toolbar);
	toolbar2->set_toolbar_style(Gtk::TOOLBAR_ICONS);

	_showAxesButton = Gtk::manage(new Gtk::ToggleToolButton);
	_showAxesButton->signal_toggled().connect(sigc::mem_fun(*this, &ParticlePreview::callbackToggleAxes));
	_showAxesButton->set_icon_widget(*Gtk::manage(new Gtk::Image(
		GlobalUIManager().getLocalPixbufWithMask("axes.png"))));
	_showAxesButton->set_tooltip_text(_("Show coordinate axes"));

	Gtk::ToolButton* reloadButton = Gtk::manage(new Gtk::ToolButton);
	reloadButton->set_icon_widget(*Gtk::manage(new Gtk::Image(Gtk::Stock::REFRESH, Gtk::ICON_SIZE_MENU)));
	reloadButton->set_tooltip_text(_("Reload Particle Defs"));
	IEventPtr ev = GlobalEventManager().findEvent("ReloadParticles");
	ev->connectWidget(reloadButton);

	_showWireFrameButton = Gtk::manage(new Gtk::ToggleToolButton);
	_showWireFrameButton->set_icon_widget(*Gtk::manage(new Gtk::Image(
		GlobalUIManager().getLocalPixbufWithMask("wireframe.png"))));
	_showWireFrameButton->set_tooltip_text(_("Show wireframe"));

	toolbar2->insert(*_showAxesButton, 0);
	toolbar2->insert(*_showWireFrameButton, 0);
	toolbar2->insert(*reloadButton, 0);

	toolHBox->pack_start(*toolbar, true, true, 0);
	toolHBox->pack_start(*toolbar2, true, true, 0);

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
		stopPlayback();
		return;
	}

	_particle = GlobalParticlesManager().getRenderableParticle(nameClean);

	if (_particle != NULL && _lastParticle != nameClean)
	{
		// Reset preview time
		stopPlayback();

		// Reset the rotation to the default one
		_rotation = Matrix4::getRotation(Vector3(0,-1,0), Vector3(0,-0.3,1));
		_rotation.multiplyBy(Matrix4::getRotation(Vector3(0,1,0), Vector3(1,-1,0)));

		// Call update(0) once to enable the bounds calculation
		_particle->update(_previewTimeMsec, *_renderSystem, _rotation);

		// Use particle AABB to adjust camera distance
		const AABB& particleBounds = _particle->getBounds();

		if (particleBounds.isValid())
		{
			_camDist = -2.0f * static_cast<float>(particleBounds.getRadius());
		}
		else
		{
			// Bounds not valid, fall back to default
			_camDist = -40.0f;
		}

		_lastParticle = nameClean;

		// Start playback when switching particles
		startPlayback();
	}

	// Redraw
	_glWidget->queueDraw();
}

Gtk::Widget* ParticlePreview::getWidget()
{
	return this;
}

void ParticlePreview::startPlayback()
{
	if (_timer.isEnabled())
	{
		// Timer is already running, just reset the preview time
		_previewTimeMsec = 0;
	}
	else
	{
		// Timer is not enabled, we're paused or stopped
		_timer.enable();
	}

	_pauseButton->set_sensitive(true);
	_stopButton->set_sensitive(true);
}

void ParticlePreview::stopPlayback()
{
	_previewTimeMsec = 0;
	_timer.disable();

	_pauseButton->set_sensitive(false);
	_stopButton->set_sensitive(false);

	_glWidget->queue_draw();
}

void ParticlePreview::callbackStart()
{
	startPlayback();
}

void ParticlePreview::callbackPause()
{
	// Disable the button
	_pauseButton->set_sensitive(false);

	if (_timer.isEnabled())
	{
		_timer.disable();
	}
	else
	{
		_timer.enable(); // re-enable playback
	}
}

void ParticlePreview::callbackStop()
{
	stopPlayback();
}

void ParticlePreview::callbackStepForward()
{
	// Disable the button
	_pauseButton->set_sensitive(false);

	if (_timer.isEnabled())
	{
		_timer.disable();
	}

	_previewTimeMsec += MSEC_PER_FRAME;
	_glWidget->queue_draw();
}

void ParticlePreview::callbackStepBack()
{
	// Disable the button
	_pauseButton->set_sensitive(false);

	if (_timer.isEnabled())
	{
		_timer.disable();
	}

	if (_previewTimeMsec > 0)
	{
		_previewTimeMsec -= MSEC_PER_FRAME;
	}

	_glWidget->queue_draw();
}

void ParticlePreview::callbackToggleAxes()
{
	_glWidget->queue_draw();
}

void ParticlePreview::onSizeAllocate(Gtk::Allocation& allocation)
{
	_previewWidth = allocation.get_width();
	_previewHeight = allocation.get_height();
}

Matrix4 ParticlePreview::getProjectionMatrix(float near_z, float far_z, float fieldOfView, int width, int height)
{
	const float half_width = static_cast<float>(near_z * tan(degrees_to_radians(fieldOfView * 0.5)));
	const float half_height = half_width * (static_cast<float>(height) / static_cast<float>(width));

	return matrix4_frustum(
		-half_width,
		half_width,
		-half_height,
		half_height,
		near_z,
		far_z
	);
}

bool ParticlePreview::callbackGLDraw(GdkEventExpose* ev)
{
	if (_renderingInProgress) return false; // avoid double-entering this method

	ScopedBoolLock lock(_renderingInProgress); // will be set to false on method exit

	// Create scoped sentry object to swap the GLWidget's buffers
	gtkutil::GLWidgetSentry sentry(*_glWidget);

	glViewport(0, 0, _previewWidth, _previewHeight);

	// Set up the render and clear the drawing area in any case
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	if (_particle == NULL)
	{
		drawTime();
		return false; // nothing to do
	}

	// Update the particle
	_particle->update(_previewTimeMsec, *_renderSystem, _rotation);

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

	// Force activation of client state GL_COLOR_ARRAY, shaders don't require this state
	flags |= RENDER_FORCE_COLORARRAY;

	// Set up the camera
	Matrix4 projection = getProjectionMatrix(0.1f, 10000, PREVIEW_FOV, _previewWidth, _previewHeight);

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

	if (_showWireFrameButton->get_active())
	{
		flags = RENDER_DEPTHTEST
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
							 | RENDER_LIGHTING
							 | RENDER_SMOOTH
							 | RENDER_SCALED;

		flags |= RENDER_LIGHTING
			   | RENDER_TEXTURE_CUBEMAP
			   | RENDER_SMOOTH
			   | RENDER_SCALED
			   | RENDER_BUMP
			   | RENDER_PROGRAM
			   | RENDER_MATERIAL_VCOL
			   | RENDER_VCOL_INVERT
			   | RENDER_SCREEN;

		flags |= RENDER_FORCE_COLORARRAY;

		_particle->renderSolid(_renderer, _volumeTest);

		_renderSystem->render(flags, modelview, projection);
	}

	// Draw coordinate axes for better orientation
	if (_showAxesButton->get_active())
	{
		drawAxes();
	}

	drawTime();
	//drawDebugInfo();

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

void ParticlePreview::drawTime()
{
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, static_cast<float>(_previewWidth), 0, static_cast<float>(_previewHeight), -100, 100);

	glScalef(1, -1, 1);
	glTranslatef(0, -static_cast<float>(_previewHeight), 0);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	if (GLEW_VERSION_1_3)
	{
		glClientActiveTexture(GL_TEXTURE0);
		glActiveTexture(GL_TEXTURE0);
	}

	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);

	glDisable(GL_TEXTURE_2D);
	glDisable(GL_LIGHTING);
	glDisable(GL_COLOR_MATERIAL);
	glDisable(GL_DEPTH_TEST);

	glColor3f( 1.f, 1.f, 1.f );
	glLineWidth(1);

	glRasterPos3f(1.0f, static_cast<float>(_previewHeight) - 1.0f, 0.0f);

	GlobalOpenGL().drawString((boost::format("%.3f sec.") % (_previewTimeMsec * 0.001f)).str());
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
	if (_particle == NULL) return false;

	// Scroll increment is a fraction of the AABB radius
	float inc = static_cast<float>(_particle->getBounds().getRadius()) * 0.1f;

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
