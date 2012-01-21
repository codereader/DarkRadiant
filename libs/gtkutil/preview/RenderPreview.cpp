#include "RenderPreview.h"

#include "ifilter.h"
#include "i18n.h"
#include "igl.h"
#include "iscenegraphfactory.h"
#include "irendersystemfactory.h"
#include "iuimanager.h"

#include "math/AABB.h"

#include <gtkmm/box.h>
#include <gtkmm/toolbar.h>
#include <gtkmm/stock.h>

#include "../GLWidgetSentry.h"

namespace gtkutil
{

namespace
{
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

// Helper class, which notifies the MapPreview about a filter change
class RenderPreviewFilterObserver :
	public FilterSystem::Observer
{
	RenderPreview& _owner;
public:
	RenderPreviewFilterObserver(RenderPreview& owner) :
		_owner(owner)
	{}

	void onFiltersChanged() {
		_owner.onFiltersChanged();
	}
};
typedef boost::shared_ptr<RenderPreviewFilterObserver> RenderPreviewFilterObserverPtr;

RenderPreview::RenderPreview() :
	Gtk::Frame(),
	_glWidget(Gtk::manage(new gtkutil::GLWidget(true, "RenderPreview"))),
	_startButton(NULL),
	_pauseButton(NULL),
	_stopButton(NULL),
	_renderSystem(GlobalRenderSystemFactory().createRenderSystem()),
	_sceneWalker(_renderer, _volumeTest),
	_renderingInProgress(false),
	_timer(MSEC_PER_FRAME, _onFrame, this),
	_previewWidth(0),
	_previewHeight(0),
	_filtersMenu(GlobalUIManager().createFilterMenu())
{
	// Main vbox - above is the GL widget, below is the toolbar
	Gtk::VBox* vbx = Gtk::manage(new Gtk::VBox(false, 0));

	// Cast the GLWidget object to GtkWidget for further use
	vbx->pack_start(*_glWidget, true, true, 0);

	// Connect up the signals
	_glWidget->set_events(Gdk::EXPOSURE_MASK | Gdk::BUTTON_PRESS_MASK | Gdk::BUTTON_RELEASE_MASK |
						 Gdk::POINTER_MOTION_MASK | Gdk::SCROLL_MASK);

	_glWidget->signal_expose_event().connect(sigc::mem_fun(*this, &RenderPreview::onGLDraw));
	_glWidget->signal_motion_notify_event().connect(sigc::mem_fun(*this, &RenderPreview::onGLMotion));
	_glWidget->signal_scroll_event().connect(sigc::mem_fun(*this, &RenderPreview::onGLScroll));
	_glWidget->signal_size_allocate().connect(sigc::mem_fun(*this, &RenderPreview::onSizeAllocate));

	// The HBox containing the toolbar and the menubar
	_toolHBox = Gtk::manage(new Gtk::HBox(false, 0));
	vbx->pack_end(*_toolHBox, false, false, 0);

	// Create the toolbar
	Gtk::Toolbar* toolbar = Gtk::manage(new Gtk::Toolbar);
	toolbar->set_toolbar_style(Gtk::TOOLBAR_ICONS);

	_startButton = Gtk::manage(new Gtk::ToolButton);
	_startButton->signal_clicked().connect(sigc::mem_fun(*this, &RenderPreview::onStart));
	_startButton->set_icon_widget(*Gtk::manage(new Gtk::Image(Gtk::Stock::MEDIA_PLAY, Gtk::ICON_SIZE_MENU)));
	_startButton->set_tooltip_text(_("Start time"));

	_pauseButton = Gtk::manage(new Gtk::ToolButton);
	_pauseButton->signal_clicked().connect(sigc::mem_fun(*this, &RenderPreview::onPause));
	_pauseButton->set_icon_widget(*Gtk::manage(new Gtk::Image(Gtk::Stock::MEDIA_PAUSE, Gtk::ICON_SIZE_MENU)));
	_pauseButton->set_tooltip_text(_("Pause time"));

	_stopButton = Gtk::manage(new Gtk::ToolButton);
	_stopButton->signal_clicked().connect(sigc::mem_fun(*this, &RenderPreview::onStop));
	_stopButton->set_icon_widget(*Gtk::manage(new Gtk::Image(Gtk::Stock::MEDIA_STOP, Gtk::ICON_SIZE_MENU)));
	_stopButton->set_tooltip_text(_("Stop time"));

	Gtk::ToolButton* nextButton = Gtk::manage(new Gtk::ToolButton);
	nextButton->signal_clicked().connect(sigc::mem_fun(*this, &RenderPreview::onStepForward));
	nextButton->set_icon_widget(*Gtk::manage(new Gtk::Image(Gtk::Stock::MEDIA_NEXT, Gtk::ICON_SIZE_MENU)));
	nextButton->set_tooltip_text(_("Next frame"));

	Gtk::ToolButton* prevButton = Gtk::manage(new Gtk::ToolButton);
	prevButton->signal_clicked().connect(sigc::mem_fun(*this, &RenderPreview::onStepBack));
	prevButton->set_icon_widget(*Gtk::manage(new Gtk::Image(Gtk::Stock::MEDIA_PREVIOUS, Gtk::ICON_SIZE_MENU)));
	prevButton->set_tooltip_text(_("Previous frame"));

	toolbar->insert(*_startButton, 0);
	toolbar->insert(*_pauseButton, 0);
	toolbar->insert(*nextButton, 0);
	toolbar->insert(*prevButton, 0);
	toolbar->insert(*_stopButton, 0);

	_toolHBox->pack_start(*toolbar, true, true, 0);
	_toolHBox->pack_start(*_filtersMenu->getMenuBarWidget(), false, false, 0);

	// Pack into a frame
	add(*vbx);

	// Add an observer to the FilterSystem to get notified about changes
	_filterObserver.reset(new RenderPreviewFilterObserver(*this));

	GlobalFilterSystem().addObserver(_filterObserver);
}

RenderPreview::~RenderPreview()
{
	if (_timer.isEnabled())
	{
		_timer.disable();
	}

	GlobalFilterSystem().removeObserver(_filterObserver);
}

void RenderPreview::onFiltersChanged()
{
	if (!getScene()->root()) return;

	GlobalFilterSystem().updateSubgraph(getScene()->root());

	_glWidget->queueDraw();
}

void RenderPreview::addToolbar(Gtk::Toolbar& toolbar)
{
	_toolHBox->pack_start(toolbar, true, true, 0);
}

void RenderPreview::setSize(int width, int height)
{
	_glWidget->set_size_request(width, height);
}

void RenderPreview::initialisePreview()
{
#ifdef WIN32
	// greebo: Unfortunate hack to fix the grey GL renderviews in the EntityChooser
	// and other windows that are hidden instead of destroyed when closed.
	Gtk::Container* container = get_parent();
	bool wasShown = get_visible();

	if (container != NULL)
	{
		if (wasShown)
		{
			hide();
		}

		container->remove(*this);
		container->add(*this);

		if (wasShown)
		{
			show();
		}
	}

#endif

	// Grab the GL widget with sentry object
	gtkutil::GLWidgetSentry sentry(*_glWidget);

	// Clear the window
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
	glClearColor(0.0, 0.0, 0.0, 0);
	glClearDepth(1.0f);
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

    if (_renderSystem->shaderProgramsAvailable())
    {
        _renderSystem->setShaderProgram(
            RenderSystem::SHADER_PROGRAM_INTERACTION
        );
    }
}

const scene::GraphPtr& RenderPreview::getScene()
{
	if (!_scene)
	{
		_scene = GlobalSceneGraphFactory().createSceneGraph();

		setupSceneGraph();

		associateRenderSystem();
	}

	return _scene;
}

void RenderPreview::setupSceneGraph()
{
	// Set our render time to 0
	_renderSystem->setTime(0);
}

void RenderPreview::associateRenderSystem()
{
	if (_scene && _scene->root())
	{
		_scene->root()->setRenderSystem(_renderSystem);
	}
}

Matrix4 RenderPreview::getProjectionMatrix(float near_z, float far_z, float fieldOfView, int width, int height)
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

Matrix4 RenderPreview::getModelViewMatrix()
{
	// Premultiply with the translations
	Matrix4 modelview;

	{
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();

		glTranslatef(0, 0, _camDist); // camera translation
		glMultMatrixf(_rotation); // post multiply with rotations

		// Load the matrix from openGL
		glGetFloatv(GL_MODELVIEW_MATRIX, modelview);
	}

	return modelview;
}

void RenderPreview::startPlayback()
{
	if (_timer.isEnabled())
	{
		// Timer is already running, just reset the preview time
		_renderSystem->setTime(0);
	}
	else
	{
		// Timer is not enabled, we're paused or stopped
		_timer.enable();
	}

	_pauseButton->set_sensitive(true);
	_stopButton->set_sensitive(true);
}

void RenderPreview::stopPlayback()
{
	_renderSystem->setTime(0);
	_timer.disable();

	_pauseButton->set_sensitive(false);
	_stopButton->set_sensitive(false);

	_glWidget->queue_draw();
}

bool RenderPreview::onPreRender()
{
	return true;
}

RenderStateFlags RenderPreview::getRenderFlagsFill()
{
	return RENDER_COLOURWRITE | 
			RENDER_ALPHATEST | 
			RENDER_BLEND |
			RENDER_CULLFACE |
			RENDER_COLOURARRAY |
            RENDER_OFFSETLINE |
            RENDER_POLYGONSMOOTH |
            RENDER_LINESMOOTH |
            RENDER_VERTEX_COLOUR |
			RENDER_FILL |
			RENDER_LIGHTING |
			RENDER_TEXTURE_2D |
			RENDER_SMOOTH |
			RENDER_SCALED |
			RENDER_FILL |
			RENDER_TEXTURE_CUBEMAP |
			RENDER_BUMP |
			RENDER_PROGRAM |
			RENDER_SCREEN;
}

RenderStateFlags RenderPreview::getRenderFlagsWireframe()
{
	return RENDER_COLOURWRITE |
           RENDER_ALPHATEST |
           RENDER_BLEND |
           RENDER_CULLFACE |
           RENDER_COLOURARRAY |
           RENDER_OFFSETLINE |
           RENDER_POLYGONSMOOTH |
           RENDER_LINESMOOTH |
           RENDER_VERTEX_COLOUR |
		   RENDER_LIGHTING |
		   RENDER_SMOOTH |
		   RENDER_SCALED |
		   RENDER_TEXTURE_CUBEMAP |
		   RENDER_BUMP |
		   RENDER_PROGRAM |
		   RENDER_SCREEN;
}

bool RenderPreview::onGLDraw(GdkEventExpose*)
{
	if (_renderingInProgress) return false; // avoid double-entering this method

	ScopedBoolLock lock(_renderingInProgress); // will be set to false on method exit

	// Create scoped sentry object to swap the GLWidget's buffers
	gtkutil::GLWidgetSentry sentry(*_glWidget);

	glViewport(0, 0, _previewWidth, _previewHeight);

	// Set up the render and clear the drawing area in any case
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Set up the camera
	Matrix4 projection = getProjectionMatrix(0.1f, 10000, PREVIEW_FOV, _previewWidth, _previewHeight);

	// Keep the modelview matrix in the volumetest class up to date
	_volumeTest.setModelView(getModelViewMatrix());
	_volumeTest.setProjection(projection);

	// Pre-Render event
	if (!onPreRender())
	{
		// a return value of false means to cancel rendering
		drawTime();
		return false;
	}

	// Front-end render phase, collect OpenGLRenderable objects from the scene
	getScene()->foreachVisibleNodeInVolume(_volumeTest, _sceneWalker);

	RenderStateFlags flags = getRenderFlagsFill();

	// Launch the back end rendering
	_renderSystem->render(flags, _volumeTest.GetModelview(), projection);

	// Give subclasses an opportunity to render their own on-screen stuff
	onPostRender();

	// Draw the render time
	drawTime();

	return false;
}

void RenderPreview::renderWireFrame()
{
	RenderStateFlags flags = getRenderFlagsWireframe();

	// Set up the camera
	Matrix4 projection = getProjectionMatrix(0.1f, 10000, PREVIEW_FOV, _previewWidth, _previewHeight);

	// Front-end render phase, collect OpenGLRenderable objects from the scene
	getScene()->foreachVisibleNodeInVolume(_volumeTest, _sceneWalker);

	// Launch the back end rendering
	_renderSystem->render(flags, _volumeTest.GetModelview(), projection);
}

bool RenderPreview::onGLMotion(GdkEventMotion* ev)
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

AABB RenderPreview::getSceneBounds()
{
	return AABB(Vector3(0,0,0), Vector3(64,64,64));
}

bool RenderPreview::onGLScroll(GdkEventScroll* ev)
{
	// Scroll increment is a fraction of the AABB radius
	float inc = static_cast<float>(getSceneBounds().getRadius()) * 0.1f;

	if (ev->direction == GDK_SCROLL_UP)
	{
		_camDist += inc;
	}
	else if (ev->direction == GDK_SCROLL_DOWN)
	{
		_camDist -= inc;
	}

	if (!_renderingInProgress)
	{
		_glWidget->queueDraw();
	}

	return false;
}

void RenderPreview::onStart()
{
	startPlayback();
}

void RenderPreview::onPause()
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

void RenderPreview::onStop()
{
	stopPlayback();
}

void RenderPreview::onStepForward()
{
	// Disable the button
	_pauseButton->set_sensitive(false);

	if (_timer.isEnabled())
	{
		_timer.disable();
	}

	_renderSystem->setTime(_renderSystem->getTime() + MSEC_PER_FRAME);
	_glWidget->queue_draw();
}

void RenderPreview::onStepBack()
{
	// Disable the button
	_pauseButton->set_sensitive(false);

	if (_timer.isEnabled())
	{
		_timer.disable();
	}

	if (_renderSystem->getTime() > 0)
	{
		_renderSystem->setTime(_renderSystem->getTime() - MSEC_PER_FRAME);
	}

	_glWidget->queue_draw();
}

void RenderPreview::onSizeAllocate(Gtk::Allocation& allocation)
{
	_previewWidth = allocation.get_width();
	_previewHeight = allocation.get_height();
}

void RenderPreview::drawTime()
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

	GlobalOpenGL().drawString((boost::format("%.3f sec.") % (_renderSystem->getTime() * 0.001f)).str());
}

gboolean RenderPreview::_onFrame(gpointer data)
{
	RenderPreview* self = reinterpret_cast<RenderPreview*>(data);

	if (!self->_renderingInProgress)
	{
		self->_renderSystem->setTime(self->_renderSystem->getTime() + MSEC_PER_FRAME);
		self->_glWidget->queue_draw();
	}

	// Return true, so that the timer gets called again
	return TRUE;
}

} // namespace
