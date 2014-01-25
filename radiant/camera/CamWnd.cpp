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

#include "util/ScopedBoolLock.h"
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

inline WindowVector windowvector_for_widget_centre(wxutil::GLWidget& widget)
{
    wxSize size = widget.GetSize();
	return WindowVector(static_cast<float>(size.GetWidth() / 2), static_cast<float>(size.GetHeight() / 2));
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

CamWnd::CamWnd(wxWindow* parent) :
    gtkutil::GladeWidgetHolder("CamWnd.glade"),
    _mainWidget(gladeWidget<Gtk::Container>("mainVbox")),
	_mainWxWidget(loadNamedPanel(parent, "CamWndPanel")),
    _id(++_maxId),
    _view(true),
    _camera(&_view, Callback(boost::bind(&CamWnd::queueDraw, this))),
    _cameraView(_camera, &_view, Callback(boost::bind(&CamWnd::update, this))),
    _drawing(false),
    _freeMoveEnabled(false),
    _camGLWidget(Gtk::manage(new gtkutil::GLWidget(true, "CamWnd"))),
	_wxGLWidget(new wxutil::GLWidget(_mainWxWidget, boost::bind(&CamWnd::onRender, this))),
    _timer(MSEC_PER_FRAME, _onFrame, this),
    _windowObserver(NewWindowObserver()),
    _deferredDraw(boost::bind(&CamWnd::performDeferredDraw, this)),
	_deferredMouseMotion(boost::bind(&CamWnd::onGLMouseMove, this, _1, _2, _3))
{
    _windowObserver->setRectangleDrawCallback(
        boost::bind(&CamWnd::updateSelectionBox, this, _1)
    );
    _windowObserver->setView(_view);

    constructGUIComponents();

    GlobalMap().signal_mapValidityChanged().connect(
        sigc::mem_fun(_deferredDraw, &DeferredDraw::onMapValidChanged)
    );

    // Deactivate all commands, just to make sure
    disableDiscreteMoveEvents();
    disableFreeMoveEvents();

    // Now add the handlers for the non-freelook mode, the events are activated by this
    addHandlersMove();

	_wxFreezePointer.connectMouseEvents(
		boost::bind(&CamWnd::onGLMouseButtonPressFreeMove, this, _1),
		boost::bind(&CamWnd::onGLMouseButtonReleaseFreeMove, this, _1),
		boost::bind(&CamWnd::onGLMouseMoveFreeMove, this, _1));

    // Subscribe to the global scene graph update
    GlobalSceneGraph().addSceneObserver(this);

    // Let the window observer connect its handlers to the GL widget first
    // (before the eventmanager)
	_windowObserver->addObservedWidget(*_wxGLWidget);

	GlobalEventManager().connect(*_wxGLWidget);
}

wxWindow* CamWnd::getMainWidget() const
{ 
	return _mainWxWidget;
}

void CamWnd::constructToolbar()
{
	// If lighting is not available, grey out the lighting button
	wxToolBar* camToolbar = dynamic_cast<wxToolBar*>(_mainWxWidget->FindWindow("CamToolbar"));

	const wxToolBarToolBase* wireframeBtn = getToolBarToolByLabel(camToolbar, "wireframeBtn");
	const wxToolBarToolBase* flatShadeBtn = getToolBarToolByLabel(camToolbar, "flatShadeBtn");
	const wxToolBarToolBase* texturedBtn = getToolBarToolByLabel(camToolbar, "texturedBtn");
	const wxToolBarToolBase* lightingBtn = getToolBarToolByLabel(camToolbar, "lightingBtn");

    if (!GlobalRenderSystem().shaderProgramsAvailable())
    {
        //lightingBtn->set_sensitive(false);
		camToolbar->EnableTool(lightingBtn->GetId(), false);
    }

    // Listen for render-mode changes, and set the correct active button to
    // start with.
    getCameraSettings()->signalRenderModeChanged().connect(
        sigc::mem_fun(this, &CamWnd::updateActiveRenderModeButton)
    );
    updateActiveRenderModeButton();

    // Connect button signals
	_mainWxWidget->GetParent()->Connect(wireframeBtn->GetId(), wxEVT_COMMAND_TOOL_CLICKED, 
		wxCommandEventHandler(CamWnd::onRenderModeButtonsChanged), NULL, this);
	_mainWxWidget->GetParent()->Connect(flatShadeBtn->GetId(), wxEVT_COMMAND_TOOL_CLICKED, 
		wxCommandEventHandler(CamWnd::onRenderModeButtonsChanged), NULL, this);
	_mainWxWidget->GetParent()->Connect(texturedBtn->GetId(), wxEVT_COMMAND_TOOL_CLICKED, 
		wxCommandEventHandler(CamWnd::onRenderModeButtonsChanged), NULL, this);
	_mainWxWidget->GetParent()->Connect(lightingBtn->GetId(), wxEVT_COMMAND_TOOL_CLICKED, 
		wxCommandEventHandler(CamWnd::onRenderModeButtonsChanged), NULL, this);

    // Far clip buttons.
	wxToolBar* miscToolbar = static_cast<wxToolBar*>(_mainWxWidget->FindWindow("MiscToolbar"));

	const wxToolBarToolBase* clipPlaneInButton = getToolBarToolByLabel(miscToolbar, "clipPlaneInButton");
	const wxToolBarToolBase* clipPlaneOutButton = getToolBarToolByLabel(miscToolbar, "clipPlaneOutButton");

	_mainWxWidget->GetParent()->Connect(clipPlaneInButton->GetId(), wxEVT_COMMAND_TOOL_CLICKED, 
		wxCommandEventHandler(CamWnd::onFarClipPlaneInClick), NULL, this);
	_mainWxWidget->GetParent()->Connect(clipPlaneOutButton->GetId(), wxEVT_COMMAND_TOOL_CLICKED, 
		wxCommandEventHandler(CamWnd::onFarClipPlaneOutClick), NULL, this);

    setFarClipButtonSensitivity();

    GlobalRegistry().signalForKey(RKEY_ENABLE_FARCLIP).connect(
        sigc::mem_fun(*this, &CamWnd::setFarClipButtonSensitivity)
    );

	const wxToolBarToolBase* startTimeButton = getToolBarToolByLabel(miscToolbar, "startTimeButton");
	const wxToolBarToolBase* stopTimeButton = getToolBarToolByLabel(miscToolbar, "stopTimeButton");

	_mainWxWidget->GetParent()->Connect(startTimeButton->GetId(), wxEVT_COMMAND_TOOL_CLICKED, 
		wxCommandEventHandler(CamWnd::onStartTimeButtonClick), NULL, this);
	_mainWxWidget->GetParent()->Connect(stopTimeButton->GetId(), wxEVT_COMMAND_TOOL_CLICKED, 
		wxCommandEventHandler(CamWnd::onStopTimeButtonClick), NULL, this);

    // Stop time, initially
    stopRenderTime();

    // Hide the toolbar if requested
    if (!getCameraSettings()->showCameraToolbar())
    {
        camToolbar->Hide();
    }

    // Connect to show/hide registry key
    registry::observeBooleanKey(
       RKEY_SHOW_CAMERA_TOOLBAR,
       sigc::hide_return(sigc::bind(sigc::mem_fun(camToolbar, &wxWindowBase::Show), true)),
       sigc::hide_return(sigc::mem_fun(camToolbar, &wxWindowBase::Hide))
    );
}

void CamWnd::setFarClipButtonSensitivity()
{
    // Only enabled if cubic clipping is enabled.
    bool enabled = registry::getValue<bool>(RKEY_ENABLE_FARCLIP, true);

	wxToolBar* miscToolbar = static_cast<wxToolBar*>(_mainWxWidget->FindWindow("MiscToolbar"));

	wxToolBarToolBase* clipPlaneInButton = 
		const_cast<wxToolBarToolBase*>(getToolBarToolByLabel(miscToolbar, "clipPlaneInButton"));
	wxToolBarToolBase* clipPlaneOutButton = 
		const_cast<wxToolBarToolBase*>(getToolBarToolByLabel(miscToolbar, "clipPlaneOutButton"));

	miscToolbar->EnableTool(clipPlaneInButton->GetId(), enabled);
	miscToolbar->EnableTool(clipPlaneOutButton->GetId(), enabled);

    // Update tooltips so users know why they are disabled
	clipPlaneInButton->SetShortHelp(FAR_CLIP_IN_TEXT + (enabled ? "" : FAR_CLIP_DISABLED_TEXT));
	clipPlaneOutButton->SetShortHelp(FAR_CLIP_OUT_TEXT + (enabled ? "" : FAR_CLIP_DISABLED_TEXT));
}

void CamWnd::constructGUIComponents()
{
    constructToolbar();

	// Set up wxGL widget
	_wxGLWidget->SetMinClientSize(wxSize(CAMWND_MINSIZE_X, CAMWND_MINSIZE_Y));
	_wxGLWidget->Connect(wxEVT_SIZE, wxSizeEventHandler(CamWnd::onGLResize), NULL, this);
	_wxGLWidget->Connect(wxEVT_MOUSEWHEEL, wxMouseEventHandler(CamWnd::onMouseScroll), NULL, this);

	_mainWxWidget->GetSizer()->Add(_wxGLWidget, 1, wxEXPAND); 
}

CamWnd::~CamWnd()
{
    // Unsubscribe from the global scene graph update
    GlobalSceneGraph().removeSceneObserver(this);

	_windowObserver->removeObservedWidget(*_wxGLWidget);

	GlobalEventManager().disconnect(*_wxGLWidget);

    if (_freeMoveEnabled) {
        disableFreeMove();
    }

    removeHandlersMove();

    _windowObserver->release();

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

	wxToolBar* miscToolbar = static_cast<wxToolBar*>(_mainWxWidget->FindWindow("MiscToolbar"));

	const wxToolBarToolBase* stopTimeButton = getToolBarToolByLabel(miscToolbar, "stopTimeButton");
	miscToolbar->EnableTool(stopTimeButton->GetId(), true);
}

void CamWnd::onStartTimeButtonClick(wxCommandEvent& ev)
{
	startRenderTime();
}

void CamWnd::onStopTimeButtonClick(wxCommandEvent& ev)
{
	stopRenderTime();
}

gboolean CamWnd::_onFrame(gpointer data)
{
    CamWnd* self = reinterpret_cast<CamWnd*>(data);

    if (!self->_drawing)
    {
        GlobalRenderSystem().setTime(GlobalRenderSystem().getTime() + MSEC_PER_FRAME);

        // Give the UI a chance to react, but don't hang in there forever
        std::size_t maxEventsPerCallback = 5;

		while (wxTheApp->HasPendingEvents() && --maxEventsPerCallback != 0)
		{
			wxTheApp->Dispatch();
		} 

		self->_wxGLWidget->Refresh();
    }

    // Return true, so that the timer gets called again
    return TRUE;
}

void CamWnd::stopRenderTime()
{
    _timer.disable();

	wxToolBar* miscToolbar = static_cast<wxToolBar*>(_mainWxWidget->FindWindow("MiscToolbar"));

	const wxToolBarToolBase* startTimeButton = getToolBarToolByLabel(miscToolbar, "startTimeButton");
	const wxToolBarToolBase* stopTimeButton = getToolBarToolByLabel(miscToolbar, "stopTimeButton");

	miscToolbar->EnableTool(startTimeButton->GetId(), true);
	miscToolbar->EnableTool(stopTimeButton->GetId(), false);
}

void CamWnd::onRenderModeButtonsChanged(wxCommandEvent& ev)
{
	if (ev.GetInt() == 0) // un-toggled
	{
		return; // Don't react on UnToggle events
	}

	wxToolBar* camToolbar = static_cast<wxToolBar*>(_mainWxWidget->FindWindow("CamToolbar"));

	// This function will be called twice, once for the inactivating button and
    // once for the activating button
	if (getToolBarToolByLabel(camToolbar, "texturedBtn")->GetId() == ev.GetId())
    {
        getCameraSettings()->setRenderMode(RENDER_MODE_TEXTURED);
    }
    else if (getToolBarToolByLabel(camToolbar, "wireframeBtn")->GetId() == ev.GetId())
    {
        getCameraSettings()->setRenderMode(RENDER_MODE_WIREFRAME);
    }
    else if (getToolBarToolByLabel(camToolbar, "flatShadeBtn")->GetId() == ev.GetId())
    {
        getCameraSettings()->setRenderMode(RENDER_MODE_SOLID);
    }
    else if (getToolBarToolByLabel(camToolbar, "lightingBtn")->GetId() == ev.GetId())
    {
        getCameraSettings()->setRenderMode(RENDER_MODE_LIGHTING);
    }
}

void CamWnd::updateActiveRenderModeButton()
{
	wxToolBar* camToolbar = static_cast<wxToolBar*>(_mainWxWidget->FindWindow("CamToolbar"));

    switch (getCameraSettings()->getRenderMode())
    {
    case RENDER_MODE_WIREFRAME:
		camToolbar->ToggleTool(getToolBarToolByLabel(camToolbar, "wireframeBtn")->GetId(), true);
        break;
    case RENDER_MODE_SOLID:
		camToolbar->ToggleTool(getToolBarToolByLabel(camToolbar, "flatShadeBtn")->GetId(), true);
        break;
    case RENDER_MODE_TEXTURED:
		camToolbar->ToggleTool(getToolBarToolByLabel(camToolbar, "texturedBtn")->GetId(), true);
        break;
    case RENDER_MODE_LIGHTING:
		camToolbar->ToggleTool(getToolBarToolByLabel(camToolbar, "lightingBtn")->GetId(), true);
        break;
    default:
        assert(false);
    }
}

int CamWnd::getId()
{
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

void CamWnd::updateSelectionBox(const selection::Rectangle& area)
{
	if (_wxGLWidget->IsShown())
    {
        // Get the rectangle and convert it to screen coordinates
        _dragRectangle = area;
        _dragRectangle.toScreenCoords(_camera.width, _camera.height);

        queueDraw();
    }
}

void CamWnd::changeFloor(const bool up) {
    float current = _camera.getOrigin()[2] - 48;
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

    const Vector3& org = _camera.getOrigin();
    _camera.setOrigin(Vector3(org[0], org[1], current + 48));

    _camera.updateModelview();
    update();
    GlobalCamera().movedNotify();
}

void CamWnd::enableFreeMove()
{
    ASSERT_MESSAGE(!_freeMoveEnabled, "EnableFreeMove: free-move was already enabled");
    _freeMoveEnabled = true;
    _camera.clearMovementFlags(MOVE_ALL);

    removeHandlersMove();

	_wxGLWidget->Connect(wxEVT_LEFT_DOWN, wxMouseEventHandler(CamWnd::onGLMouseButtonPressFreeMove), NULL, this);
	_wxGLWidget->Connect(wxEVT_LEFT_UP, wxMouseEventHandler(CamWnd::onGLMouseButtonReleaseFreeMove), NULL, this);
	_wxGLWidget->Connect(wxEVT_RIGHT_DOWN, wxMouseEventHandler(CamWnd::onGLMouseButtonPressFreeMove), NULL, this);
	_wxGLWidget->Connect(wxEVT_RIGHT_UP, wxMouseEventHandler(CamWnd::onGLMouseButtonReleaseFreeMove), NULL, this);
	_wxGLWidget->Connect(wxEVT_MIDDLE_DOWN, wxMouseEventHandler(CamWnd::onGLMouseButtonPressFreeMove), NULL, this);
	_wxGLWidget->Connect(wxEVT_MIDDLE_UP, wxMouseEventHandler(CamWnd::onGLMouseButtonReleaseFreeMove), NULL, this);

    enableFreeMoveEvents();

	_wxFreezePointer.freeze(*_wxGLWidget->GetParent(), 
		boost::bind(&CamWnd::onGLMouseMoveFreeMoveDelta, this, _1, _2, _3), 
		boost::bind(&CamWnd::onGLFreeMoveCaptureLost, this));
	
    update();
}

void CamWnd::disableFreeMove()
{
    ASSERT_MESSAGE(_freeMoveEnabled, "DisableFreeMove: free-move was not enabled");
    _freeMoveEnabled = false;
    _camera.clearMovementFlags(MOVE_ALL);

    disableFreeMoveEvents();

	_wxGLWidget->Disconnect(wxEVT_LEFT_DOWN, wxMouseEventHandler(CamWnd::onGLMouseButtonPressFreeMove), NULL, this);
	_wxGLWidget->Disconnect(wxEVT_LEFT_UP, wxMouseEventHandler(CamWnd::onGLMouseButtonReleaseFreeMove), NULL, this);
	_wxGLWidget->Disconnect(wxEVT_RIGHT_DOWN, wxMouseEventHandler(CamWnd::onGLMouseButtonPressFreeMove), NULL, this);
	_wxGLWidget->Disconnect(wxEVT_RIGHT_UP, wxMouseEventHandler(CamWnd::onGLMouseButtonReleaseFreeMove), NULL, this);
	_wxGLWidget->Disconnect(wxEVT_MIDDLE_DOWN, wxMouseEventHandler(CamWnd::onGLMouseButtonPressFreeMove), NULL, this);
	_wxGLWidget->Disconnect(wxEVT_MIDDLE_UP, wxMouseEventHandler(CamWnd::onGLMouseButtonReleaseFreeMove), NULL, this);

    addHandlersMove();

	_wxFreezePointer.unfreeze();

    update();
}

bool CamWnd::freeMoveEnabled() const
{
    return _freeMoveEnabled;
}

void CamWnd::Cam_Draw()
{
	wxSize glSize = _wxGLWidget->GetSize();

	if (_camera.width != glSize.GetWidth() || _camera.height != glSize.GetHeight())
	{
		_camera.width = glSize.GetWidth();
		_camera.height = glSize.GetHeight();
		_camera.updateProjection();

		_windowObserver->onSizeChanged(_camera.width, _camera.height);
	}

	if (_camera.height == 0 || _camera.width == 0)
	{
		return; // otherwise we'll receive OpenGL errors in ortho rendering below
	}

    glViewport(0, 0, _camera.width, _camera.height);

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

    glLoadMatrixd(_camera.projection);

    glMatrixMode(GL_MODELVIEW);

    glLoadMatrixd(_camera.modelview);

    // one directional light source directly behind the viewer
    {
        GLfloat inverse_cam_dir[4], ambient[4], diffuse[4];//, material[4];

        ambient[0] = ambient[1] = ambient[2] = 0.4f;
        ambient[3] = 1.0f;
        diffuse[0] = diffuse[1] = diffuse[2] = 0.4f;
        diffuse[3] = 1.0f;
        //material[0] = material[1] = material[2] = 0.8f;
        //material[3] = 1.0f;

        inverse_cam_dir[0] = _camera.vpn[0];
        inverse_cam_dir[1] = _camera.vpn[1];
        inverse_cam_dir[2] = _camera.vpn[2];
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
        CamRenderer renderer(allowedRenderFlags, _selectShader2, _selectShader1, _view.getViewer());

		render::RenderHighlighted::collectRenderablesInScene(renderer, _view);

        renderer.render(_camera.modelview, _camera.projection);
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
    glOrtho(0, (float)_camera.width, 0, (float)_camera.height, -100, 100);

    glScalef(1, -1, 1);
    glTranslatef(0, -(float)_camera.height, 0);

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

    if (_freeMoveEnabled) {
        glBegin( GL_LINES );
        glVertex2f( (float)_camera.width / 2.f, (float)_camera.height / 2.f + 6 );
        glVertex2f( (float)_camera.width / 2.f, (float)_camera.height / 2.f + 2 );
        glVertex2f( (float)_camera.width / 2.f, (float)_camera.height / 2.f - 6 );
        glVertex2f( (float)_camera.width / 2.f, (float)_camera.height / 2.f - 2 );
        glVertex2f( (float)_camera.width / 2.f + 6, (float)_camera.height / 2.f );
        glVertex2f( (float)_camera.width / 2.f + 2, (float)_camera.height / 2.f );
        glVertex2f( (float)_camera.width / 2.f - 6, (float)_camera.height / 2.f );
        glVertex2f( (float)_camera.width / 2.f - 2, (float)_camera.height / 2.f );
        glEnd();
    }

    glRasterPos3f(1.0f, static_cast<float>(_camera.height) - 1.0f, 0.0f);

    GlobalOpenGL().drawString(render::RenderStatistics::Instance().getStatString());

    glRasterPos3f(1.0f, static_cast<float>(_camera.height) - 11.0f, 0.0f);

	GlobalOpenGL().drawString(render::View::getCullStats());

    drawTime();

    // Draw the selection drag rectangle
    if (!_dragRectangle.empty())
    {
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(0, (float)_camera.width, 0, (float)_camera.height, -100, 100);

        glScalef(1, -1, 1);
        glTranslatef(0, -(float)_camera.height, 0);

        // Define the blend function for transparency
        glEnable(GL_BLEND);
        glBlendColor(0, 0, 0, 0.2f);
        glBlendFunc(GL_CONSTANT_ALPHA_EXT, GL_ONE_MINUS_CONSTANT_ALPHA_EXT);

        Vector3 dragBoxColour = ColourSchemes().getColour("drag_selection");
        glColor3dv(dragBoxColour);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

        // Correct the glScale and glTranslate calls above
        selection::Rectangle rect = _dragRectangle;

        double width = rect.max.x() - rect.min.x();
        double height = rect.max.y() - rect.min.y();

        rect.min.y() = _camera.height - rect.min.y();
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

void CamWnd::onRender()
{
	draw();
}

void CamWnd::performDeferredDraw()
{
	_wxGLWidget->Refresh();
}

void CamWnd::draw()
{
    if (_drawing) return;

    util::ScopedBoolLock lock(_drawing);

    if (GlobalMap().isValid() && GlobalMainFrame().screenUpdatesEnabled())
	{
        GlobalOpenGL().assertNoErrors();

        Cam_Draw();

        GlobalOpenGL().assertNoErrors();
    }
}

void CamWnd::benchmark()
{
    double dStart = clock() / 1000.0;

    for (int i=0 ; i < 100 ; i++)
	{
        Vector3 angles;
        angles[CAMERA_ROLL] = 0;
        angles[CAMERA_PITCH] = 0;
        angles[CAMERA_YAW] = static_cast<double>(i * (360.0 / 100.0));
        setCameraAngles(angles);
    }

    double dEnd = clock() / 1000.0;

    rMessage() << (boost::format("%5.2lf") % (dEnd - dStart)) << " seconds\n";
}

void CamWnd::onSceneGraphChange()
{
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
	_wxGLWidget->Connect(wxEVT_LEFT_DOWN, wxMouseEventHandler(CamWnd::onGLMouseButtonPress), NULL, this);
	_wxGLWidget->Connect(wxEVT_LEFT_UP, wxMouseEventHandler(CamWnd::onGLMouseButtonRelease), NULL, this);
	_wxGLWidget->Connect(wxEVT_RIGHT_DOWN, wxMouseEventHandler(CamWnd::onGLMouseButtonPress), NULL, this);
	_wxGLWidget->Connect(wxEVT_RIGHT_UP, wxMouseEventHandler(CamWnd::onGLMouseButtonRelease), NULL, this);
	_wxGLWidget->Connect(wxEVT_MIDDLE_DOWN, wxMouseEventHandler(CamWnd::onGLMouseButtonPress), NULL, this);
	_wxGLWidget->Connect(wxEVT_MIDDLE_UP, wxMouseEventHandler(CamWnd::onGLMouseButtonRelease), NULL, this);

	_wxGLWidget->Connect(wxEVT_MOTION, wxMouseEventHandler(gtkutil::DeferredMotion::wxOnMouseMotion), NULL, &_deferredMouseMotion);

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
	_wxGLWidget->Disconnect(wxEVT_MOTION, wxMouseEventHandler(gtkutil::DeferredMotion::wxOnMouseMotion), NULL, &_deferredMouseMotion);

	_wxGLWidget->Disconnect(wxEVT_LEFT_DOWN, wxMouseEventHandler(CamWnd::onGLMouseButtonPress), NULL, this);
	_wxGLWidget->Disconnect(wxEVT_LEFT_UP, wxMouseEventHandler(CamWnd::onGLMouseButtonRelease), NULL, this);
	_wxGLWidget->Disconnect(wxEVT_RIGHT_DOWN, wxMouseEventHandler(CamWnd::onGLMouseButtonPress), NULL, this);
	_wxGLWidget->Disconnect(wxEVT_RIGHT_UP, wxMouseEventHandler(CamWnd::onGLMouseButtonRelease), NULL, this);
	_wxGLWidget->Disconnect(wxEVT_MIDDLE_DOWN, wxMouseEventHandler(CamWnd::onGLMouseButtonPress), NULL, this);
	_wxGLWidget->Disconnect(wxEVT_MIDDLE_UP, wxMouseEventHandler(CamWnd::onGLMouseButtonRelease), NULL, this);

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

Camera& CamWnd::getCamera()
{
    return _camera;
}

void CamWnd::captureStates()
{
    _selectShader1 = GlobalRenderSystem().capture("$CAM_HIGHLIGHT");
    _selectShader2 = GlobalRenderSystem().capture("$CAM_OVERLAY");
}

void CamWnd::releaseStates()
{
    _selectShader1.reset();
    _selectShader2.reset();
}

void CamWnd::queueDraw()
{
    if (_drawing)
	{
        return;
    }

    _deferredDraw.draw();
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
        _windowObserver->removeObservedWidget(_parentWindow);
        GlobalEventManager().disconnect(_parentWindow->get_toplevel());

        if (_freeMoveEnabled)
        {
            disableFreeMove();
        }

        _parentWindow.reset();
    }

    if (newParent)
    {
        _parentWindow = newParent;

        _windowObserver->addObservedWidget(_parentWindow);
        GlobalEventManager().connect(_parentWindow->get_toplevel());
    }
}

Vector3 CamWnd::getCameraOrigin() const
{
    return _camera.getOrigin();
}

void CamWnd::setCameraOrigin(const Vector3& origin)
{
    _camera.setOrigin(origin);
}

Vector3 CamWnd::getCameraAngles() const
{
    return _camera.getAngles();
}

void CamWnd::setCameraAngles(const Vector3& angles)
{
    _camera.setAngles(angles);
}

void CamWnd::onFarClipPlaneOutClick(wxCommandEvent& ev) 
{
    farClipPlaneOut();
}

void CamWnd::onFarClipPlaneInClick(wxCommandEvent& ev) 
{
    farClipPlaneIn();
}

void CamWnd::farClipPlaneOut() 
{
    getCameraSettings()->setCubicScale( getCameraSettings()->cubicScale() + 1 );

    _camera.updateProjection();
    update();
}

void CamWnd::farClipPlaneIn() 
{
    getCameraSettings()->setCubicScale( getCameraSettings()->cubicScale() - 1 );

    _camera.updateProjection();
    update();
}

void CamWnd::onGLResize(wxSizeEvent& ev)
{
	getCamera().width = ev.GetSize().GetWidth();
	getCamera().height = ev.GetSize().GetHeight();
    getCamera().updateProjection();

    _windowObserver->onSizeChanged(getCamera().width, getCamera().height);

    queueDraw();
}

bool CamWnd::onExpose(GdkEventExpose* ev)
{
    draw();

    return false;
}

void CamWnd::onMouseScroll(wxMouseEvent& ev)
{
    // Determine the direction we are moving.
	if (ev.GetWheelRotation() > 0)
    {
        getCamera().freemoveUpdateAxes();
        setCameraOrigin(getCameraOrigin() + getCamera().forward * static_cast<float>(getCameraSettings()->movementSpeed()));
    }
    else if (ev.GetWheelRotation() < 0)
    {
        getCamera().freemoveUpdateAxes();
        setCameraOrigin(getCameraOrigin() + getCamera().forward * (-static_cast<float>(getCameraSettings()->movementSpeed())));
    }
}

void CamWnd::onGLMouseButtonPress(wxMouseEvent& ev)
{
	// wxTODO GlobalEventManager().MouseEvents().stateMatchesCameraViewEvent(ui::camEnableFreeLookMode, ev)
	if (ev.RightDown())
    {
        enableFreeMove();
        return;
    }

	_windowObserver->onMouseDown(WindowVector(ev.GetX(), ev.GetY()), ev);
}

void CamWnd::onGLMouseButtonRelease(wxMouseEvent& ev)
{
	_windowObserver->onMouseUp(WindowVector(ev.GetX(), ev.GetY()), ev);
}

void CamWnd::onGLMouseMove(int x, int y, unsigned int state)
{
	_windowObserver->onMouseMotion(WindowVector(x, y), state);
}

void CamWnd::onGLMouseButtonPressFreeMove(wxMouseEvent& ev)
{
	if (ev.RightDown() && getCameraSettings()->toggleFreelook())
		// wxTODO GlobalEventManager().MouseEvents().stateMatchesCameraViewEvent(ui::camDisableFreeLookMode, ev)
	{
		// "Toggle free look" option is on, so end the active freelook state on mouse button down
        disableFreeMove();
		return;
	}

	_windowObserver->onMouseDown(windowvector_for_widget_centre(*_wxGLWidget), ev);
}

void CamWnd::onGLMouseButtonReleaseFreeMove(wxMouseEvent& ev)
{
	if (ev.RightUp() && !getCameraSettings()->toggleFreelook())
		// wxTODO GlobalEventManager().MouseEvents().stateMatchesCameraViewEvent(ui::camDisableFreeLookMode, ev)
	{
		// "Toggle free look" option is off, so end the active freelook state on mouse button up
        disableFreeMove();
		return;
	}

	_windowObserver->onMouseUp(windowvector_for_widget_centre(*_wxGLWidget), ev);
}

void CamWnd::onGLMouseMoveFreeMove(wxMouseEvent& ev)
{
	unsigned int state = wxutil::MouseButton::GetStateForMouseEvent(ev);
	_windowObserver->onMouseMotion(windowvector_for_widget_centre(*_wxGLWidget), state);
}

void CamWnd::onGLMouseMoveFreeMoveDelta(int x, int y, unsigned int state)
{
	_camera.m_mouseMove.onMouseMotionDelta(x, y, state);
    _camera.m_strafe = GlobalEventManager().MouseEvents().strafeActive(state);

    if (_camera.m_strafe)
    {
        _camera.m_strafe_forward = GlobalEventManager().MouseEvents().strafeForwardActive(state);
    }
    else
    {
        _camera.m_strafe_forward = false;
    }
}

void CamWnd::onGLFreeMoveCaptureLost()
{
	// Disable free look mode when focus is lost
    disableFreeMove();
}

void CamWnd::drawTime()
{
    if (GlobalRenderSystem().getTime() == 0)
    {
        return;
    }

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, static_cast<float>(_camera.width), 0, static_cast<float>(_camera.height), -100, 100);

    glScalef(1, -1, 1);
    glTranslatef(static_cast<float>(_camera.width) - 90, -static_cast<float>(_camera.height), 0);

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

    glRasterPos3f(1.0f, static_cast<float>(_camera.height) - 1.0f, 0.0f);

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

ShaderPtr CamWnd::_selectShader1;
ShaderPtr CamWnd::_selectShader2;
int CamWnd::_maxId = 0;
