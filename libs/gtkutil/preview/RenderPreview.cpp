#include "RenderPreview.h"

#include "ifilter.h"
#include "i18n.h"
#include "igl.h"
#include "iscenegraphfactory.h"
#include "irendersystemfactory.h"
#include "iuimanager.h"

#include "math/AABB.h"
#include "util/ScopedBoolLock.h"

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

    // Widget names
    const std::string BOTTOM_BOX("bottomBox");
    const std::string PAUSE_BUTTON("pauseButton");
    const std::string STOP_BUTTON("stopButton");
}

RenderPreview::RenderPreview(bool enableAnimation) :
    GladeWidgetHolder("RenderPreview.glade"),
    _glWidget(Gtk::manage(new gtkutil::GLWidget(true, "RenderPreview"))),
    _renderSystem(GlobalRenderSystemFactory().createRenderSystem()),
    _sceneWalker(_renderer, _volumeTest),
    _renderingInProgress(false),
    _timer(MSEC_PER_FRAME, _onFrame, this),
    _previewWidth(0),
    _previewHeight(0),
    _filtersMenu(GlobalUIManager().createFilterMenu())
{
    // Get main glade vbox and pack into self (frame)
    Gtk::Box* main = gladeWidget<Gtk::Box>("main");
    add(*main);

    // Insert GL widget
    main->pack_start(*_glWidget, true, true, 0);

    // Connect up the signals from the GL widget
    _glWidget->set_events(Gdk::EXPOSURE_MASK
                        | Gdk::BUTTON_PRESS_MASK
                        | Gdk::BUTTON_RELEASE_MASK
                        | Gdk::POINTER_MOTION_MASK
                        | Gdk::SCROLL_MASK);

    _glWidget->signal_expose_event().connect(
        sigc::hide(sigc::mem_fun(this, &RenderPreview::drawPreview))
    );
    _glWidget->signal_motion_notify_event().connect(
        sigc::mem_fun(this, &RenderPreview::onGLMotion)
    );
    _glWidget->signal_scroll_event().connect(
        sigc::mem_fun(this, &RenderPreview::onGLScroll)
    );
    _glWidget->signal_size_allocate().connect(
        sigc::mem_fun(this, &RenderPreview::onSizeAllocate)
    );

    // Set up the toolbar
    if (enableAnimation)
    {
        connectToolbarSignals();
    }
    else
    {
        Gtk::Widget* toolbar = gladeWidget<Gtk::Widget>("animationToolbar");
        Gtk::Container* parent = toolbar->get_parent();
        g_assert(parent);
        parent->remove(*toolbar);
        delete toolbar; // no longer managed
    }

    // Add filters menu to end of bottom hbox
    Gtk::Box* bottom = gladeWidget<Gtk::Box>(BOTTOM_BOX);
    bottom->pack_start(*_filtersMenu->getMenuBarWidget(), false, false, 0);

    // Get notified of filter changes
    GlobalFilterSystem().filtersChangedSignal().connect(
        sigc::mem_fun(this, &RenderPreview::filtersChanged)
    );
}

void RenderPreview::connectToolbarSignals()
{
    gladeWidget<Gtk::ToolButton>("startButton")->signal_clicked().connect(
        sigc::mem_fun(this, &RenderPreview::startPlayback)
    );
    gladeWidget<Gtk::ToolButton>(PAUSE_BUTTON)->signal_clicked().connect(
        sigc::mem_fun(this, &RenderPreview::onPause)
    );
    gladeWidget<Gtk::ToolButton>(STOP_BUTTON)->signal_clicked().connect(
        sigc::mem_fun(this, &RenderPreview::stopPlayback)
    );
    gladeWidget<Gtk::ToolButton>("nextButton")->signal_clicked().connect(
        sigc::mem_fun(this, &RenderPreview::onStepForward)
    );
    gladeWidget<Gtk::ToolButton>("prevButton")->signal_clicked().connect(
        sigc::mem_fun(this, &RenderPreview::onStepBack)
    );
}

RenderPreview::~RenderPreview()
{
    if (_timer.isEnabled())
    {
        _timer.disable();
    }
}

void RenderPreview::filtersChanged()
{
    if (!getScene()->root()) return;

    GlobalFilterSystem().updateSubgraph(getScene()->root());
    queue_draw();
}

void RenderPreview::addToolbar(Gtk::Toolbar& toolbar)
{
    gladeWidget<Gtk::Box>(BOTTOM_BOX)->pack_start(toolbar, true, true, 0);
}

void RenderPreview::queueDraw()
{
    _glWidget->queue_draw();
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
        glMultMatrixd(_rotation); // post multiply with rotations

        // Load the matrix from openGL
        glGetDoublev(GL_MODELVIEW_MATRIX, modelview);
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

    gladeWidget<Gtk::Widget>(PAUSE_BUTTON)->set_sensitive(true);
    gladeWidget<Gtk::Widget>(STOP_BUTTON)->set_sensitive(true);
}

void RenderPreview::stopPlayback()
{
    _renderSystem->setTime(0);
    _timer.disable();

    gladeWidget<Gtk::Widget>(PAUSE_BUTTON)->set_sensitive(false);
    gladeWidget<Gtk::Widget>(STOP_BUTTON)->set_sensitive(false);

    _glWidget->queue_draw();
}

bool RenderPreview::onPreRender()
{
    return true;
}

RenderStateFlags RenderPreview::getRenderFlagsFill()
{
    return  RENDER_MASKCOLOUR |
            RENDER_ALPHATEST |
            RENDER_BLEND |
            RENDER_CULLFACE |
            RENDER_OFFSETLINE |
            RENDER_VERTEX_COLOUR |
            RENDER_FILL |
            RENDER_LIGHTING |
            RENDER_TEXTURE_2D |
            RENDER_SMOOTH |
            RENDER_SCALED |
            RENDER_FILL |
            RENDER_TEXTURE_CUBEMAP |
            RENDER_BUMP |
            RENDER_PROGRAM;
}

RenderStateFlags RenderPreview::getRenderFlagsWireframe()
{
    return RENDER_MASKCOLOUR |
           RENDER_ALPHATEST |
           RENDER_BLEND |
           RENDER_CULLFACE |
           RENDER_OFFSETLINE |
           RENDER_VERTEX_COLOUR |
           RENDER_LIGHTING |
           RENDER_SMOOTH |
           RENDER_SCALED |
           RENDER_TEXTURE_CUBEMAP |
           RENDER_BUMP |
           RENDER_PROGRAM;
}

bool RenderPreview::drawPreview()
{
    if (_renderingInProgress) return false; // avoid double-entering this method

    util::ScopedBoolLock lock(_renderingInProgress);

    // Create scoped sentry object to swap the GLWidget's buffers
    gtkutil::GLWidgetSentry sentry(*_glWidget);

    glViewport(0, 0, _previewWidth, _previewHeight);

    // Set up the render and clear the drawing area in any case
    glDepthMask(GL_TRUE);
    glClearColor(0.3f, 0.3f, 0.3f, 1.0f);
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
        if (_glWidget->makeCurrent())
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

            _glWidget->queue_draw(); // trigger the GLDraw method to draw the actual model
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
        _glWidget->queue_draw();
    }

    return false;
}

void RenderPreview::onPause()
{
    // Disable the button
    gladeWidget<Gtk::Widget>(PAUSE_BUTTON)->set_sensitive(false);

    if (_timer.isEnabled())
    {
        _timer.disable();
    }
    else
    {
        _timer.enable(); // re-enable playback
    }
}

void RenderPreview::onStepForward()
{
    // Disable the button
    gladeWidget<Gtk::Widget>(PAUSE_BUTTON)->set_sensitive(false);

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
    gladeWidget<Gtk::Widget>(PAUSE_BUTTON)->set_sensitive(false);

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
