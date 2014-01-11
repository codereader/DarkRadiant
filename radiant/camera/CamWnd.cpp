#include "CamWnd.h"

#include "ibrush.h"
#include "iclipper.h"
#include "iuimanager.h"
#include "ieventmanager.h"
#include "imainframe.h"

#include "gtkutil/GLWidgetSentry.h"
#include <time.h>
#include <boost/format.hpp>

#include "iselectiontest.h"
#include "selectionlib.h"
#include "gamelib.h"
#include "map/Map.h"
#include "CamRenderer.h"
#include "CameraSettings.h"
#include "GlobalCamera.h"
#include "render/RenderStatistics.h"
#include "registry/adaptors.h"
#include "selection/OccludeSelector.h"

#include <boost/bind.hpp>

#include <gtkmm/main.h>
#include <gtkmm/image.h>
#include <gtkmm/radiotoolbutton.h>

namespace
{
    const std::size_t MSEC_PER_FRAME = 16;

    const std::string FAR_CLIP_IN_TEXT = "Move far clip plane closer";
    const std::string FAR_CLIP_OUT_TEXT = "Move far clip plane further away";
    const std::string FAR_CLIP_DISABLED_TEXT = " (currently disabled in preferences)";
}

class ObjectFinder :
    public scene::NodeVisitor
{
    scene::INodePtr _node;
    SelectionTest& _selectionTest;

    // To store the best intersection candidate
    SelectionIntersection _bestIntersection;
public:
    // Constructor
    ObjectFinder(SelectionTest& test) :
        _selectionTest(test)
    {}

    // Return the found node
    const scene::INodePtr& getNode() const {
        return _node;
    }

    // The visitor function
    bool pre(const scene::INodePtr& node) {
        // Check if the node is filtered
        if (node->visible()) {
            SelectionTestablePtr selectionTestable = Node_getSelectionTestable(node);

            if (selectionTestable != NULL) {
                bool occluded;
                selection::OccludeSelector selector(_bestIntersection, occluded);
                selectionTestable->testSelect(selector, _selectionTest);

                if (occluded) {
                    _node = node;
                }
            }
        }
        else {
            return false; // don't traverse filtered nodes
        }

        return true;
    }
};

inline WindowVector windowvector_for_widget_centre(Gtk::Widget& widget)
{
    Gtk::Allocation alloc = widget.get_allocation();
    return WindowVector(static_cast<float>(alloc.get_width() / 2), static_cast<float>(alloc.get_height() / 2));
}

class FloorHeightWalker :
    public scene::NodeVisitor
{
    float _current;
    float& _bestUp;
    float& _bestDown;

public:
    FloorHeightWalker(float current, float& bestUp, float& bestDown) :
        _current(current),
        _bestUp(bestUp),
        _bestDown(bestDown)
    {
		_bestUp = game::current::getValue<float>("/defaults/maxWorldCoord");
        _bestDown = -game::current::getValue<float>("/defaults/maxWorldCoord");
    }

    bool pre(const scene::INodePtr& node) {

        if (!node->visible()) return false; // don't traverse hidden nodes

        if (Node_isBrush(node)) // this node is a floor
        {
            const AABB& aabb = node->worldAABB();

            float floorHeight = aabb.origin.z() + aabb.extents.z();

            if (floorHeight > _current && floorHeight < _bestUp) {
                _bestUp = floorHeight;
            }

            if (floorHeight < _current && floorHeight > _bestDown) {
                _bestDown = floorHeight;
            }

            return false;
        }

        return true;
    }
};

// ---------- CamWnd Implementation --------------------------------------------------

CamWnd::CamWnd() :
    gtkutil::GladeWidgetHolder("CamWnd.glade"),
    _mainWidget(gladeWidget<Gtk::Container>("mainVbox")),
    _id(++_maxId),
    m_view(true),
    m_Camera(&m_view, Callback(boost::bind(&CamWnd::queueDraw, this))),
    m_cameraview(m_Camera, &m_view, Callback(boost::bind(&CamWnd::update, this))),
    m_drawing(false),
    m_bFreeMove(false),
    _camGLWidget(Gtk::manage(new gtkutil::GLWidget(true, "CamWnd"))),
    _timer(MSEC_PER_FRAME, _onFrame, this),
    m_window_observer(NewWindowObserver()),
    m_deferredDraw(boost::bind(&gtkutil::GLWidget::queue_draw, _camGLWidget)),
    m_deferred_motion(boost::bind(&CamWnd::_onDeferredMouseMotion, this, _1, _2, _3))
{
    m_window_observer->setRectangleDrawCallback(
        boost::bind(&CamWnd::updateSelectionBox, this, _1)
    );
    m_window_observer->setView(m_view);

    constructGUIComponents();

    GlobalMap().signal_mapValidityChanged().connect(
        sigc::mem_fun(m_deferredDraw, &DeferredDraw::onMapValidChanged)
    );

    // Deactivate all commands, just to make sure
    disableDiscreteMoveEvents();
    disableFreeMoveEvents();

    // Now add the handlers for the non-freelook mode, the events are activated by this
    addHandlersMove();

    _camGLWidget->signal_scroll_event().connect(sigc::mem_fun(*this, &CamWnd::onMouseScroll));

    // Subscribe to the global scene graph update
    GlobalSceneGraph().addSceneObserver(this);

    // Let the window observer connect its handlers to the GL widget first
    // (before the eventmanager)
    m_window_observer->addObservedWidget(_camGLWidget);

    GlobalEventManager().connect(_camGLWidget);
}

void CamWnd::constructToolbar()
{
    // If lighting is not available, grey out the lighting button
    Gtk::ToggleToolButton* lightingBtn = gladeWidget<Gtk::ToggleToolButton>(
        "lightingBtn"
    );
    if (!GlobalRenderSystem().shaderProgramsAvailable())
    {
        lightingBtn->set_sensitive(false);
    }

    // Listen for render-mode changes, and set the correct active button to
    // start with.
    getCameraSettings()->signalRenderModeChanged().connect(
        sigc::mem_fun(this, &CamWnd::updateActiveRenderModeButton)
    );
    updateActiveRenderModeButton();

    // Connect button signals
    gladeWidget<Gtk::ToggleToolButton>("texturedBtn")->signal_toggled().connect(
        sigc::mem_fun(*this, &CamWnd::onRenderModeButtonsChanged)
    );
    lightingBtn->signal_toggled().connect(
        sigc::mem_fun(*this, &CamWnd::onRenderModeButtonsChanged)
    );
    gladeWidget<Gtk::ToggleToolButton>("flatShadeBtn")->signal_toggled().connect(
        sigc::mem_fun(*this, &CamWnd::onRenderModeButtonsChanged)
    );
    gladeWidget<Gtk::ToggleToolButton>("wireframeBtn")->signal_toggled().connect(
        sigc::mem_fun(*this, &CamWnd::onRenderModeButtonsChanged)
    );

    // Far clip buttons.
    gladeWidget<Gtk::ToolButton>("clipPlaneInButton")->signal_clicked().connect(
        sigc::mem_fun(*this, &CamWnd::farClipPlaneIn)
    );
    gladeWidget<Gtk::ToolButton>("clipPlaneOutButton")->signal_clicked().connect(
        sigc::mem_fun(*this, &CamWnd::farClipPlaneOut)
    );

    setFarClipButtonSensitivity();
    GlobalRegistry().signalForKey(RKEY_ENABLE_FARCLIP).connect(
        sigc::mem_fun(*this, &CamWnd::setFarClipButtonSensitivity)
    );

    gladeWidget<Gtk::ToolButton>("startTimeButton")->signal_clicked().connect(
        sigc::mem_fun(*this, &CamWnd::startRenderTime)
    );
    gladeWidget<Gtk::ToolButton>("stopTimeButton")->signal_clicked().connect(
        sigc::mem_fun(*this, &CamWnd::stopRenderTime)
    );

    // Stop time, initially
    stopRenderTime();

    Gtk::Widget* toolbar = gladeWidget<Gtk::Widget>("camToolbar");

    // Hide the toolbar if requested
    if (!getCameraSettings()->showCameraToolbar())
    {
        toolbar->hide();
        toolbar->set_no_show_all(true);
    }

    // Connect to show/hide registry key
    registry::observeBooleanKey(
       RKEY_SHOW_CAMERA_TOOLBAR,
       sigc::mem_fun(toolbar, &Gtk::Widget::show),
       sigc::mem_fun(toolbar, &Gtk::Widget::hide)
    );
}

void CamWnd::setFarClipButtonSensitivity()
{
    // Only enabled if cubic clipping is enabled.
    bool enabled = registry::getValue<bool>(RKEY_ENABLE_FARCLIP, true);
    gladeWidget<Gtk::Widget>("clipPlaneInButton")->set_sensitive(enabled);
    gladeWidget<Gtk::Widget>("clipPlaneOutButton")->set_sensitive(enabled);

    // Update tooltips so users know why they are disabled
    gladeWidget<Gtk::Widget>("clipPlaneInButton")->set_tooltip_text(
        FAR_CLIP_IN_TEXT + (enabled ? "" : FAR_CLIP_DISABLED_TEXT)
    );
    gladeWidget<Gtk::Widget>("clipPlaneOutButton")->set_tooltip_text(
        FAR_CLIP_OUT_TEXT + (enabled ? "" : FAR_CLIP_DISABLED_TEXT)
    );
}

void CamWnd::constructGUIComponents()
{
    constructToolbar();

    // Set up GL widget
    _camGLWidget->set_events(  Gdk::EXPOSURE_MASK 
                             | Gdk::BUTTON_PRESS_MASK 
                             | Gdk::BUTTON_RELEASE_MASK 
                             | Gdk::POINTER_MOTION_MASK 
                             | Gdk::SCROLL_MASK);
    _camGLWidget->set_flags(Gtk::CAN_FOCUS);
    _camGLWidget->set_size_request(CAMWND_MINSIZE_X, CAMWND_MINSIZE_Y);
    _camGLWidget->property_can_focus() = true;

    _camGLWidget->signal_size_allocate().connect(
        sigc::mem_fun(*this, &CamWnd::onSizeAllocate)
    );
    _camGLWidget->signal_expose_event().connect(
        sigc::mem_fun(*this, &CamWnd::onExpose)
    );

    // Pack GL widget into outer widget
    Gtk::Container* glWidgetFrame = gladeWidget<Gtk::Container>(
        "glWidgetFrame"
    );
    glWidgetFrame->add(*_camGLWidget);
}

CamWnd::~CamWnd()
{
    // Unsubscribe from the global scene graph update
    GlobalSceneGraph().removeSceneObserver(this);

    m_window_observer->removeObservedWidget(_camGLWidget);

    // Disconnect self from EventManager
    GlobalEventManager().disconnect(_camGLWidget);

    if (m_bFreeMove) {
        disableFreeMove();
    }

    removeHandlersMove();

    /*g_signal_handler_disconnect(G_OBJECT(glWidget), m_sizeHandler);
    g_signal_handler_disconnect(G_OBJECT(glWidget), m_exposeHandler);*/

    m_window_observer->release();

    // Notify the camera manager about our destruction
    GlobalCamera().removeCamWnd(_id);
}

void CamWnd::startRenderTime()
{
    if (_timer.isEnabled())
    {
        // Timer is already running, just reset the preview time
        GlobalRenderSystem().setTime(0);
    }
    else
    {
        // Timer is not enabled, we're paused or stopped
        _timer.enable();
    }

    //gladeWidget<Gtk::ToolButton>("startTimeButton")->set_sensitive(false);
    gladeWidget<Gtk::ToolButton>("stopTimeButton")->set_sensitive(true);
}

gboolean CamWnd::_onFrame(gpointer data)
{
    CamWnd* self = reinterpret_cast<CamWnd*>(data);

    if (!self->m_drawing)
    {
        GlobalRenderSystem().setTime(GlobalRenderSystem().getTime() + MSEC_PER_FRAME);

        // Give the UI a chance to react, but don't hang in there forever
        std::size_t maxEventsPerCallback = 5;

        while (Gtk::Main::events_pending() && --maxEventsPerCallback != 0)
        {
            Gtk::Main::iteration();
        }
        
        self->_camGLWidget->queue_draw();
    }

    // Return true, so that the timer gets called again
    return TRUE;
}

void CamWnd::stopRenderTime()
{
    _timer.disable();

    gladeWidget<Gtk::ToolButton>("startTimeButton")->set_sensitive(true);
    gladeWidget<Gtk::ToolButton>("stopTimeButton")->set_sensitive(false);
}

void CamWnd::onRenderModeButtonsChanged()
{
    using Gtk::ToggleToolButton;

    // This function will be called twice, once for the inactivating button and
    // once for the activating button
    if (gladeWidget<ToggleToolButton>("texturedBtn")->get_active())
    {
        getCameraSettings()->setRenderMode(RENDER_MODE_TEXTURED);
    }
    else if (gladeWidget<ToggleToolButton>("wireframeBtn")->get_active())
    {
        getCameraSettings()->setRenderMode(RENDER_MODE_WIREFRAME);
    }
    else if (gladeWidget<ToggleToolButton>("flatShadeBtn")->get_active())
    {
        getCameraSettings()->setRenderMode(RENDER_MODE_SOLID);
    }
    else if (gladeWidget<ToggleToolButton>("lightingBtn")->get_active())
    {
        getCameraSettings()->setRenderMode(RENDER_MODE_LIGHTING);
    }
}

void CamWnd::updateActiveRenderModeButton()
{
    switch (getCameraSettings()->getRenderMode())
    {
    case RENDER_MODE_WIREFRAME:
        gladeWidget<Gtk::ToggleToolButton>("wireframeBtn")->set_active(true);
        break;
    case RENDER_MODE_SOLID:
        gladeWidget<Gtk::ToggleToolButton>("flatShadeBtn")->set_active(true);
        break;
    case RENDER_MODE_TEXTURED:
        gladeWidget<Gtk::ToggleToolButton>("texturedBtn")->set_active(true);
        break;
    case RENDER_MODE_LIGHTING:
        gladeWidget<Gtk::ToggleToolButton>("lightingBtn")->set_active(true);
        break;
    default:
        g_assert(false);
    }
}

int CamWnd::getId() {
    return _id;
}

void CamWnd::jumpToObject(SelectionTest& selectionTest) {
    // Find a suitable target node
    ObjectFinder finder(selectionTest);
    GlobalSceneGraph().root()->traverse(finder);

    if (finder.getNode() != NULL) {
        // A node has been found, get the bounding box
        AABB found = finder.getNode()->worldAABB();

        // Focus the view at the center of the found AABB
        map::Map::focusViews(found.origin, getCameraAngles());
    }
}

void CamWnd::updateSelectionBox(const Rectangle& area)
{
    if (_camGLWidget->is_visible())
    {
        // Get the rectangle and convert it to screen coordinates
        _dragRectangle = area;
        _dragRectangle.toScreenCoords(m_Camera.width, m_Camera.height);

        queueDraw();
    }
}

void CamWnd::changeFloor(const bool up) {
    float current = m_Camera.getOrigin()[2] - 48;
    float bestUp;
    float bestDown;
    FloorHeightWalker walker(current, bestUp, bestDown);
    GlobalSceneGraph().root()->traverse(walker);

    if (up && bestUp != game::current::getValue<float>("/defaults/maxWorldCoord")) {
        current = bestUp;
    }

    if (!up && bestDown != -game::current::getValue<float>("/defaults/maxWorldCoord")) {
        current = bestDown;
    }

    const Vector3& org = m_Camera.getOrigin();
    m_Camera.setOrigin(Vector3(org[0], org[1], current + 48));

    m_Camera.updateModelview();
    update();
    GlobalCamera().movedNotify();
}

void CamWnd::enableFreeMove()
{
    ASSERT_MESSAGE(!m_bFreeMove, "EnableFreeMove: free-move was already enabled");
    m_bFreeMove = true;
    m_Camera.clearMovementFlags(MOVE_ALL);

    removeHandlersMove();

    m_selection_button_press_handler = _camGLWidget->signal_button_press_event().connect(
        sigc::bind(sigc::mem_fun(*this, &CamWnd::selectionButtonPressFreemove), m_window_observer));

    m_selection_button_release_handler = _camGLWidget->signal_button_release_event().connect(
        sigc::bind(sigc::mem_fun(*this, &CamWnd::selectionButtonReleaseFreemove), m_window_observer));

    m_selection_motion_handler = _camGLWidget->signal_motion_notify_event().connect(
        sigc::bind(sigc::mem_fun(*this,& CamWnd::selectionMotionFreemove), m_window_observer));

    if (getCameraSettings()->toggleFreelook())
    {
        m_freelook_button_press_handler = _camGLWidget->signal_button_press_event().connect(
            sigc::mem_fun(*this, &CamWnd::disableFreelookButtonPress));
    }
    else
    {
        m_freelook_button_release_handler = _camGLWidget->signal_button_release_event().connect(
            sigc::mem_fun(*this, &CamWnd::disableFreelookButtonRelease));
    }

    enableFreeMoveEvents();

    // greebo: For entering free move, we need a valid parent window
    assert(_parentWindow);

    _parentWindow->set_focus(*_camGLWidget);

    m_freemove_handle_focusout = _camGLWidget->signal_focus_out_event().connect(sigc::mem_fun(*this, &CamWnd::freeMoveFocusOut));
    _freezePointer.freeze(_parentWindow, sigc::mem_fun(*this, &CamWnd::_onFreelookMotion));

    update();
}

void CamWnd::disableFreeMove()
{
    ASSERT_MESSAGE(m_bFreeMove, "DisableFreeMove: free-move was not enabled");
    m_bFreeMove = false;
    m_Camera.clearMovementFlags(MOVE_ALL);

    disableFreeMoveEvents();

    m_selection_button_press_handler.disconnect();
    m_selection_button_release_handler.disconnect();
    m_selection_motion_handler.disconnect();

    if (getCameraSettings()->toggleFreelook())
    {
        m_freelook_button_press_handler.disconnect();
    }
    else
    {
        m_freelook_button_release_handler.disconnect();
    }

    addHandlersMove();

    assert(_parentWindow);
    _freezePointer.unfreeze(_parentWindow);

    m_freemove_handle_focusout.disconnect();

    update();
}

bool CamWnd::freeMoveEnabled() const {
    return m_bFreeMove;
}

void CamWnd::Cam_Draw()
{
    glViewport(0, 0, m_Camera.width, m_Camera.height);

    // enable depth buffer writes
    glDepthMask(GL_TRUE);

    Vector3 clearColour(0, 0, 0);

    if (getCameraSettings()->getRenderMode() != RENDER_MODE_LIGHTING) 
    {
        clearColour = ColourSchemes().getColour("camera_background");
    }

    glClearColor(clearColour[0], clearColour[1], clearColour[2], 0);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    render::RenderStatistics::Instance().resetStats();

	render::View::resetCullStats();

    glMatrixMode(GL_PROJECTION);

    glLoadMatrixd(m_Camera.projection);

    glMatrixMode(GL_MODELVIEW);

    glLoadMatrixd(m_Camera.modelview);


    // one directional light source directly behind the viewer
    {
        GLfloat inverse_cam_dir[4], ambient[4], diffuse[4];//, material[4];

        ambient[0] = ambient[1] = ambient[2] = 0.4f;
        ambient[3] = 1.0f;
        diffuse[0] = diffuse[1] = diffuse[2] = 0.4f;
        diffuse[3] = 1.0f;
        //material[0] = material[1] = material[2] = 0.8f;
        //material[3] = 1.0f;

        inverse_cam_dir[0] = m_Camera.vpn[0];
        inverse_cam_dir[1] = m_Camera.vpn[1];
        inverse_cam_dir[2] = m_Camera.vpn[2];
        inverse_cam_dir[3] = 0;

        glLightfv(GL_LIGHT0, GL_POSITION, inverse_cam_dir);

        glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
        glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);

        glEnable(GL_LIGHT0);
    }


    // Set the allowed render flags for this view
    unsigned int allowedRenderFlags = RENDER_DEPTHTEST
                                     | RENDER_MASKCOLOUR
                                     | RENDER_DEPTHWRITE
                                     | RENDER_ALPHATEST
                                     | RENDER_BLEND
                                     | RENDER_CULLFACE
                                     | RENDER_OFFSETLINE
                                     | RENDER_VERTEX_COLOUR
                                     | RENDER_POINT_COLOUR;

    // Add mode-specific render flags
    switch (getCameraSettings()->getRenderMode())
    {
        case RENDER_MODE_WIREFRAME:
            break;

        case RENDER_MODE_SOLID:
            allowedRenderFlags |= RENDER_FILL
                                | RENDER_LIGHTING
                                | RENDER_SMOOTH
                                | RENDER_SCALED;

            break;

        case RENDER_MODE_TEXTURED:
            allowedRenderFlags |= RENDER_FILL
                                | RENDER_LIGHTING
                                | RENDER_TEXTURE_2D
                                | RENDER_SMOOTH
                                | RENDER_SCALED;

            break;

        case RENDER_MODE_LIGHTING:
            allowedRenderFlags |= RENDER_FILL
                                | RENDER_LIGHTING
                                | RENDER_TEXTURE_2D
                                | RENDER_TEXTURE_CUBEMAP
                                | RENDER_VERTEX_COLOUR
                                | RENDER_SMOOTH
                                | RENDER_SCALED
                                | RENDER_BUMP
                                | RENDER_PROGRAM;

            break;

        default:
            allowedRenderFlags = 0;

            break;
    }

    if (!getCameraSettings()->solidSelectionBoxes())
    {
        allowedRenderFlags |= RENDER_LINESTIPPLE
                            | RENDER_POLYGONSTIPPLE;
    }

    {
        CamRenderer renderer(allowedRenderFlags, m_state_select2, m_state_select1, m_view.getViewer());

		render::RenderHighlighted::collectRenderablesInScene(renderer, m_view);

        renderer.render(m_Camera.modelview, m_Camera.projection);
    }

    // greebo: Draw the clipper's points (skipping the depth-test)
    {
        glDisable(GL_DEPTH_TEST);

        glColor4f(1, 1, 1, 1);
        glPointSize(5);

        if (GlobalClipper().clipMode()) {
            GlobalClipper().draw(1.0f);
        }

        glPointSize(1);
    }

    // prepare for 2d stuff
    glColor4f(1, 1, 1, 1);

    glDisable(GL_BLEND);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, (float)m_Camera.width, 0, (float)m_Camera.height, -100, 100);

    glScalef(1, -1, 1);
    glTranslatef(0, -(float)m_Camera.height, 0);

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

    // draw the crosshair

    if (m_bFreeMove) {
        glBegin( GL_LINES );
        glVertex2f( (float)m_Camera.width / 2.f, (float)m_Camera.height / 2.f + 6 );
        glVertex2f( (float)m_Camera.width / 2.f, (float)m_Camera.height / 2.f + 2 );
        glVertex2f( (float)m_Camera.width / 2.f, (float)m_Camera.height / 2.f - 6 );
        glVertex2f( (float)m_Camera.width / 2.f, (float)m_Camera.height / 2.f - 2 );
        glVertex2f( (float)m_Camera.width / 2.f + 6, (float)m_Camera.height / 2.f );
        glVertex2f( (float)m_Camera.width / 2.f + 2, (float)m_Camera.height / 2.f );
        glVertex2f( (float)m_Camera.width / 2.f - 6, (float)m_Camera.height / 2.f );
        glVertex2f( (float)m_Camera.width / 2.f - 2, (float)m_Camera.height / 2.f );
        glEnd();
    }

    glRasterPos3f(1.0f, static_cast<float>(m_Camera.height) - 1.0f, 0.0f);

    GlobalOpenGL().drawString(render::RenderStatistics::Instance().getStatString());

    glRasterPos3f(1.0f, static_cast<float>(m_Camera.height) - 11.0f, 0.0f);

	GlobalOpenGL().drawString(render::View::getCullStats());

    drawTime();

    // Draw the selection drag rectangle
    if (!_dragRectangle.empty())
    {
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(0, (float)m_Camera.width, 0, (float)m_Camera.height, -100, 100);

        glScalef(1, -1, 1);
        glTranslatef(0, -(float)m_Camera.height, 0);

        // Define the blend function for transparency
        glEnable(GL_BLEND);
        glBlendColor(0, 0, 0, 0.2f);
        glBlendFunc(GL_CONSTANT_ALPHA_EXT, GL_ONE_MINUS_CONSTANT_ALPHA_EXT);

        Vector3 dragBoxColour = ColourSchemes().getColour("drag_selection");
        glColor3dv(dragBoxColour);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

        // Correct the glScale and glTranslate calls above
        Rectangle rect = _dragRectangle;

        double width = rect.max.x() - rect.min.x();
        double height = rect.max.y() - rect.min.y();

        rect.min.y() = m_Camera.height - rect.min.y();
        height *= -1;

        // The transparent fill rectangle
        glBegin(GL_QUADS);
        glVertex2d(rect.min.x(), rect.min.y() + height);
        glVertex2d(rect.min.x() + width, rect.min.y() + height);
        glVertex2d(rect.min.x() + width, rect.min.y());
        glVertex2d(rect.min.x(), rect.min.y());
        glEnd();

        // The solid borders
        glColor3f(0.9f, 0.9f, 0.9f);
        glBlendColor(0, 0, 0, 0.8f);

        glBegin(GL_LINE_LOOP);
        glVertex2d(rect.min.x(), rect.min.y() + height);
        glVertex2d(rect.min.x() + width, rect.min.y() + height);
        glVertex2d(rect.min.x() + width, rect.min.y());
        glVertex2d(rect.min.x(), rect.min.y());
        glEnd();

        glDisable(GL_BLEND);
    }

    // bind back to the default texture so that we don't have problems
    // elsewhere using/modifying texture maps between contexts
    glBindTexture( GL_TEXTURE_2D, 0 );
}

void CamWnd::draw()
{
    if (m_drawing) return;

    m_drawing = true;

    // Scoped object handling the GL context switching
    gtkutil::GLWidgetSentry sentry(*_camGLWidget);

    if (GlobalMap().isValid() && GlobalMainFrame().screenUpdatesEnabled()) {
        GlobalOpenGL().assertNoErrors();
        Cam_Draw();
        GlobalOpenGL().assertNoErrors();
    }

    m_drawing = false;
}

void CamWnd::benchmark() {
    double dStart = clock() / 1000.0;

    for (int i=0 ; i < 100 ; i++) {
        Vector3 angles;
        angles[CAMERA_ROLL] = 0;
        angles[CAMERA_PITCH] = 0;
        angles[CAMERA_YAW] = static_cast<double>(i * (360.0 / 100.0));
        setCameraAngles(angles);
    }

    double dEnd = clock() / 1000.0;

    rMessage() << (boost::format("%5.2lf") % (dEnd - dStart)) << " seconds\n";
}

void CamWnd::onSceneGraphChange() {
    // Just pass the call to the update method
    update();
}

// ----------------------------------------------------------

void CamWnd::enableFreeMoveEvents()
{
    GlobalEventManager().enableEvent("CameraFreeMoveForward");
    GlobalEventManager().enableEvent("CameraFreeMoveBack");
    GlobalEventManager().enableEvent("CameraFreeMoveLeft");
    GlobalEventManager().enableEvent("CameraFreeMoveRight");
    GlobalEventManager().enableEvent("CameraFreeMoveUp");
    GlobalEventManager().enableEvent("CameraFreeMoveDown");
}

void CamWnd::disableFreeMoveEvents()
{
    GlobalEventManager().disableEvent("CameraFreeMoveForward");
    GlobalEventManager().disableEvent("CameraFreeMoveBack");
    GlobalEventManager().disableEvent("CameraFreeMoveLeft");
    GlobalEventManager().disableEvent("CameraFreeMoveRight");
    GlobalEventManager().disableEvent("CameraFreeMoveUp");
    GlobalEventManager().disableEvent("CameraFreeMoveDown");
}

void CamWnd::enableDiscreteMoveEvents()
{
    GlobalEventManager().enableEvent("CameraForward");
    GlobalEventManager().enableEvent("CameraBack");
    GlobalEventManager().enableEvent("CameraLeft");
    GlobalEventManager().enableEvent("CameraRight");
    GlobalEventManager().enableEvent("CameraStrafeRight");
    GlobalEventManager().enableEvent("CameraStrafeLeft");
    GlobalEventManager().enableEvent("CameraUp");
    GlobalEventManager().enableEvent("CameraDown");
    GlobalEventManager().enableEvent("CameraAngleUp");
    GlobalEventManager().enableEvent("CameraAngleDown");
}

void CamWnd::disableDiscreteMoveEvents()
{
    GlobalEventManager().disableEvent("CameraForward");
    GlobalEventManager().disableEvent("CameraBack");
    GlobalEventManager().disableEvent("CameraLeft");
    GlobalEventManager().disableEvent("CameraRight");
    GlobalEventManager().disableEvent("CameraStrafeRight");
    GlobalEventManager().disableEvent("CameraStrafeLeft");
    GlobalEventManager().disableEvent("CameraUp");
    GlobalEventManager().disableEvent("CameraDown");
    GlobalEventManager().disableEvent("CameraAngleUp");
    GlobalEventManager().disableEvent("CameraAngleDown");
}

void CamWnd::addHandlersMove()
{
    m_selection_button_press_handler = _camGLWidget->signal_button_press_event().connect(
        sigc::bind(sigc::mem_fun(*this, &CamWnd::selectionButtonPress), m_window_observer));

    m_selection_button_release_handler = _camGLWidget->signal_button_release_event().connect(
        sigc::bind(sigc::mem_fun(*this, &CamWnd::selectionButtonRelease), m_window_observer));

    m_selection_motion_handler = _camGLWidget->signal_motion_notify_event().connect(sigc::mem_fun(m_deferred_motion, &gtkutil::DeferredMotion::onMouseMotion));

    m_freelook_button_press_handler = _camGLWidget->signal_button_press_event().connect(
        sigc::mem_fun(*this, &CamWnd::enableFreelookButtonPress));

    // Enable either the free-look movement commands or the discrete ones,
    // depending on the selection
    if (getCameraSettings()->discreteMovement())
    {
        enableDiscreteMoveEvents();
    }
    else
    {
        enableFreeMoveEvents();
    }
}

void CamWnd::removeHandlersMove()
{
    m_selection_button_press_handler.disconnect();
    m_selection_button_release_handler.disconnect();
    m_selection_motion_handler.disconnect();
    m_freelook_button_press_handler.disconnect();

    // Disable either the free-look movement commands or the discrete ones, depending on the selection
    if (getCameraSettings()->discreteMovement())
    {
        disableDiscreteMoveEvents();
    }
    else
    {
        disableFreeMoveEvents();
    }
}

void CamWnd::update()
{
    queueDraw();
}

Camera& CamWnd::getCamera() {
    return m_Camera;
}

void CamWnd::captureStates() {
    m_state_select1 = GlobalRenderSystem().capture("$CAM_HIGHLIGHT");
    m_state_select2 = GlobalRenderSystem().capture("$CAM_OVERLAY");
}

void CamWnd::releaseStates() {
    m_state_select1 = ShaderPtr();
    m_state_select2 = ShaderPtr();
}

void CamWnd::queueDraw() {
    if (m_drawing) {
        return;
    }

    m_deferredDraw.draw();
}

Gtk::Widget* CamWnd::getWidget() const
{
    return _mainWidget;
}

const Glib::RefPtr<Gtk::Window>& CamWnd::getParent() const
{
    return _parentWindow;
}

void CamWnd::setContainer(const Glib::RefPtr<Gtk::Window>& newParent)
{
    if (newParent == _parentWindow)
    {
        // Do nothing if no change required
        return;
    }

    if (_parentWindow)
    {
        // Parent change, disconnect first
        m_window_observer->removeObservedWidget(_parentWindow);
        GlobalEventManager().disconnect(_parentWindow->get_toplevel());

        if (m_bFreeMove)
        {
            disableFreeMove();
        }

        _parentWindow.reset();
    }

    if (newParent)
    {
        _parentWindow = newParent;

        m_window_observer->addObservedWidget(_parentWindow);
        GlobalEventManager().connect(_parentWindow->get_toplevel());
    }
}

Vector3 CamWnd::getCameraOrigin() const {
    return m_Camera.getOrigin();
}

void CamWnd::setCameraOrigin(const Vector3& origin) {
    m_Camera.setOrigin(origin);
}

Vector3 CamWnd::getCameraAngles() const {
    return m_Camera.getAngles();
}

void CamWnd::setCameraAngles(const Vector3& angles) {
    m_Camera.setAngles(angles);
}

void CamWnd::farClipPlaneOut() 
{
    getCameraSettings()->setCubicScale( getCameraSettings()->cubicScale() + 1 );

    m_Camera.updateProjection();
    update();
}

void CamWnd::farClipPlaneIn() 
{
    getCameraSettings()->setCubicScale( getCameraSettings()->cubicScale() - 1 );

    m_Camera.updateProjection();
    update();
}

void CamWnd::onSizeAllocate(Gtk::Allocation& allocation)
{
    getCamera().width = allocation.get_width();
    getCamera().height = allocation.get_height();
    getCamera().updateProjection();

    m_window_observer->onSizeChanged(getCamera().width, getCamera().height);

    queueDraw();
}

bool CamWnd::onExpose(GdkEventExpose* ev)
{
    draw();

    return false;
}

bool CamWnd::onMouseScroll(GdkEventScroll* ev)
{
    // Set the GTK focus to this widget
    _camGLWidget->grab_focus();

    // Determine the direction we are moving.
    if (ev->direction == GDK_SCROLL_UP)
    {
        getCamera().freemoveUpdateAxes();
        setCameraOrigin(getCameraOrigin() + getCamera().forward * static_cast<float>(getCameraSettings()->movementSpeed()));
    }
    else if (ev->direction == GDK_SCROLL_DOWN)
    {
        getCamera().freemoveUpdateAxes();
        setCameraOrigin(getCameraOrigin() + getCamera().forward * (-static_cast<float>(getCameraSettings()->movementSpeed())));
    }

    return false;
}

bool CamWnd::enableFreelookButtonPress(GdkEventButton* ev)
{
    if (ev->type == GDK_BUTTON_PRESS)
    {
        if (GlobalEventManager().MouseEvents().stateMatchesCameraViewEvent(ui::camEnableFreeLookMode, ev))
        {
            enableFreeMove();
            return true;
        }
    }

    return false;
}

bool CamWnd::disableFreelookButtonPress(GdkEventButton* ev)
{
    if (ev->type == GDK_BUTTON_PRESS)
    {
        if (GlobalEventManager().MouseEvents().stateMatchesCameraViewEvent(ui::camDisableFreeLookMode, ev))
        {
            disableFreeMove();
            return true;
        }
    }

    return false;
}

bool CamWnd::disableFreelookButtonRelease(GdkEventButton* ev)
{
    if (ev->type == GDK_BUTTON_RELEASE)
    {
        if (GlobalEventManager().MouseEvents().stateMatchesCameraViewEvent(ui::camDisableFreeLookMode, ev))
        {
            disableFreeMove();
            return true;
        }
    }

    return false;
}

bool CamWnd::selectionButtonPress(GdkEventButton* ev, SelectionSystemWindowObserver* observer)
{
    // Set the GTK focus to this widget
    _camGLWidget->grab_focus();

    // Check for the correct event type
    if (ev->type == GDK_BUTTON_PRESS)
    {
        observer->onMouseDown(WindowVector(ev->x, ev->y), ev);
    }

    return false;
}

bool CamWnd::selectionButtonRelease(GdkEventButton* ev, SelectionSystemWindowObserver* observer)
{
    if (ev->type == GDK_BUTTON_RELEASE)
    {
        observer->onMouseUp(WindowVector(ev->x, ev->y), ev);
    }

    return false;
}

bool CamWnd::selectionButtonPressFreemove(GdkEventButton* ev, SelectionSystemWindowObserver* observer)
{
    // Check for the correct event type
    if (ev->type == GDK_BUTTON_PRESS)
    {
        observer->onMouseDown(windowvector_for_widget_centre(*_camGLWidget), ev);
    }

    return false;
}

bool CamWnd::selectionButtonReleaseFreemove(GdkEventButton* ev, SelectionSystemWindowObserver* observer)
{
    if (ev->type == GDK_BUTTON_RELEASE)
    {
        observer->onMouseUp(windowvector_for_widget_centre(*_camGLWidget), ev);
    }

    return false;
}

bool CamWnd::selectionMotionFreemove(GdkEventMotion* ev, SelectionSystemWindowObserver* observer)
{
    observer->onMouseMotion(windowvector_for_widget_centre(*_camGLWidget), ev->state);

    return false;
}

bool CamWnd::freeMoveFocusOut(GdkEventFocus* ev)
{
    // Disable free look mode when focus is lost
    disableFreeMove();
    return false;
}

void CamWnd::_onDeferredMouseMotion(gdouble x, gdouble y, guint state)
{
    m_window_observer->onMouseMotion(WindowVector(x, y), state);
}

void CamWnd::_onFreelookMotion(int x, int y, guint state)
{
    m_Camera.m_mouseMove.onMouseMotionDelta(x, y, state);
    m_Camera.m_strafe = GlobalEventManager().MouseEvents().strafeActive(state);

    if (m_Camera.m_strafe)
    {
        m_Camera.m_strafe_forward = GlobalEventManager().MouseEvents().strafeForwardActive(state);
    }
    else
    {
        m_Camera.m_strafe_forward = false;
    }
}

void CamWnd::drawTime()
{
    if (GlobalRenderSystem().getTime() == 0)
    {
        return;
    }

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, static_cast<float>(m_Camera.width), 0, static_cast<float>(m_Camera.height), -100, 100);

    glScalef(1, -1, 1);
    glTranslatef(static_cast<float>(m_Camera.width) - 90, -static_cast<float>(m_Camera.height), 0);

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

    glColor3f(1.f, 1.f, 1.f);
    glLineWidth(1);

    glRasterPos3f(1.0f, static_cast<float>(m_Camera.height) - 1.0f, 0.0f);

    std::size_t time = GlobalRenderSystem().getTime();
    GlobalOpenGL().drawString((boost::format("Time: %.3f sec.") % (time * 0.001f)).str());
}

void CamWnd::connectWindowStateEvent(Gtk::Window& window)
{
    // Connect to the window-state-event signal
    _windowStateConn = window.signal_window_state_event().connect(
        sigc::mem_fun(*this, &CamWnd::onWindowStateEvent)
    );
}

void CamWnd::disconnectWindowStateEvent()
{
    _windowStateConn.disconnect();
}

bool CamWnd::onWindowStateEvent(GdkEventWindowState* ev)
{
    if ((ev->changed_mask & (GDK_WINDOW_STATE_ICONIFIED|GDK_WINDOW_STATE_WITHDRAWN)) != 0)
    {
        // Now let's see what the new state of the window is
        if ((ev->new_window_state & (GDK_WINDOW_STATE_ICONIFIED|GDK_WINDOW_STATE_WITHDRAWN)) == 0)
        {
            // Window got maximised again, re-add the GL widget to fix it from going gray
            Gtk::Widget* glWidget = getWidget();

            // greebo: Unfortunate hack to fix the grey GL renderviews in Win32
            Gtk::Container* container = glWidget->get_parent();

            if (container != NULL)
            {
                glWidget->reference();
                container->remove(*glWidget);
                container->add(*glWidget);
                glWidget->unreference();
            }
        }
    }

    return false;
}

// -------------------------------------------------------------------------------

ShaderPtr CamWnd::m_state_select1;
ShaderPtr CamWnd::m_state_select2;
int CamWnd::_maxId = 0;
