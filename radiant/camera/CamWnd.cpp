#include "CamWnd.h"

#include "igl.h"
#include "ibrush.h"
#include "iclipper.h"
#include "iuimanager.h"
#include "ieventmanager.h"
#include "imainframe.h"
#include "itextstream.h"

#include <time.h>
#include <fmt/format.h>

#include "util/ScopedBoolLock.h"
#include "iselectiontest.h"
#include "selectionlib.h"
#include "gamelib.h"
#include "map/Map.h"
#include "CamRenderer.h"
#include "CameraSettings.h"
#include "GlobalCamera.h"
#include "render/RenderStatistics.h"
#include "render/frontend/RenderableCollectionWalker.h"
#include "wxutil/MouseButton.h"
#include "registry/adaptors.h"
#include "selection/OccludeSelector.h"
#include "selection/Device.h"
#include "selection/SelectionTest.h"

#include "debugging/debugging.h"
#include <wx/sizer.h>
#include "util/ScopedBoolLock.h"
#include <functional>
#include <sigc++/retype_return.h>

namespace ui
{

namespace
{
    const std::size_t MSEC_PER_FRAME = 16;

    const std::string FAR_CLIP_IN_TEXT = "Move far clip plane closer";
    const std::string FAR_CLIP_OUT_TEXT = "Move far clip plane further away";
    const std::string FAR_CLIP_DISABLED_TEXT = " (currently disabled in preferences)";
    const char* const RKEY_SELECT_EPSILON = "user/ui/selectionEpsilon";
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

inline Vector2 windowvector_for_widget_centre(wxutil::GLWidget& widget)
{
    wxSize size = widget.GetSize();
	return Vector2(static_cast<float>(size.GetWidth() / 2), static_cast<float>(size.GetHeight() / 2));
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
    MouseToolHandler(IMouseToolGroup::Type::CameraView),
	_mainWxWidget(loadNamedPanel(parent, "CamWndPanel")),
    _id(++_maxId),
    _view(true),
    _camera(&_view, Callback(std::bind(&CamWnd::queueDraw, this))),
    _cameraView(_camera, &_view, Callback(std::bind(&CamWnd::update, this))),
    _drawing(false),
    _freeMoveEnabled(false),
	_wxGLWidget(new wxutil::GLWidget(_mainWxWidget, std::bind(&CamWnd::onRender, this), "CamWnd")),
    _timer(this),
    _timerLock(false)
{
	Connect(wxEVT_TIMER, wxTimerEventHandler(CamWnd::onFrame), NULL, this);

    constructGUIComponents();

    // Deactivate all commands, just to make sure
    disableDiscreteMoveEvents();
    disableFreeMoveEvents();

    // Connect the mouse button events
    _wxGLWidget->Connect(wxEVT_LEFT_DOWN, wxMouseEventHandler(CamWnd::onGLMouseButtonPress), NULL, this);
    _wxGLWidget->Connect(wxEVT_LEFT_DCLICK, wxMouseEventHandler(CamWnd::onGLMouseButtonPress), NULL, this);
    _wxGLWidget->Connect(wxEVT_LEFT_UP, wxMouseEventHandler(CamWnd::onGLMouseButtonRelease), NULL, this);
    _wxGLWidget->Connect(wxEVT_RIGHT_DOWN, wxMouseEventHandler(CamWnd::onGLMouseButtonPress), NULL, this);
    _wxGLWidget->Connect(wxEVT_RIGHT_DCLICK, wxMouseEventHandler(CamWnd::onGLMouseButtonPress), NULL, this);
    _wxGLWidget->Connect(wxEVT_RIGHT_UP, wxMouseEventHandler(CamWnd::onGLMouseButtonRelease), NULL, this);
    _wxGLWidget->Connect(wxEVT_MIDDLE_DOWN, wxMouseEventHandler(CamWnd::onGLMouseButtonPress), NULL, this);
    _wxGLWidget->Connect(wxEVT_MIDDLE_DCLICK, wxMouseEventHandler(CamWnd::onGLMouseButtonPress), NULL, this);
    _wxGLWidget->Connect(wxEVT_MIDDLE_UP, wxMouseEventHandler(CamWnd::onGLMouseButtonRelease), NULL, this);
	_wxGLWidget->Connect(wxEVT_AUX1_DOWN, wxMouseEventHandler(CamWnd::onGLMouseButtonPress), NULL, this);
	_wxGLWidget->Connect(wxEVT_AUX1_DCLICK, wxMouseEventHandler(CamWnd::onGLMouseButtonPress), NULL, this);
	_wxGLWidget->Connect(wxEVT_AUX1_UP, wxMouseEventHandler(CamWnd::onGLMouseButtonRelease), NULL, this);
	_wxGLWidget->Connect(wxEVT_AUX2_DOWN, wxMouseEventHandler(CamWnd::onGLMouseButtonPress), NULL, this);
	_wxGLWidget->Connect(wxEVT_AUX2_DCLICK, wxMouseEventHandler(CamWnd::onGLMouseButtonPress), NULL, this);
	_wxGLWidget->Connect(wxEVT_AUX2_UP, wxMouseEventHandler(CamWnd::onGLMouseButtonRelease), NULL, this);

    // Now add the handlers for the non-freelook mode, the events are activated by this
    addHandlersMove();

    // Clicks are eaten when the FreezePointer is active, request to receive them
    _freezePointer.connectMouseEvents(
        std::bind(&CamWnd::onGLMouseButtonPress, this, std::placeholders::_1),
        std::bind(&CamWnd::onGLMouseButtonRelease, this, std::placeholders::_1));

    // Subscribe to the global scene graph update
    GlobalSceneGraph().addSceneObserver(this);

	_glExtensionsInitialisedNotifier = GlobalRenderSystem().signal_extensionsInitialised().connect(
		sigc::mem_fun(this, &CamWnd::onGLExtensionsInitialised));
}

wxWindow* CamWnd::getMainWidget() const
{ 
	return _mainWxWidget;
}

void CamWnd::constructToolbar()
{
	// If lighting is not available, grey out the lighting button
	wxToolBar* camToolbar = findNamedObject<wxToolBar>(_mainWxWidget, "CamToolbar");

	const wxToolBarToolBase* wireframeBtn = getToolBarToolByLabel(camToolbar, "wireframeBtn");
	const wxToolBarToolBase* flatShadeBtn = getToolBarToolByLabel(camToolbar, "flatShadeBtn");
	const wxToolBarToolBase* texturedBtn = getToolBarToolByLabel(camToolbar, "texturedBtn");
	const wxToolBarToolBase* lightingBtn = getToolBarToolByLabel(camToolbar, "lightingBtn");

    if (!GlobalOpenGL().shaderProgramsAvailable())
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

void CamWnd::onGLExtensionsInitialised()
{
	// If lighting is not available, grey out the lighting button
	wxToolBar* camToolbar = findNamedObject<wxToolBar>(_mainWxWidget, "CamToolbar");
	const wxToolBarToolBase* lightingBtn = getToolBarToolByLabel(camToolbar, "lightingBtn");

    camToolbar->EnableTool(lightingBtn->GetId(), GlobalOpenGL().shaderProgramsAvailable());
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
	_wxGLWidget->SetCanFocus(false);
	_wxGLWidget->SetMinClientSize(wxSize(CAMWND_MINSIZE_X, CAMWND_MINSIZE_Y));
	_wxGLWidget->Connect(wxEVT_SIZE, wxSizeEventHandler(CamWnd::onGLResize), NULL, this);
	_wxGLWidget->Connect(wxEVT_MOUSEWHEEL, wxMouseEventHandler(CamWnd::onMouseScroll), NULL, this);

	_mainWxWidget->GetSizer()->Add(_wxGLWidget, 1, wxEXPAND); 
}

CamWnd::~CamWnd()
{
    // Stop the timer, it might still fire even during shutdown
    _timer.Stop();

    // Unsubscribe from the global scene graph update
    GlobalSceneGraph().removeSceneObserver(this);

    if (_freeMoveEnabled) {
        disableFreeMove();
    }

    removeHandlersMove();

    // Notify the camera manager about our destruction
    GlobalCamera().removeCamWnd(_id);
}

SelectionTestPtr CamWnd::createSelectionTestForPoint(const Vector2& point)
{
    float selectEpsilon = registry::getValue<float>(RKEY_SELECT_EPSILON);

    // Get the mouse position
    Vector2 deviceEpsilon(selectEpsilon / getCamera().width, selectEpsilon / getCamera().height);

    // Copy the current view and constrain it to a small rectangle
    render::View scissored(_view);
    ConstructSelectionTest(scissored, selection::Rectangle::ConstructFromPoint(point, deviceEpsilon));

    return SelectionTestPtr(new SelectionVolume(scissored));
}

const VolumeTest& CamWnd::getVolumeTest() const
{
    return _view;
}

int CamWnd::getDeviceWidth() const
{
    return _camera.width;
}

int CamWnd::getDeviceHeight() const
{
    return _camera.height;
}

void CamWnd::startRenderTime()
{
	if (_timer.IsRunning())
    {
        // Timer is already running, just reset the preview time
        GlobalRenderSystem().setTime(0);
    }
    else
    {
        // Timer is not enabled, we're paused or stopped
		_timer.Start(MSEC_PER_FRAME);
        _timerLock = false; // reset the lock, just in case
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

void CamWnd::onFrame(wxTimerEvent& ev)
{
    // Calling wxTheApp->Yield() might cause another timer callback if enough 
    // time has passed during rendering. Calling Yield() within Yield() 
    // might in the end cause stack overflows and is caught by wxWidgets.
    if (!_timerLock)
    {
        util::ScopedBoolLock lock(_timerLock);

        GlobalRenderSystem().setTime(GlobalRenderSystem().getTime() + _timer.GetInterval());

        // Mouse movement is handled via idle callbacks, so let's give the app a chance to react
        wxTheApp->ProcessIdle();

		_wxGLWidget->Refresh();
    }
}

void CamWnd::stopRenderTime()
{
    _timer.Stop();

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

    enableFreeMoveEvents();

    update();
}

void CamWnd::disableFreeMove()
{
    ASSERT_MESSAGE(_freeMoveEnabled, "DisableFreeMove: free-move was not enabled");
    _freeMoveEnabled = false;
    _camera.clearMovementFlags(MOVE_ALL);

    disableFreeMoveEvents();

    addHandlersMove();

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
        CamRenderer renderer(allowedRenderFlags, _primitiveHighlightShader,
                             _faceHighlightShader, _view.getViewer());

		render::RenderableCollectionWalker::CollectRenderablesInScene(renderer, _view);

		// Render any active mousetools
		for (const ActiveMouseTools::value_type& i : _activeMouseTools)
		{
			i.second->render(GlobalRenderSystem(), renderer, _view);
		}

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

    if (!_activeMouseTools.empty())
    {
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(0, (float)_camera.width, 0, (float)_camera.height, -100, 100);

        for (const ActiveMouseTools::value_type& i : _activeMouseTools)
        {
            i.second->renderOverlay();
        }
    }

    // bind back to the default texture so that we don't have problems
    // elsewhere using/modifying texture maps between contexts
    glBindTexture( GL_TEXTURE_2D, 0 );
}

void CamWnd::onRender()
{
	draw();
}

void CamWnd::draw()
{
    if (_drawing) return;

    util::ScopedBoolLock lock(_drawing);

    if (GlobalMainFrame().screenUpdatesEnabled())
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

    rMessage() << fmt::format("{0:5.2lf}", (dEnd - dStart)) << " seconds\n";
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
	_wxGLWidget->Connect(wxEVT_MOTION, wxMouseEventHandler(CamWnd::onGLMouseMove), NULL, this);

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
    _wxGLWidget->Disconnect(wxEVT_MOTION, wxMouseEventHandler(CamWnd::onGLMouseMove), NULL, this);

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
    _faceHighlightShader = GlobalRenderSystem().capture("$CAM_HIGHLIGHT");
    _primitiveHighlightShader = GlobalRenderSystem().capture("$CAM_OVERLAY");
}

void CamWnd::releaseStates() {
    _faceHighlightShader = ShaderPtr();
    _primitiveHighlightShader = ShaderPtr();
}

void CamWnd::queueDraw()
{
    if (_drawing)
	{
        return;
    }

	_wxGLWidget->Refresh(false);
}

Vector3 CamWnd::getCameraOrigin() const
{
    return _camera.getOrigin();
}

void CamWnd::setCameraOrigin(const Vector3& origin)
{
    _camera.setOrigin(origin);
}

Vector3 CamWnd::getRightVector() const
{
	return _camera.vright;
}

Vector3 CamWnd::getUpVector() const
{
	return _camera.vup;
}

Vector3 CamWnd::getForwardVector() const
{
	return _camera.vpn;
}

Vector3 CamWnd::getCameraAngles() const
{
    return _camera.getAngles();
}

void CamWnd::setCameraAngles(const Vector3& angles)
{
    _camera.setAngles(angles);
}

const Frustum& CamWnd::getViewFrustum() const
{
    return _view.getFrustum();
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

    queueDraw();

	ev.Skip();
}

void CamWnd::onMouseScroll(wxMouseEvent& ev)
{
	float movementSpeed = static_cast<float>(getCameraSettings()->movementSpeed());

	if (ev.ShiftDown())
	{
		movementSpeed *= 2;
	}
	else if (ev.AltDown())
	{
		movementSpeed *= 0.1f;
	}

    // Determine the direction we are moving.
	if (ev.GetWheelRotation() > 0)
    {
        getCamera().freemoveUpdateAxes();
		setCameraOrigin(getCameraOrigin() + getCamera().forward * movementSpeed);
    }
    else if (ev.GetWheelRotation() < 0)
    {
        getCamera().freemoveUpdateAxes();
		setCameraOrigin(getCameraOrigin() + getCamera().forward * -movementSpeed);
    }
}

ui::CameraMouseToolEvent CamWnd::createMouseEvent(const Vector2& point, const Vector2& delta)
{
    // When freeMove is enabled, snap the mouse coordinates to the center of the view widget
    Vector2 actualPoint = freeMoveEnabled() ? windowvector_for_widget_centre(*_wxGLWidget) : point;

    Vector2 normalisedDeviceCoords = device_constrained(
        window_to_normalised_device(actualPoint, _camera.width, _camera.height));

    return ui::CameraMouseToolEvent(*this, normalisedDeviceCoords, delta);
}

MouseTool::Result CamWnd::processMouseDownEvent(const MouseToolPtr& tool, const Vector2& point)
{
    CameraMouseToolEvent ev = createMouseEvent(point);
    return tool->onMouseDown(ev);
}

MouseTool::Result CamWnd::processMouseUpEvent(const MouseToolPtr& tool, const Vector2& point)
{
    CameraMouseToolEvent ev = createMouseEvent(point);
    return tool->onMouseUp(ev);
}

MouseTool::Result CamWnd::processMouseMoveEvent(const MouseToolPtr& tool, int x, int y)
{
    bool mouseToolReceivesDeltas = (tool->getPointerMode() & MouseTool::PointerMode::MotionDeltas) != 0;

    // New MouseTool event, optionally passing the delta only
    CameraMouseToolEvent ev = mouseToolReceivesDeltas ?
        createMouseEvent(Vector2(0, 0), Vector2(x, y)) :
        createMouseEvent(Vector2(x, y));

    return tool->onMouseMove(ev);
}

void CamWnd::startCapture(const ui::MouseToolPtr& tool)
{
    if (_freezePointer.isCapturing(_wxGLWidget))
    {
        return;
    }

    unsigned int pointerMode = tool->getPointerMode();

    _freezePointer.startCapture(_wxGLWidget,
        [&](int x, int y, int mouseState) // Motion Functor
        { 
            MouseToolHandler::onGLCapturedMouseMove(x, y, mouseState); 

            if (freeMoveEnabled())
            {
                handleGLMouseMoveFreeMoveDelta(x, y, mouseState);
            }
        }, 
        [&, tool]() { MouseToolHandler::handleCaptureLost(tool); }, // called when the capture is lost.
        (pointerMode & MouseTool::PointerMode::Freeze) != 0,
        (pointerMode & MouseTool::PointerMode::Hidden) != 0,
        (pointerMode & MouseTool::PointerMode::MotionDeltas) != 0
    );
}

void CamWnd::endCapture()
{
    if (!_freezePointer.isCapturing(_wxGLWidget))
    {
        return;
    }

    _freezePointer.endCapture();
}

IInteractiveView& CamWnd::getInteractiveView()
{
    return *this;
}

void CamWnd::forceRedraw()
{
    if (_drawing)
    {
        return;
    }

	_wxGLWidget->Refresh(false);
    _wxGLWidget->Update();
}

void CamWnd::onGLMouseButtonPress(wxMouseEvent& ev)
{
	// The focus might be on some editable child window - since the
	// GL widget cannot be focused itself, let's reset the focus on the toplevel window
	// which will propagate any key events accordingly.
	GlobalMainFrame().getWxTopLevelWindow()->SetFocus();

    // Pass the call to the actual handler
    MouseToolHandler::onGLMouseButtonPress(ev);
}

void CamWnd::onGLMouseButtonRelease(wxMouseEvent& ev)
{
    MouseToolHandler::onGLMouseButtonRelease(ev);
}

void CamWnd::onGLMouseMove(wxMouseEvent& ev)
{
    MouseToolHandler::onGLMouseMove(ev);
}

void CamWnd::handleGLMouseMoveFreeMoveDelta(int x, int y, unsigned int state)
{
	_camera.m_mouseMove.onMouseMotionDelta(x, y, state);

    unsigned int strafeFlags = GlobalCamera().getStrafeModifierFlags();

    _camera.m_strafe = (state & strafeFlags) == strafeFlags;

    if (_camera.m_strafe)
    {
        unsigned int strafeForwardFlags = GlobalCamera().getStrafeForwardModifierFlags();
        _camera.m_strafe_forward = (state & strafeForwardFlags) == strafeForwardFlags;
    }
    else
    {
        _camera.m_strafe_forward = false;
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
    GlobalOpenGL().drawString(fmt::format("Time: {0:.3f} sec.", (time * 0.001f)));
}

// -------------------------------------------------------------------------------

ShaderPtr CamWnd::_faceHighlightShader;
ShaderPtr CamWnd::_primitiveHighlightShader;
int CamWnd::_maxId = 0;

} // namespace
