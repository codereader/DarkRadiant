#include "XYWnd.h"

#include "i18n.h"
#include "iscenegraph.h"
#include "iundo.h"
#include "ieventmanager.h"
#include "imainframe.h"
#include "ientity.h"
#include "igrid.h"
#include "iuimanager.h"

#include "wxutil/MouseButton.h"
#include "wxutil/GLWidget.h"
#include "string/string.h"

#include "brush/TexDef.h"
#include "ibrush.h"
#include "camera/GlobalCamera.h"
#include "camera/CameraSettings.h"
#include "ui/ortho/OrthoContextMenu.h"
#include "ui/overlay/Overlay.h"
#include "ui/texturebrowser/TextureBrowser.h"
#include "map/RegionManager.h"
#include "selection/algorithm/General.h"
#include "selection/algorithm/Primitives.h"
#include "registry/registry.h"

#include "GlobalXYWnd.h"
#include "XYRenderer.h"
#include "gamelib.h"
#include "scenelib.h"
#include "render/frontend/RenderableCollectionWalker.h"

#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>
#include <boost/bind.hpp>

inline float Betwixt(float f1, float f2) {
    return (f1 + f2) * 0.5f;
}

double two_to_the_power(int power) {
    return pow(2.0f, power);
}

inline float screen_normalised(int pos, unsigned int size) {
    return ((2.0f * pos) / size) - 1.0f;
}

inline float normalised_to_world(float normalised, float world_origin, float normalised2world_scale) {
    return world_origin + normalised * normalised2world_scale;
}

namespace
{
    const char* const RKEY_XYVIEW_ROOT = "user/ui/xyview";
}

// Constructor
XYWnd::XYWnd(int id, wxWindow* parent) :
	_id(id),
	_wxGLWidget(new wxutil::GLWidget(parent, boost::bind(&XYWnd::onRender, this), "XYWnd")),
	m_deferredDraw(boost::bind(&XYWnd::performDeferredDraw, this)),
	_deferredMouseMotion(boost::bind(&XYWnd::onGLMouseMove, this, _1, _2, _3)),
	_minWorldCoord(game::current::getValue<float>("/defaults/minWorldCoord")),
	_maxWorldCoord(game::current::getValue<float>("/defaults/maxWorldCoord")),
	_defaultCursor(wxCURSOR_DEFAULT),
	_crossHairCursor(wxCURSOR_CROSS),
	_moveStarted(false),
	_zoomStarted(false),
	_chasingMouse(false),
	_wxMouseButtonState(0),
	m_window_observer(NewWindowObserver()),
	_isActive(false)
{
    m_bNewBrushDrag = false;

    _width = 0;
    _height = 0;

    // Try to retrieve a recently used origin and scale from the registry
    std::string recentPath = std::string(RKEY_XYVIEW_ROOT) + "/recent";
    m_vOrigin = string::convert<Vector3>(
        GlobalRegistry().getAttribute(recentPath, "origin")
    );
    m_fScale = string::convert<double>(
        GlobalRegistry().getAttribute(recentPath, "scale")
    );

    if (m_fScale == 0)
    {
        m_fScale = 1;
    }

    m_viewType = XY;

    m_entityCreate = false;

    m_window_observer->setRectangleDrawCallback(boost::bind(&XYWnd::updateSelectionBox, this, _1));
    m_window_observer->setView(m_view);

	_wxGLWidget->SetCanFocus(false);
	// Don't set a minimum size, to allow for cam window maximisation
	//_wxGLWidget->SetMinClientSize(wxSize(XYWND_MINSIZE_X, XYWND_MINSIZE_Y));

	// wxGLWidget wireup
	_wxGLWidget->Connect(wxEVT_SIZE, wxSizeEventHandler(XYWnd::onGLResize), NULL, this);

	_wxGLWidget->Connect(wxEVT_MOUSEWHEEL, wxMouseEventHandler(XYWnd::onGLWindowScroll), NULL, this);
	_wxGLWidget->Connect(wxEVT_MOTION, wxMouseEventHandler(wxutil::DeferredMotion::wxOnMouseMotion), NULL, &_deferredMouseMotion);
	
	_wxGLWidget->Connect(wxEVT_LEFT_DOWN, wxMouseEventHandler(XYWnd::onGLMouseButtonPress), NULL, this);
	_wxGLWidget->Connect(wxEVT_LEFT_UP, wxMouseEventHandler(XYWnd::onGLMouseButtonRelease), NULL, this);
	_wxGLWidget->Connect(wxEVT_RIGHT_DOWN, wxMouseEventHandler(XYWnd::onGLMouseButtonPress), NULL, this);
	_wxGLWidget->Connect(wxEVT_RIGHT_UP, wxMouseEventHandler(XYWnd::onGLMouseButtonRelease), NULL, this);
	_wxGLWidget->Connect(wxEVT_MIDDLE_DOWN, wxMouseEventHandler(XYWnd::onGLMouseButtonPress), NULL, this);
	_wxGLWidget->Connect(wxEVT_MIDDLE_UP, wxMouseEventHandler(XYWnd::onGLMouseButtonRelease), NULL, this);

	_wxGLWidget->Connect(wxEVT_IDLE, wxIdleEventHandler(XYWnd::onIdle), NULL, this);

	_freezePointer.setCallEndMoveOnMouseUp(true);
	_freezePointer.connectMouseEvents(
		wxutil::FreezePointer::MouseEventFunction(),
		boost::bind(&XYWnd::onGLMouseButtonRelease, this, _1),
		wxutil::FreezePointer::MouseEventFunction());

    GlobalMap().signal_mapValidityChanged().connect(
        sigc::mem_fun(m_deferredDraw, &DeferredDraw::onMapValidChanged)
    );

    updateProjection();
    updateModelview();

    // Add self to the scenechange callbacks
    GlobalSceneGraph().addSceneObserver(this);

    // greebo: Connect <self> as CameraObserver to the CamWindow. This way this class gets notified on camera change
    GlobalCamera().addCameraObserver(this);

	// Let the window observer connect its handlers to the GL widget first (before the event manager)
	m_window_observer->addObservedWidget(*_wxGLWidget);

	GlobalEventManager().connect(*_wxGLWidget);
}

// Destructor
XYWnd::~XYWnd()
{
    destroyXYView();

    // Store the current position and scale to the registry, so that it may be
    // picked up again when creating XYViews after switching layouts
    std::string recentPath = std::string(RKEY_XYVIEW_ROOT) + "/recent";
    GlobalRegistry().setAttribute(recentPath, "origin",
                                  string::to_string(m_vOrigin));
    GlobalRegistry().setAttribute(recentPath, "scale",
                                  string::to_string(m_fScale));
}

int XYWnd::getId() const
{
    return _id;
}

void XYWnd::destroyXYView()
{
    // Remove <self> from the scene change callback list
    GlobalSceneGraph().removeSceneObserver(this);

    // greebo: Remove <self> as CameraObserver from the CamWindow.
    GlobalCamera().removeCameraObserver(this);

	if (_wxGLWidget != NULL)
	{
		if (m_window_observer != NULL)
		{
			m_window_observer->removeObservedWidget(*_wxGLWidget);
		}

		GlobalEventManager().disconnect(*_wxGLWidget);
	}

    // This deletes the RadiantWindowObserver from the heap
    if (m_window_observer != NULL)
    {
        m_window_observer->release();
        m_window_observer = NULL;
    }
}

void XYWnd::setScale(float f) {
    m_fScale = f;
    updateProjection();
    updateModelview();
    queueDraw();
}

float XYWnd::getScale() const {
    return m_fScale;
}

int XYWnd::getWidth() const {
    return _width;
}

int XYWnd::getHeight() const {
    return _height;
}

void XYWnd::captureStates() {
    _selectedShader = GlobalRenderSystem().capture("$XY_OVERLAY");
}

void XYWnd::releaseStates() {
    _selectedShader = ShaderPtr();
}

const std::string XYWnd::getViewTypeTitle(EViewType viewtype) {
    if (viewtype == XY) {
        return _("XY Top");
    }
    if (viewtype == XZ) {
        return _("XZ Front");
    }
    if (viewtype == YZ) {
        return _("YZ Side");
    }
    return "";
}

const std::string XYWnd::getViewTypeStr(EViewType viewtype) {
    if (viewtype == XY) {
        return "XY";
    }
    if (viewtype == XZ) {
        return "XZ";
    }
    if (viewtype == YZ) {
        return "YZ";
    }
    return "";
}

void XYWnd::queueDraw()
{
    m_deferredDraw.draw();
}

void XYWnd::onSceneGraphChange() {
    // Pass the call to queueDraw.
    queueDraw();
}

void XYWnd::setActive(bool b) {
    _isActive = b;
};

bool XYWnd::isActive() const {
    return _isActive;
};

const Vector3& XYWnd::getOrigin() {
    return m_vOrigin;
}

void XYWnd::setOrigin(const Vector3& origin) {
    m_vOrigin = origin;
    updateModelview();
}

void XYWnd::scroll(int x, int y)
{
    int nDim1 = (m_viewType == YZ) ? 1 : 0;
    int nDim2 = (m_viewType == XY) ? 1 : 2;
    m_vOrigin[nDim1] += x / m_fScale;
    m_vOrigin[nDim2] += y / m_fScale;
    updateModelview();
    queueDraw();
}

/* greebo: This gets repeatedly called during a mouse chase operation.
 * The method is making use of a timer to determine the amount of time that has
 * passed since the chaseMouse has been started
 */
void XYWnd::chaseMouse()
{
	float multiplier = _chaseMouseTimer.Time() / 10.0f;
	scroll(float_to_integer(multiplier * _chasemouseDeltaX), float_to_integer(multiplier * -_chasemouseDeltaY));

	//rMessage() << "chasemouse: multiplier=" << multiplier << " x=" << _chasemouseDeltaX << " y=" << _chasemouseDeltaY << std::endl;

	handleGLMouseMove(_chasemouseCurrentX, _chasemouseCurrentY, _eventState);

    // greebo: Restart the timer
    _chaseMouseTimer.Start();
}

/* greebo: This handles the "chase mouse" behaviour, if the user drags something
 * beyond the XY window boundaries. If the chaseMouse option (currently a global)
 * is set true, the view origin gets relocated along with the mouse movements.
 *
 * @returns: true, if the mousechase has been performed, false if no mouse chase was necessary
 */
bool XYWnd::chaseMouseMotion(int pointx, int pointy, unsigned int state)
{
	_chasemouseDeltaX = 0;
	_chasemouseDeltaY = 0;

    IMouseEvents& mouseEvents = GlobalEventManager().MouseEvents();

	// These are the events that are allowed
	bool isAllowedEvent = mouseEvents.stateMatchesXYViewEvent(ui::xySelect, state) || 
		mouseEvents.stateMatchesXYViewEvent(ui::xyNewBrushDrag, state);

	// greebo: The mouse chase is only active when the according global is set to true and if we
	// are in the right state
	if (GlobalXYWnd().chaseMouse() && isAllowedEvent)
	{
		const int epsilon = 16;

		// Calculate the X delta
		if (pointx < epsilon)
		{
			_chasemouseDeltaX = std::max(pointx, 0) - epsilon;
		}
		else if ((pointx - _width) > -epsilon)
		{
			_chasemouseDeltaX = std::min((pointx - _width), 0) + epsilon;
		}

		// Calculate the Y delta
		if (pointy < epsilon)
		{
			_chasemouseDeltaY = std::max(pointy, 0) - epsilon;
		}
		else if ((pointy - _height) > -epsilon) 
		{
			_chasemouseDeltaY = std::min((pointy - _height), 0) + epsilon;
		}

		// If any of the deltas is uneqal to zero the mouse chase is to be performed
		if (_chasemouseDeltaY != 0 || _chasemouseDeltaX != 0)
		{
			//rMessage() << "chasemouse motion: x=" << pointx << " y=" << pointy << "... " << std::endl;

			_chasemouseCurrentX = pointx;
			_chasemouseCurrentY = pointy;

			// Start the timer, if there isn't one already connected
			if (!_chasingMouse)
			{
				//rMessage() << "chasemouse timer start... " << std::endl;
				_chaseMouseTimer.Start();

				// Enable chase mouse handling in  the idle callbacks, so it gets called as
				// soon as there is nothing more important to do. The callback queries the timer
				// and takes the according window movement actions
				_chasingMouse = true;
			}

			// Return true to signal that there are no other mouseMotion handlers to be performed
			return true;
		}
		else
		{
			if (_chasingMouse)
			{
				// All deltas are zero, so there is no more mouse chasing necessary, remove the handlers
				_chasingMouse = false;
				//rMessage() << "chasemouse cancel" << std::endl;
			}
		}
	}
	else
	{
		if (_chasingMouse)
		{
			// Remove the handlers, the user has probably released the mouse button during chase
			_chasingMouse = false;
			//rMessage() << "chasemouse cancel" << std::endl;
		}
	}

    // No mouse chasing has been performed, return false
    return false;
}

void XYWnd::DropClipPoint(int pointx, int pointy)
{
    Vector3 point = convertXYToWorld(pointx, pointy);

    Vector3 mid = selection::algorithm::getCurrentSelectionCenter();

    GlobalClipper().setViewType(static_cast<EViewType>(getViewType()));
    int nDim = (GlobalClipper().getViewType() == YZ ) ?  0 : ( (GlobalClipper().getViewType() == XZ) ? 1 : 2 );
    point[nDim] = mid[nDim];
    point.snap(GlobalGrid().getGridSize());
    GlobalClipper().newClipPoint(point);
}

void XYWnd::Clipper_OnLButtonDown(int x, int y)
{
    Vector3 mousePosition = convertXYToWorld(x, y);

    ClipPoint* foundClipPoint = GlobalClipper().find(mousePosition, (EViewType)m_viewType, m_fScale);

    GlobalClipper().setMovingClip(foundClipPoint);

    if (foundClipPoint == NULL) {
        DropClipPoint(x, y);
    }
}

void XYWnd::Clipper_OnLButtonUp(int x, int y)
{
    GlobalClipper().setMovingClip(NULL);
}

void XYWnd::Clipper_OnMouseMoved(int x, int y)
{
    ClipPoint* movingClip = GlobalClipper().getMovingClip();

    if (movingClip != NULL)
    {
        GlobalClipper().getMovingClipCoords() = convertXYToWorld(x, y);
        snapToGrid(GlobalClipper().getMovingClipCoords());
        GlobalClipper().update();
        GlobalMainFrame().updateAllWindows();
    }
}

void XYWnd::Clipper_Crosshair_OnMouseMoved(int x, int y)
{
#if 0
    // mtTODO
    Vector3 mousePosition = convertXYToWorld(x, y);

	if (GlobalClipper().clipMode() && GlobalClipper().find(mousePosition, m_viewType, m_fScale) != 0)
	{
        setCursorType(CursorType::Crosshair);
	}
	else
	{
        setCursorType(CursorType::Default);
	}
#endif
}

void XYWnd::setCursorType(CursorType type)
{
    switch (type)
    {
    case CursorType::Pointer:
        _wxGLWidget->SetCursor(_defaultCursor);
        break;
    case CursorType::Crosshair:
        _wxGLWidget->SetCursor(_crossHairCursor);
        break;
    };
}

void XYWnd::positionCamera(int x, int y, CamWnd& camwnd)
{
    Vector3 origin = convertXYToWorld(x, y);

    switch (m_viewType)
    {
    case XY:
        origin[2] = camwnd.getCameraOrigin()[2];
        break;
    case YZ:
        origin[0] = camwnd.getCameraOrigin()[0];
        break;
    case XZ:
        origin[1] = camwnd.getCameraOrigin()[1];
        break;
    };

    snapToGrid(origin);
    camwnd.setCameraOrigin(origin);
}

void XYWnd::orientCamera(int x, int y, CamWnd& camwnd)
{
    Vector3 point = convertXYToWorld(x, y);
    snapToGrid(point);
    point -= camwnd.getCameraOrigin();

    int n1 = (getViewType() == XY) ? 1 : 2;
    int n2 = (getViewType() == YZ) ? 1 : 0;
    int nAngle = (getViewType() == XY) ? CAMERA_YAW : CAMERA_PITCH;
    if (point[n1] || point[n2]) {
        Vector3 angles(camwnd.getCameraAngles());
        angles[nAngle] = static_cast<float>(radians_to_degrees(atan2 (point[n1], point[n2])));
        camwnd.setCameraAngles(angles);
    }
}

// Callback that gets invoked on camera move
void XYWnd::cameraMoved() {
    if (GlobalXYWnd().camXYUpdate()) {
        queueDraw();
    }
}

/*
==============
NewBrushDrag
==============
*/
void XYWnd::NewBrushDrag_Begin(int x, int y) {
    m_NewBrushDrag = scene::INodePtr();
    m_nNewBrushPressx = x;
    m_nNewBrushPressy = y;

    m_bNewBrushDrag = true;
    GlobalUndoSystem().start();
}

void XYWnd::NewBrushDrag_End(int x, int y) {
    if (m_NewBrushDrag != 0) {
        GlobalUndoSystem().finish("brushDragNew");
    }
}

void XYWnd::NewBrushDrag(int x, int y)
{
    Vector3 mins = convertXYToWorld(m_nNewBrushPressx, m_nNewBrushPressy);
    snapToGrid(mins);

    Vector3 maxs = convertXYToWorld(x, y);
    snapToGrid(maxs);

    int nDim = (m_viewType == XY) ? 2 : (m_viewType == YZ) ? 0 : 1;

    const selection::WorkZone& wz = GlobalSelectionSystem().getWorkZone();

    mins[nDim] = float_snapped(wz.min[nDim], GlobalGrid().getGridSize());
    maxs[nDim] = float_snapped(wz.max[nDim], GlobalGrid().getGridSize());

    if (maxs[nDim] <= mins[nDim])
        maxs[nDim] = mins[nDim] + GlobalGrid().getGridSize();

    for (int i=0 ; i<3 ; i++) {
        if (mins[i] == maxs[i])
            return; // don't create a degenerate brush
        if (mins[i] > maxs[i]) {
            float   temp = mins[i];
            mins[i] = maxs[i];
            maxs[i] = temp;
        }
    }

    if (m_NewBrushDrag == NULL) {
        // greebo: Create a new brush
        scene::INodePtr brushNode(GlobalBrushCreator().createBrush());

        if (brushNode != NULL) {
            // Brush could be created

            // Insert the brush into worldspawn
            scene::INodePtr worldspawn = GlobalMap().findOrInsertWorldspawn();
            scene::addNodeToContainer(brushNode, worldspawn);

            // Make sure the brush is selected
            Node_setSelected(brushNode, true);

            // Remember the node
            m_NewBrushDrag = brushNode;
        }
    }

    selection::algorithm::resizeBrushesToBounds(
        AABB::createFromMinMax(mins, maxs), 
        GlobalTextureBrowser().getSelectedShader()
    );
}

void XYWnd::onContextMenu()
{
	// Get the click point in 3D space
	Vector3 point;
	mouseToPoint(m_entityCreate_x, m_entityCreate_y, point);

	// Display the menu, passing the coordinates for creation
	ui::OrthoContextMenu::Instance().Show(_wxGLWidget, point);
}

void XYWnd::beginMove()
{
	if (_moveStarted) {
		endMove();
	}
	_moveStarted = true;

	_freezePointer.freeze(*_wxGLWidget->GetParent(), 
		boost::bind(&XYWnd::onGLMouseMoveDelta, this, _1, _2, _3), 
		boost::bind(&XYWnd::onGLMouseCaptureLost, this));
}

void XYWnd::endMove()
{
	_moveStarted = false;
	_freezePointer.unfreeze();
}

void XYWnd::beginZoom()
{
	if (_zoomStarted)
	{
		endZoom();
	}

	_zoomStarted = true;
	_dragZoom = 0;

	_freezePointer.freeze(*_wxGLWidget->GetParent(), 
		boost::bind(&XYWnd::onGLZoomDelta, this, _1, _2, _3), 
		boost::bind(&XYWnd::onGLZoomMouseCaptureLost, this));
}

void XYWnd::endZoom() 
{
	_zoomStarted = false;
	_freezePointer.unfreeze();
}

// makes sure the selected brush or camera is in view
void XYWnd::positionView(const Vector3& position) {
    int nDim1 = (m_viewType == YZ) ? 1 : 0;
    int nDim2 = (m_viewType == XY) ? 1 : 2;

    m_vOrigin[nDim1] = position[nDim1];
    m_vOrigin[nDim2] = position[nDim2];

    updateModelview();

    queueDraw();
}

void XYWnd::setViewType(EViewType viewType) {
    m_viewType = viewType;
    updateModelview();
}

EViewType XYWnd::getViewType() const {
    return m_viewType;
}

void XYWnd::handleGLMouseDown(wxMouseEvent& ev)
{
    IMouseEvents& mouseEvents = GlobalEventManager().MouseEvents();

	if (mouseEvents.stateMatchesXYViewEvent(ui::xyMoveView, ev))
	{
		beginMove();
    	EntityCreate_MouseDown(ev.GetX(), ev.GetY());
	}

	if (mouseEvents.stateMatchesXYViewEvent(ui::xyZoom, ev))
	{
		beginZoom();
	}

	if (mouseEvents.stateMatchesXYViewEvent(ui::xyCameraMove, ev))
	{
		CamWndPtr cam = GlobalCamera().getActiveCamWnd();

		if (cam != NULL)
		{
			positionCamera(ev.GetX(), ev.GetY(), *cam);
		}
	}

	if (mouseEvents.stateMatchesXYViewEvent(ui::xyCameraAngle, ev))
	{
		CamWndPtr cam = GlobalCamera().getActiveCamWnd();

		if (cam)
		{
			orientCamera(ev.GetX(), ev.GetY(), *cam);
		}
	}

    ui::MouseToolStack tools = GlobalXYWnd().getMouseToolStackForEvent(ev);

    // Construct the mousedown event and see which tool is able to handle it
    ui::XYMouseToolEvent mouseEvent(*this, convertXYToWorld(ev.GetX(), ev.GetY()));

    _activeMouseTool = tools.handleMouseDownEvent(mouseEvent);

    if (_activeMouseTool)
    {
        // Check if the mousetool requires pointer freeze
        if (_activeMouseTool->getPointerMode() == ui::MouseTool::PointerMode::Capture)
        {
            _freezePointer.freeze(*_wxGLWidget, 
                [&] (int dx, int dy, int mouseState)   // Motion Functor
                {
                    // New MouseTool event, passing the delta
                    ui::XYMouseToolEvent ev(*this, convertXYToWorld(ev.GetX(), ev.GetY()), Vector2(dx, dy));

                    if (_activeMouseTool->onMouseMove(ev) == ui::MouseTool::Result::Finished)
                    {
                        _activeMouseTool.reset();
                    }
                },
                [&]()   // End move function
                {
                    // Release the active mouse tool when done
                    _activeMouseTool.reset();
                });
        }

        return; // we have an active tool, don't pass the event
    }

#if 0
	if (mouseEvents.stateMatchesXYViewEvent(ui::xySelect, ev))
	{
		// There are two possibilites for the "select" click: Clip or Select
		if (GlobalClipper().clipMode())
		{
			Clipper_OnLButtonDown(ev.GetX(), ev.GetY());
			return; // Prevent the call from being passed to the windowobserver
		}
	}
#endif

	// Pass the call to the window observer
	m_window_observer->onMouseDown(WindowVector(ev.GetX(), ev.GetY()), ev);
}

void XYWnd::handleGLMouseUp(wxMouseEvent& ev)
{
    if (_activeMouseTool)
    {
        // Construct the mousedown event and see which tool is able to handle it
        ui::XYMouseToolEvent mouseEvent(*this, convertXYToWorld(ev.GetX(), ev.GetY()));
        
        // Ask the active mousetool to handle this event
        ui::MouseTool::Result result = _activeMouseTool->onMouseUp(mouseEvent);

        if (result == ui::MouseTool::Result::Finished)
        {
            // Freezing mouse tools: release the mouse cursor again
            if (_activeMouseTool->getPointerMode() == ui::MouseTool::PointerMode::Capture)
            {
                _freezePointer.unfreeze();
            }

            // Tool is done
            _activeMouseTool.reset();
            return;
        }
    }

	IMouseEvents& mouseEvents = GlobalEventManager().MouseEvents();

	// End move
	if (_moveStarted)
	{
		endMove();
		EntityCreate_MouseUp(ev.GetX(), ev.GetY());
	}

	// End zoom
	if (_zoomStarted)
	{
		endZoom();
	}

#if 0
	// Finish any pending NewBrushDrag operations
	if (m_bNewBrushDrag)
	{
		// End the NewBrushDrag operation
		m_bNewBrushDrag = false;
		NewBrushDrag_End(ev.GetX(), ev.GetY());
		return; // Prevent the call from being passed to the windowobserver
	}
#endif

#if 0
	if (GlobalClipper().clipMode() && 
		mouseEvents.stateMatchesXYViewEvent(ui::xySelect, ev))
	{
		// End the clip operation
		Clipper_OnLButtonUp(ev.GetX(), ev.GetY());
		return; // Prevent the call from being passed to the windowobserver
	}
#endif

	// Pass the call to the window observer
	m_window_observer->onMouseUp(WindowVector(ev.GetX(), ev.GetY()), ev);
}

// This gets called by the wx mousemoved callback or the periodical mousechase event
void XYWnd::handleGLMouseMove(int x, int y, unsigned int state)
{
    // Construct the mousedown event and see which tool is able to handle it
    ui::XYMouseToolEvent mouseEvent(*this, convertXYToWorld(x, y));

    if (_activeMouseTool)
    {
        // Ask the active mousetool to handle this event
        switch (_activeMouseTool->onMouseMove(mouseEvent))
        {
        case ui::MouseTool::Result::Finished:
            // Tool is done
            _activeMouseTool.reset();
            return;

        case ui::MouseTool::Result::Activated:
        case ui::MouseTool::Result::Continued:
            return;

        case ui::MouseTool::Result::Ignored:
            break;
        };
    }

    // Send mouse move events to all tools that want them
    GlobalXYWnd().foreachMouseTool([&] (const ui::MouseToolPtr& tool)
    {
        // The active tool already received that event above
        if (tool != _activeMouseTool && tool->alwaysReceivesMoveEvents())
        {
            tool->onMouseMove(mouseEvent);
        }
    });

	IMouseEvents& mouseEvents = GlobalEventManager().MouseEvents();

	if (mouseEvents.stateMatchesXYViewEvent(ui::xyCameraMove, state))
	{
		CamWndPtr cam = GlobalCamera().getActiveCamWnd();
		if (cam != NULL) {
			positionCamera(x, y, *cam);
			queueDraw();
			return;
		}
	}

	if (mouseEvents.stateMatchesXYViewEvent(ui::xyCameraAngle, state))
	{
		CamWndPtr cam = GlobalCamera().getActiveCamWnd();
		if (cam != NULL) {
			orientCamera(x, y, *cam);
			queueDraw();
			return;
		}
	}

#if 0
	// Check, if we are in a NewBrushDrag operation and continue it
	if (m_bNewBrushDrag && 
		mouseEvents.stateMatchesXYViewEvent(ui::xyNewBrushDrag, state))
	{
		NewBrushDrag(x, y);
		return; // Prevent the call from being passed to the windowobserver
    }
#endif

#if 0
	if (mouseEvents.stateMatchesXYViewEvent(ui::xySelect, state))
	{
		// Check, if we have a clip point operation running
		if (GlobalClipper().clipMode() && GlobalClipper().getMovingClip() != 0)
		{
			Clipper_OnMouseMoved(x, y);
			return; // Prevent the call from being passed to the windowobserver
		}
	}
#endif

    // default windowobserver::mouseMotion call, if no other clauses called "return" till now
    m_window_observer->onMouseMotion(WindowVector(x, y), state);

    m_mousePosition = convertXYToWorld(x, y);
    snapToGrid(m_mousePosition);

    GlobalUIManager().getStatusBarManager().setText(
        "XYZPos",
        (boost::format(_("x: %6.1lf y: %6.1lf z: %6.1lf"))
            % m_mousePosition[0]
            % m_mousePosition[1]
            % m_mousePosition[2]).str()
    );

	if (GlobalXYWnd().showCrossHairs())
	{
		queueDraw();
	}
	
#if 0
    Clipper_Crosshair_OnMouseMoved(x, y);
#endif
}

void XYWnd::EntityCreate_MouseDown(int x, int y) {
    m_entityCreate = true;
    m_entityCreate_x = x;
    m_entityCreate_y = y;
}

void XYWnd::EntityCreate_MouseMove(int x, int y) {
    if (m_entityCreate && (m_entityCreate_x != x || m_entityCreate_y != y)) {
        m_entityCreate = false;
    }
}

void XYWnd::EntityCreate_MouseUp(int x, int y) {
    if (m_entityCreate) {
        m_entityCreate = false;
        onContextMenu();

        // Tell the other window observers to cancel their operation,
        // the context menu will be stealing focus.
        m_window_observer->cancelOperation();
    }
}

Vector3 XYWnd::convertXYToWorld(int x, int y)
{
    float normalised2world_scale_x = _width / 2 / m_fScale;
    float normalised2world_scale_y = _height / 2 / m_fScale;

    if (m_viewType == XY)
    {
        return Vector3(
            normalised_to_world(screen_normalised(x, _width), m_vOrigin[0], normalised2world_scale_x),
            normalised_to_world(-screen_normalised(y, _height), m_vOrigin[1], normalised2world_scale_y),
            0);
    }
    else if (m_viewType == YZ)
    {
        return Vector3(
            0,
            normalised_to_world(screen_normalised(x, _width), m_vOrigin[1], normalised2world_scale_x),
            normalised_to_world(-screen_normalised(y, _height), m_vOrigin[2], normalised2world_scale_y));
    }
    else // XZ
    {
        return Vector3(
            normalised_to_world(screen_normalised(x, _width), m_vOrigin[0], normalised2world_scale_x),
            0,
            normalised_to_world(-screen_normalised(y, _height), m_vOrigin[2], normalised2world_scale_y));
    }
}

void XYWnd::snapToGrid(Vector3& point) {
    if (m_viewType == XY) {
        point[0] = float_snapped(point[0], GlobalGrid().getGridSize());
        point[1] = float_snapped(point[1], GlobalGrid().getGridSize());
    }
    else if (m_viewType == YZ) {
        point[1] = float_snapped(point[1], GlobalGrid().getGridSize());
        point[2] = float_snapped(point[2], GlobalGrid().getGridSize());
    }
    else {
        point[0] = float_snapped(point[0], GlobalGrid().getGridSize());
        point[2] = float_snapped(point[2], GlobalGrid().getGridSize());
    }
}

/* greebo: This calculates the coordinates of the xy view window corners.
 *
 * @returns: Vector4( xbegin, xend, ybegin, yend);
 */
Vector4 XYWnd::getWindowCoordinates() {
    int nDim1 = (m_viewType == YZ) ? 1 : 0;
    int nDim2 = (m_viewType == XY) ? 1 : 2;

    double w = (_width / 2 / m_fScale);
    double h = (_height / 2 / m_fScale);

    // Query the region minimum/maximum vectors
    Vector3 regionMin;
    Vector3 regionMax;
    GlobalRegion().getMinMax(regionMin, regionMax);

    double xb = m_vOrigin[nDim1] - w;
    // Constrain this value to the region minimum
    if (xb < regionMin[nDim1])
        xb = regionMin[nDim1];

    double xe = m_vOrigin[nDim1] + w;
    // Constrain this value to the region maximum
    if (xe > regionMax[nDim1])
        xe = regionMax[nDim1];

    double yb = m_vOrigin[nDim2] - h;
    // Constrain this value to the region minimum
    if (yb < regionMin[nDim2])
        yb = regionMin[nDim2];

    double ye = m_vOrigin[nDim2] + h;
    // Constrain this value to the region maximum
    if (ye > regionMax[nDim2])
        ye = regionMax[nDim2];

    return Vector4(xb, xe, yb, ye);
}

void XYWnd::drawGrid()
{
    double step, minor_step, stepx, stepy;
    step = minor_step = stepx = stepy = GlobalGrid().getGridSize();

    int minor_power = GlobalGrid().getGridPower();

    while (minor_step * m_fScale <= 4.0f) // make sure minor grid spacing is at least 4 pixels on the screen
    {
        ++minor_power;
        minor_step *= 2;
    }

    int power = minor_power;

    while (power % 3 != 0 || step * m_fScale <= 32.0f) // make sure major grid spacing is at least 32 pixels on the screen
    {
        ++power;
        step = double(two_to_the_power(power));
    }

    int mask = (1 << (power - minor_power)) - 1;

    while (stepx * m_fScale <= 32.0f) // text step x must be at least 32
    {
        stepx *= 2;
    }

    while (stepy * m_fScale <= 32.0f) // text step y must be at least 32
    {
        stepy *= 2;
    }

    glDisable(GL_TEXTURE_2D);
    glDisable(GL_TEXTURE_1D);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glLineWidth(1);

    double w = _width / 2 / m_fScale;
    double h = _height / 2 / m_fScale;

    Vector4 windowCoords = getWindowCoordinates();

    double xb = step * floor(windowCoords[0]/step);
    double xe = step * ceil(windowCoords[1]/step);
    double yb = step * floor(windowCoords[2]/step);
    double ye = step * ceil(windowCoords[3]/step);

    if (GlobalXYWnd().showGrid())
    {
        Vector3 colourGridBack = ColourSchemes().getColour("grid_background");
        Vector3 colourGridMinor = ColourSchemes().getColour("grid_minor");
        Vector3 colourGridMajor = ColourSchemes().getColour("grid_major");

        // run grid rendering twice, first run is minor grid, then major
        // NOTE: with a bit more work, we can have variable number of grids
        for (int gf = 0 ; gf < 2 ; ++gf)
        {
            double cur_step, density, sizeFactor;
            GridLook look;

            if (gf)
            {
                // major grid
                if (colourGridMajor == colourGridBack)
                {
                    continue;
                }

                glColor3dv(colourGridMajor);
                look = GlobalGrid().getMajorLook();
                cur_step = step;
                density = 4;
                // slightly bigger crosses
                sizeFactor = 1.95;
            }
            else
            {
                // minor grid (rendered first)
                if (colourGridMinor == colourGridBack)
                {
                    continue;
                }

                glColor3dv(colourGridMinor);
                look = GlobalGrid().getMinorLook();
                cur_step = minor_step;
                density = 4;
                sizeFactor = 0.95;
            }

            switch (look)
            {
                case GRIDLOOK_DOTS:
                    glBegin (GL_POINTS);
                    for (double x = xb ; x < xe ; x += cur_step)
                    {
                        for (double y = yb ; y < ye ; y += cur_step)
                        {
                            glVertex2d (x, y);
                        }
                    }
                    glEnd();
                    break;

                case GRIDLOOK_BIGDOTS:
                    glPointSize(3);
                    glEnable(GL_POINT_SMOOTH);
                    glBegin (GL_POINTS);
                    for (double x = xb ; x < xe ; x += cur_step)
                    {
                        for (double y = yb ; y < ye ; y += cur_step)
                        {
                            glVertex2d (x, y);
                        }
                    }
                    glEnd();
                    glDisable(GL_POINT_SMOOTH);
                    glPointSize(1);
                    break;

                case GRIDLOOK_SQUARES:
                    glPointSize(3);
                    glBegin (GL_POINTS);
                    for (double x = xb ; x < xe ; x += cur_step)
                    {
                        for (double y = yb ; y < ye ; y += cur_step)
                        {
                            glVertex2d (x, y);
                        }
                    }
                    glEnd();
                    glPointSize(1);
                    break;

                case GRIDLOOK_MOREDOTLINES:
                    density = 8;

                case GRIDLOOK_DOTLINES:
                    glBegin (GL_POINTS);
                    for (double x = xb ; x < xe ; x += cur_step)
                    {
                        for (double y = yb ; y < ye ; y += minor_step / density)
                        {
                            glVertex2d (x, y);
                        }
                    }

                    for (double y = yb ; y < ye ; y += cur_step)
                    {
                        for (double x = xb ; x < xe ; x += minor_step / density)
                        {
                            glVertex2d (x, y);
                        }
                    }
                    glEnd();
                    break;

                case GRIDLOOK_CROSSES:
                    glBegin (GL_LINES);
                    for (double x = xb ; x <= xe ; x += cur_step)
                    {
                        for (double y = yb ; y <= ye ; y += cur_step)
                        {
                            glVertex2d (x - sizeFactor / m_fScale, y);
                            glVertex2d (x + sizeFactor / m_fScale, y);
                            glVertex2d (x, y - sizeFactor / m_fScale);
                            glVertex2d (x, y + sizeFactor / m_fScale);
                        }
                    }
                    glEnd();
                    break;

                case GRIDLOOK_LINES:
                default:
                    glBegin (GL_LINES);
                    int i = 0;
                    for (double x = xb ; x < xe ; x += cur_step, ++i)
                    {
                        if (gf == 1 || (i & mask) != 0) // greebo: No mask check for major grid
                        {
                            glVertex2d (x, yb);
                            glVertex2d (x, ye);
                        }
                    }

                    i = 0;

                    for (double y = yb ; y < ye ; y += cur_step, ++i)
                    {
                        if (gf == 1 || (i & mask) != 0) // greebo: No mask check for major grid
                        {
                            glVertex2d (xb, y);
                            glVertex2d (xe, y);
                        }
                    }

                    glEnd();
                    break;
            }
        }
    }

    int nDim1 = (m_viewType == YZ) ? 1 : 0;
    int nDim2 = (m_viewType == XY) ? 1 : 2;

    // draw coordinate text if needed
    if (GlobalXYWnd().showCoordinates())
    {
        char text[32];

        glColor3dv(ColourSchemes().getColour("grid_text"));

        double offx = m_vOrigin[nDim2] + h - 12 / m_fScale;
        double offy = m_vOrigin[nDim1] - w + 1 / m_fScale;

        for (double x = xb - fmod(xb, stepx); x <= xe ; x += stepx)
        {
            glRasterPos2d (x, offx);
            sprintf(text, "%g", x);
            GlobalOpenGL().drawString(text);
        }

        for (double y = yb - fmod(yb, stepy); y <= ye ; y += stepy)
        {
            glRasterPos2f (offy, y);
            sprintf (text, "%g", y);
            GlobalOpenGL().drawString(text);
        }

        if (isActive())
        {
            glColor3dv(ColourSchemes().getColour("active_view_name"));
        }

        // we do this part (the old way) only if show_axis is disabled
        if (!GlobalXYWnd().showAxes())
        {
            glRasterPos2d( m_vOrigin[nDim1] - w + 35 / m_fScale, m_vOrigin[nDim2] + h - 20 / m_fScale );

            GlobalOpenGL().drawString(getViewTypeTitle(m_viewType));
        }
    }

    if (GlobalXYWnd().showAxes())
    {
        const char g_AxisName[3] = { 'X', 'Y', 'Z' };

        const std::string colourNameX = (m_viewType == YZ) ? "axis_y" : "axis_x";
        const std::string colourNameY = (m_viewType == XY) ? "axis_y" : "axis_z";
        const Vector3& colourX = ColourSchemes().getColour(colourNameX);
        const Vector3& colourY = ColourSchemes().getColour(colourNameY);

        // draw two lines with corresponding axis colors to highlight current view
        // horizontal line: nDim1 color
        glLineWidth(2);
        glBegin( GL_LINES );
        glColor3dv (colourX);
        glVertex2f( m_vOrigin[nDim1] - w + 40 / m_fScale, m_vOrigin[nDim2] + h - 45 / m_fScale );
        glVertex2f( m_vOrigin[nDim1] - w + 65 / m_fScale, m_vOrigin[nDim2] + h - 45 / m_fScale );
        glVertex2f( 0, 0 );
        glVertex2f( 32 / m_fScale, 0 );
        glColor3dv (colourY);
        glVertex2f( m_vOrigin[nDim1] - w + 40 / m_fScale, m_vOrigin[nDim2] + h - 45 / m_fScale );
        glVertex2f( m_vOrigin[nDim1] - w + 40 / m_fScale, m_vOrigin[nDim2] + h - 20 / m_fScale );
        glVertex2f( 0, 0 );
        glVertex2f( 0, 32 / m_fScale );
        glEnd();
        glLineWidth(1);
        // now print axis symbols
        glColor3dv (colourX);
        glRasterPos2f ( m_vOrigin[nDim1] - w + 55 / m_fScale, m_vOrigin[nDim2] + h - 55 / m_fScale );
        GlobalOpenGL().drawChar(g_AxisName[nDim1]);
        glRasterPos2f (28 / m_fScale, -10 / m_fScale );
        GlobalOpenGL().drawChar(g_AxisName[nDim1]);
        glColor3dv (colourY);
        glRasterPos2f ( m_vOrigin[nDim1] - w + 25 / m_fScale, m_vOrigin[nDim2] + h - 30 / m_fScale );
        GlobalOpenGL().drawChar(g_AxisName[nDim2]);
        glRasterPos2f ( -10 / m_fScale, 28 / m_fScale );
        GlobalOpenGL().drawChar(g_AxisName[nDim2]);

    }

    // show current work zone?
    // the work zone is used to place dropped points and brushes
    if (GlobalXYWnd().showWorkzone())
    {
        const selection::WorkZone& wz = GlobalSelectionSystem().getWorkZone();

        if (wz.bounds.isValid())
        {
            glColor3dv( ColourSchemes().getColour("workzone") );
            glBegin( GL_LINES );

            glVertex2f( xb, wz.min[nDim2] );
            glVertex2f( xe, wz.min[nDim2] );
            glVertex2f( xb, wz.max[nDim2] );
            glVertex2f( xe, wz.max[nDim2] );
            glVertex2f( wz.min[nDim1], yb );
            glVertex2f( wz.min[nDim1], ye );
            glVertex2f( wz.max[nDim1], yb );
            glVertex2f( wz.max[nDim1], ye );
            glEnd();
        }
    }
}

void XYWnd::drawBlockGrid() {
    if (GlobalMap().findWorldspawn() == NULL) {
        return;
    }
    // Set a default blocksize of 1024
    int blockSize = GlobalXYWnd().defaultBlockSize();

    // Check the worldspawn for a custom blocksize
    Entity* worldSpawn = Node_getEntity(GlobalMap().getWorldspawn());
    assert(worldSpawn);
    std::string sizeVal = worldSpawn->getKeyValue("_blocksize");

    // Parse and set the custom blocksize if found
    if (!sizeVal.empty()) {
        blockSize = string::convert<int>(sizeVal);
    }

    float   x, y, xb, xe, yb, ye;
    char    text[32];

    glDisable(GL_TEXTURE_2D);
    glDisable(GL_TEXTURE_1D);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);

    Vector4 windowCoords = getWindowCoordinates();

    xb = static_cast<float>(blockSize * floor (windowCoords[0]/blockSize));
    xe = static_cast<float>(blockSize * ceil (windowCoords[1]/blockSize));
    yb = static_cast<float>(blockSize * floor (windowCoords[2]/blockSize));
    ye = static_cast<float>(blockSize * ceil (windowCoords[3]/blockSize));

    // draw major blocks

    glColor3dv(ColourSchemes().getColour("grid_block"));
    glLineWidth (2);

    glBegin (GL_LINES);

    for (x=xb ; x<=xe ; x+=blockSize) {
        glVertex2f (x, yb);
        glVertex2f (x, ye);
    }

    if (m_viewType == XY) {
        for (y=yb ; y<=ye ; y+=blockSize) {
            glVertex2f (xb, y);
            glVertex2f (xe, y);
        }
    }

    glEnd();
    glLineWidth (1);

    // draw coordinate text if needed

    if (m_viewType == XY && m_fScale > .1) {
        for (x=xb ; x<xe ; x+=blockSize)
            for (y=yb ; y<ye ; y+=blockSize) {
                glRasterPos2f (x+(blockSize/2), y+(blockSize/2));
                sprintf (text, "%i,%i",(int)floor(x/blockSize), (int)floor(y/blockSize) );
                GlobalOpenGL().drawString(text);
            }
    }

    glColor4f(0, 0, 0, 0);
}

void XYWnd::drawCameraIcon(const Vector3& origin, const Vector3& angles)
{
    float   x, y, fov, box;
    float a;

    fov = 48 / m_fScale;
    box = 16 / m_fScale;

    if (m_viewType == XY) {
        x = origin[0];
        y = origin[1];
        a = degrees_to_radians(angles[CAMERA_YAW]);
    }
    else if (m_viewType == YZ) {
        x = origin[1];
        y = origin[2];
        a = degrees_to_radians(angles[CAMERA_PITCH]);
    }
    else {
        x = origin[0];
        y = origin[2];
        a = degrees_to_radians(angles[CAMERA_PITCH]);
    }

    glColor3dv(ColourSchemes().getColour("camera_icon"));
    glBegin(GL_LINE_STRIP);
    glVertex3f (x-box,y,0);
    glVertex3f (x,y+(box/2),0);
    glVertex3f (x+box,y,0);
    glVertex3f (x,y-(box/2),0);
    glVertex3f (x-box,y,0);
    glVertex3f (x+box,y,0);
    glEnd();

    glBegin(GL_LINE_STRIP);
    glVertex3f (x + static_cast<float>(fov*cos(a+c_pi/4)), y + static_cast<float>(fov*sin(a+c_pi/4)), 0);
    glVertex3f (x, y, 0);
    glVertex3f (x + static_cast<float>(fov*cos(a-c_pi/4)), y + static_cast<float>(fov*sin(a-c_pi/4)), 0);
    glEnd();
}

// can be greatly simplified but per usual i am in a hurry
// which is not an excuse, just a fact
void XYWnd::drawSizeInfo(int nDim1, int nDim2, const Vector3& vMinBounds, const Vector3& vMaxBounds)
{
  if (vMinBounds == vMaxBounds) {
    return;
  }
  const char* g_pDimStrings[] = {"x:", "y:", "z:"};
  typedef const char* OrgStrings[2];
  const OrgStrings g_pOrgStrings[] = { { "x:", "y:", }, { "x:", "z:", }, { "y:", "z:", } };

  Vector3 vSize(vMaxBounds - vMinBounds);

  glColor3dv(ColourSchemes().getColour("brush_size_info"));

  std::ostringstream dimensions;

  if (m_viewType == XY)
  {
    glBegin (GL_LINES);

    glVertex3d(vMinBounds[nDim1], vMinBounds[nDim2] - 6.0f  / m_fScale, 0.0f);
    glVertex3d(vMinBounds[nDim1], vMinBounds[nDim2] - 10.0f / m_fScale, 0.0f);

    glVertex3d(vMinBounds[nDim1], vMinBounds[nDim2] - 10.0f  / m_fScale, 0.0f);
    glVertex3d(vMaxBounds[nDim1], vMinBounds[nDim2] - 10.0f  / m_fScale, 0.0f);

    glVertex3d(vMaxBounds[nDim1], vMinBounds[nDim2] - 6.0f  / m_fScale, 0.0f);
    glVertex3d(vMaxBounds[nDim1], vMinBounds[nDim2] - 10.0f / m_fScale, 0.0f);


    glVertex3d(vMaxBounds[nDim1] + 6.0f  / m_fScale, vMinBounds[nDim2], 0.0f);
    glVertex3d(vMaxBounds[nDim1] + 10.0f  / m_fScale, vMinBounds[nDim2], 0.0f);

    glVertex3d(vMaxBounds[nDim1] + 10.0f  / m_fScale, vMinBounds[nDim2], 0.0f);
    glVertex3d(vMaxBounds[nDim1] + 10.0f  / m_fScale, vMaxBounds[nDim2], 0.0f);

    glVertex3d(vMaxBounds[nDim1] + 6.0f  / m_fScale, vMaxBounds[nDim2], 0.0f);
    glVertex3d(vMaxBounds[nDim1] + 10.0f  / m_fScale, vMaxBounds[nDim2], 0.0f);

    glEnd();

    glRasterPos3f (Betwixt(vMinBounds[nDim1], vMaxBounds[nDim1]),  vMinBounds[nDim2] - 20.0f  / m_fScale, 0.0f);
    dimensions << g_pDimStrings[nDim1] << vSize[nDim1];
    GlobalOpenGL().drawString(dimensions.str());
    dimensions.str("");

    glRasterPos3f (vMaxBounds[nDim1] + 16.0f  / m_fScale, Betwixt(vMinBounds[nDim2], vMaxBounds[nDim2]), 0.0f);
    dimensions << g_pDimStrings[nDim2] << vSize[nDim2];
    GlobalOpenGL().drawString(dimensions.str());
    dimensions.str("");

    glRasterPos3f (vMinBounds[nDim1] + 4, vMaxBounds[nDim2] + 8 / m_fScale, 0.0f);
    dimensions << "(" << g_pOrgStrings[0][0] << vMinBounds[nDim1] << "  " << g_pOrgStrings[0][1] << vMaxBounds[nDim2] << ")";
    GlobalOpenGL().drawString(dimensions.str());
  }
  else if (m_viewType == XZ)
  {
    glBegin (GL_LINES);

    glVertex3f(vMinBounds[nDim1], 0, vMinBounds[nDim2] - 6.0f  / m_fScale);
    glVertex3f(vMinBounds[nDim1], 0, vMinBounds[nDim2] - 10.0f / m_fScale);

    glVertex3f(vMinBounds[nDim1], 0,vMinBounds[nDim2] - 10.0f  / m_fScale);
    glVertex3f(vMaxBounds[nDim1], 0,vMinBounds[nDim2] - 10.0f  / m_fScale);

    glVertex3f(vMaxBounds[nDim1], 0,vMinBounds[nDim2] - 6.0f  / m_fScale);
    glVertex3f(vMaxBounds[nDim1], 0,vMinBounds[nDim2] - 10.0f / m_fScale);


    glVertex3f(vMaxBounds[nDim1] + 6.0f  / m_fScale, 0,vMinBounds[nDim2]);
    glVertex3f(vMaxBounds[nDim1] + 10.0f  / m_fScale, 0,vMinBounds[nDim2]);

    glVertex3f(vMaxBounds[nDim1] + 10.0f  / m_fScale, 0,vMinBounds[nDim2]);
    glVertex3f(vMaxBounds[nDim1] + 10.0f  / m_fScale, 0,vMaxBounds[nDim2]);

    glVertex3f(vMaxBounds[nDim1] + 6.0f  / m_fScale, 0,vMaxBounds[nDim2]);
    glVertex3f(vMaxBounds[nDim1] + 10.0f  / m_fScale, 0,vMaxBounds[nDim2]);

    glEnd();

    glRasterPos3f (Betwixt(vMinBounds[nDim1], vMaxBounds[nDim1]), 0, vMinBounds[nDim2] - 20.0f  / m_fScale);
    dimensions << g_pDimStrings[nDim1] << vSize[nDim1];
    GlobalOpenGL().drawString(dimensions.str());
    dimensions.str("");

    glRasterPos3f (vMaxBounds[nDim1] + 16.0f  / m_fScale, 0, Betwixt(vMinBounds[nDim2], vMaxBounds[nDim2]));
    dimensions << g_pDimStrings[nDim2] << vSize[nDim2];
    GlobalOpenGL().drawString(dimensions.str());
    dimensions.str("");

    glRasterPos3f (vMinBounds[nDim1] + 4, 0, vMaxBounds[nDim2] + 8 / m_fScale);
    dimensions << "(" << g_pOrgStrings[1][0] << vMinBounds[nDim1] << "  " << g_pOrgStrings[1][1] << vMaxBounds[nDim2] << ")";
    GlobalOpenGL().drawString(dimensions.str());
  }
  else
  {
    glBegin (GL_LINES);

    glVertex3f(0, vMinBounds[nDim1], vMinBounds[nDim2] - 6.0f  / m_fScale);
    glVertex3f(0, vMinBounds[nDim1], vMinBounds[nDim2] - 10.0f / m_fScale);

    glVertex3f(0, vMinBounds[nDim1], vMinBounds[nDim2] - 10.0f  / m_fScale);
    glVertex3f(0, vMaxBounds[nDim1], vMinBounds[nDim2] - 10.0f  / m_fScale);

    glVertex3f(0, vMaxBounds[nDim1], vMinBounds[nDim2] - 6.0f  / m_fScale);
    glVertex3f(0, vMaxBounds[nDim1], vMinBounds[nDim2] - 10.0f / m_fScale);


    glVertex3f(0, vMaxBounds[nDim1] + 6.0f  / m_fScale, vMinBounds[nDim2]);
    glVertex3f(0, vMaxBounds[nDim1] + 10.0f  / m_fScale, vMinBounds[nDim2]);

    glVertex3f(0, vMaxBounds[nDim1] + 10.0f  / m_fScale, vMinBounds[nDim2]);
    glVertex3f(0, vMaxBounds[nDim1] + 10.0f  / m_fScale, vMaxBounds[nDim2]);

    glVertex3f(0, vMaxBounds[nDim1] + 6.0f  / m_fScale, vMaxBounds[nDim2]);
    glVertex3f(0, vMaxBounds[nDim1] + 10.0f  / m_fScale, vMaxBounds[nDim2]);

    glEnd();

    glRasterPos3f (0, Betwixt(vMinBounds[nDim1], vMaxBounds[nDim1]),  vMinBounds[nDim2] - 20.0f  / m_fScale);
    dimensions << g_pDimStrings[nDim1] << vSize[nDim1];
    GlobalOpenGL().drawString(dimensions.str());
    dimensions.str("");

    glRasterPos3f (0, vMaxBounds[nDim1] + 16.0f  / m_fScale, Betwixt(vMinBounds[nDim2], vMaxBounds[nDim2]));
    dimensions << g_pDimStrings[nDim2] << vSize[nDim2];
    GlobalOpenGL().drawString(dimensions.str());
    dimensions.str("");

    glRasterPos3f (0, vMinBounds[nDim1] + 4.0f, vMaxBounds[nDim2] + 8 / m_fScale);
    dimensions << "(" << g_pOrgStrings[2][0] << vMinBounds[nDim1] << "  " << g_pOrgStrings[2][1] << vMaxBounds[nDim2] << ")";
    GlobalOpenGL().drawString(dimensions.str());
  }
}

void XYWnd::updateProjection() {
    m_projection[0] = 1.0f / static_cast<float>(_width / 2);
    m_projection[5] = 1.0f / static_cast<float>(_height / 2);
    m_projection[10] = 1.0f / (_maxWorldCoord * m_fScale);

    m_projection[12] = 0.0f;
    m_projection[13] = 0.0f;
    m_projection[14] = -1.0f;

    m_projection[1] = m_projection[2] = m_projection[3] =
    m_projection[4] = m_projection[6] = m_projection[7] =
    m_projection[8] = m_projection[9] = m_projection[11] = 0.0f;

    m_projection[15] = 1.0f;

    m_view.Construct(m_projection, m_modelview, _width, _height);
}

// note: modelview matrix must have a uniform scale, otherwise strange things happen when rendering the rotation manipulator.
void XYWnd::updateModelview() {
    int nDim1 = (m_viewType == YZ) ? 1 : 0;
    int nDim2 = (m_viewType == XY) ? 1 : 2;

    // translation
    m_modelview[12] = -m_vOrigin[nDim1] * m_fScale;
    m_modelview[13] = -m_vOrigin[nDim2] * m_fScale;
    m_modelview[14] = _maxWorldCoord * m_fScale;

    // axis base
    switch (m_viewType) {
        case XY:
            m_modelview[0]  =  m_fScale;
            m_modelview[1]  =  0;
            m_modelview[2]  =  0;

            m_modelview[4]  =  0;
            m_modelview[5]  =  m_fScale;
            m_modelview[6]  =  0;

            m_modelview[8]  =  0;
            m_modelview[9]  =  0;
            m_modelview[10] = -m_fScale;
            break;
        case XZ:
            m_modelview[0]  =  m_fScale;
            m_modelview[1]  =  0;
            m_modelview[2]  =  0;

            m_modelview[4]  =  0;
            m_modelview[5]  =  0;
            m_modelview[6]  =  m_fScale;

            m_modelview[8]  =  0;
            m_modelview[9]  =  m_fScale;
            m_modelview[10] =  0;
            break;
        case YZ:
            m_modelview[0]  =  0;
            m_modelview[1]  =  0;
            m_modelview[2]  = -m_fScale;

            m_modelview[4]  =  m_fScale;
            m_modelview[5]  =  0;
            m_modelview[6]  =  0;

            m_modelview[8]  =  0;
            m_modelview[9]  =  m_fScale;
            m_modelview[10] =  0;
            break;
    }

    m_modelview[3] = m_modelview[7] = m_modelview[11] = 0;
    m_modelview[15] = 1;

    m_view.Construct(m_projection, m_modelview, _width, _height);
}

void XYWnd::draw()
{
    // clear
    glViewport(0, 0, _width, _height);
    Vector3 colourGridBack = ColourSchemes().getColour("grid_background");
    glClearColor (colourGridBack[0], colourGridBack[1], colourGridBack[2], 0);

    glClear(GL_COLOR_BUFFER_BIT);

    // set up viewpoint
    glMatrixMode(GL_PROJECTION);
    glLoadMatrixd(m_projection);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glScalef(m_fScale, m_fScale, 1);
    int nDim1 = (m_viewType == YZ) ? 1 : 0;
    int nDim2 = (m_viewType == XY) ? 1 : 2;
    glTranslatef(-m_vOrigin[nDim1], -m_vOrigin[nDim2], 0);

    // Call the image overlay draw method with the window coordinates
    Vector4 windowCoords = getWindowCoordinates();
    ui::Overlay::Instance().draw(
        windowCoords[0], windowCoords[1], windowCoords[2], windowCoords[3],
        m_fScale);

    glDisable (GL_LINE_STIPPLE);
    glLineWidth(1);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_NORMAL_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_LIGHTING);
    glDisable(GL_COLOR_MATERIAL);
    glDisable(GL_DEPTH_TEST);

    ui::XYWndManager& xyWndManager = GlobalXYWnd();

    drawGrid();
    if (xyWndManager.showBlocks())
        drawBlockGrid();

    glLoadMatrixd(m_modelview);

    unsigned int flagsMask = RENDER_POINT_COLOUR | RENDER_VERTEX_COLOUR;

    if (!getCameraSettings()->solidSelectionBoxes())
    {
        flagsMask |= RENDER_LINESTIPPLE;
    }

    {
        // Construct the renderer and render the scene
        XYRenderer renderer(flagsMask, _selectedShader.get());

        // First pass (scenegraph traversal)
        render::RenderableCollectionWalker::collectRenderablesInScene(renderer,
                                                                      m_view);

        // Second pass (GL calls)
        renderer.render(m_modelview, m_projection);
    }

    glDepthMask(GL_FALSE);

    GlobalOpenGL().assertNoErrors();

    glLoadMatrixd(m_modelview);

    GlobalOpenGL().assertNoErrors();
    glDisable(GL_LINE_STIPPLE);
    GlobalOpenGL().assertNoErrors();
    glLineWidth(1);
    GlobalOpenGL().assertNoErrors();
    if (GLEW_VERSION_1_3) {
        glActiveTexture(GL_TEXTURE0);
        glClientActiveTexture(GL_TEXTURE0);
    }
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    GlobalOpenGL().assertNoErrors();
    glDisableClientState(GL_NORMAL_ARRAY);
    GlobalOpenGL().assertNoErrors();
    glDisableClientState(GL_COLOR_ARRAY);
    GlobalOpenGL().assertNoErrors();
    glDisable(GL_TEXTURE_2D);
    GlobalOpenGL().assertNoErrors();
    glDisable(GL_LIGHTING);
    GlobalOpenGL().assertNoErrors();
    glDisable(GL_COLOR_MATERIAL);
    GlobalOpenGL().assertNoErrors();

    GlobalOpenGL().assertNoErrors();

    // greebo: Check, if the size info should be displayed (if there are any items selected)
    if (xyWndManager.showSizeInfo() && GlobalSelectionSystem().countSelected() != 0)
    {
        const selection::WorkZone& wz = GlobalSelectionSystem().getWorkZone();

        if (wz.bounds.isValid())
        {
            drawSizeInfo(nDim1, nDim2, wz.min, wz.max);
        }
    }

    if (xyWndManager.showCrossHairs()) {
        Vector3 colour = ColourSchemes().getColour("xyview_crosshairs");
        glColor4d(colour[0], colour[1], colour[2], 0.8f);
        glBegin (GL_LINES);
        if (m_viewType == XY) {
            glVertex2f(2.0f * _minWorldCoord, m_mousePosition[1]);
            glVertex2f(2.0f * _maxWorldCoord, m_mousePosition[1]);
            glVertex2f(m_mousePosition[0], 2.0f * _minWorldCoord);
            glVertex2f(m_mousePosition[0], 2.0f * _maxWorldCoord);
        }
        else if (m_viewType == YZ) {
            glVertex3f(m_mousePosition[0], 2.0f * _minWorldCoord, m_mousePosition[2]);
            glVertex3f(m_mousePosition[0], 2.0f * _maxWorldCoord, m_mousePosition[2]);
            glVertex3f(m_mousePosition[0], m_mousePosition[1], 2.0f * _minWorldCoord);
            glVertex3f(m_mousePosition[0], m_mousePosition[1], 2.0f * _maxWorldCoord);
        }
        else {
            glVertex3f (2.0f * _minWorldCoord, m_mousePosition[1], m_mousePosition[2]);
            glVertex3f (2.0f * _maxWorldCoord, m_mousePosition[1], m_mousePosition[2]);
            glVertex3f(m_mousePosition[0], m_mousePosition[1], 2.0f * _minWorldCoord);
            glVertex3f(m_mousePosition[0], m_mousePosition[1], 2.0f * _maxWorldCoord);
        }
        glEnd();
    }

    // greebo: Draw the clipper's control points
    {
        glColor3dv(ColourSchemes().getColour("clipper"));
        glPointSize(4);

        if (GlobalClipper().clipMode()) {
            GlobalClipper().draw(m_fScale);
        }

        glPointSize(1);
    }

    GlobalOpenGL().assertNoErrors();

    // reset modelview
    glLoadIdentity();
    glScalef(m_fScale, m_fScale, 1);
    glTranslatef(-m_vOrigin[nDim1], -m_vOrigin[nDim2], 0);

    CamWndPtr cam = GlobalCamera().getActiveCamWnd();

    if (cam != NULL) {
        drawCameraIcon(cam->getCameraOrigin(), cam->getCameraAngles());
    }

    // Draw the selection drag rectangle
    if (!_dragRectangle.empty())
    {
        glMatrixMode (GL_PROJECTION);
        glLoadIdentity();
        glOrtho(0, _width, 0, _height, 0, 1);

        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        // Define the blend function for transparency
        glEnable(GL_BLEND);
        glBlendColor(0, 0, 0, 0.2f);
        glBlendFunc(GL_CONSTANT_ALPHA_EXT, GL_ONE_MINUS_CONSTANT_ALPHA_EXT);

        Vector3 dragBoxColour = ColourSchemes().getColour("drag_selection");
        glColor3dv(dragBoxColour);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

        // The transparent fill rectangle
        glBegin(GL_QUADS);
        glVertex2f(_dragRectangle.min.x(), _dragRectangle.min.y());
        glVertex2f(_dragRectangle.max.x(), _dragRectangle.min.y());
        glVertex2f(_dragRectangle.max.x(), _dragRectangle.max.y());
        glVertex2f(_dragRectangle.min.x(), _dragRectangle.max.y());
        glEnd();

        // The solid borders
        glBlendColor(0, 0, 0, 0.8f);
        glBegin(GL_LINE_LOOP);
        glVertex2f(_dragRectangle.min.x(), _dragRectangle.min.y());
        glVertex2f(_dragRectangle.max.x(), _dragRectangle.min.y());
        glVertex2f(_dragRectangle.max.x(), _dragRectangle.max.y());
        glVertex2f(_dragRectangle.min.x(), _dragRectangle.max.y());
        glEnd();

        glDisable(GL_BLEND);
    }

    if (xyWndManager.showOutline()) {
        if (isActive()) {
            glMatrixMode (GL_PROJECTION);
            glLoadIdentity();
            glOrtho (0, _width, 0, _height, 0, 1);

            glMatrixMode (GL_MODELVIEW);
            glLoadIdentity();

            switch (m_viewType) {
                case YZ:
                    glColor3dv(ColourSchemes().getColour("axis_x"));
                    break;
                case XZ:
                    glColor3dv(ColourSchemes().getColour("axis_y"));
                    break;
                case XY:
                    glColor3dv(ColourSchemes().getColour("axis_z"));
                    break;
            }

            glBegin (GL_LINE_LOOP);
            glVertex2i (0, 0);
            glVertex2i (_width-1, 0);
            glVertex2i (_width-1, _height-1);
            glVertex2i (0, _height-1);
            glEnd();
        }
    }

    GlobalOpenGL().assertNoErrors();

    glFinish();
}

void XYWnd::mouseToPoint(int x, int y, Vector3& point)
{
    point = convertXYToWorld(x, y);
    snapToGrid(point);

    int nDim = (getViewType() == XY) ? 2 : (getViewType() == YZ) ? 0 : 1;

    const selection::WorkZone& wz = GlobalSelectionSystem().getWorkZone();

    point[nDim] = float_snapped(wz.bounds.getOrigin()[nDim], GlobalGrid().getGridSize());
}

void XYWnd::updateSelectionBox(const selection::Rectangle& area)
{
	if (_wxGLWidget->IsShown())
	{
		// Take the rectangle and convert it to screen coordinates
		_dragRectangle = area;
		_dragRectangle.toScreenCoords(getWidth(), getHeight());

        queueDraw();
    }
}

// NOTE: the zoom out factor is 4/5, we could think about customizing it
//  we don't go below a zoom factor corresponding to 10% of the max world size
//  (this has to be computed against the window size)
//void XYWnd_ZoomOut(XYWnd* xy);
void XYWnd::zoomOut() {
    float min_scale = std::min(getWidth(),getHeight()) / ( 1.1f * (_maxWorldCoord - _minWorldCoord));
    float scale = getScale() * 4.0f / 5.0f;
    if (scale < min_scale) {
        if (getScale() != min_scale) {
            setScale(min_scale);
        }
    } else {
        setScale(scale);
    }
}

void XYWnd::zoomIn() {
    float max_scale = 64;
    float scale = getScale() * 5.0f / 4.0f;

    if (scale > max_scale) {
        if (getScale() != max_scale) {
            setScale(max_scale);
        }
    }
    else {
        setScale(scale);
    }
}

// ================ CALLBACKS ======================================

// This is the chase mouse handler that gets connected by XYWnd::chaseMouseMotion()
// It passes te call on to the XYWnd::chaseMouse() method.
void XYWnd::onIdle(wxIdleEvent& ev)
{
	if (_chasingMouse)
	{
		chaseMouse();
	}
}

void XYWnd::onGLZoomMouseCaptureLost()
{
	endZoom();
}

void XYWnd::onGLZoomDelta(int x, int y, unsigned int state)
{
	if (y != 0)
	{
		_dragZoom += y;

		while (abs(_dragZoom) > 8)
		{
			if (_dragZoom > 0)
			{
				zoomOut();
				_dragZoom -= 8;
			}
			else
			{
				zoomIn();
				_dragZoom += 8;
			}
		}
	}
}

void XYWnd::performDeferredDraw()
{
	_wxGLWidget->Refresh();
}

void XYWnd::onGLResize(wxSizeEvent& ev)
{
	const wxSize clientSize = _wxGLWidget->GetClientSize();

	_width = clientSize.GetWidth();
	_height = clientSize.GetHeight();

	updateProjection();

	m_window_observer->onSizeChanged(getWidth(), getHeight());

	ev.Skip();
}

void XYWnd::onRender()
{
	if (GlobalMap().isValid() && GlobalMainFrame().screenUpdatesEnabled())
	{
		draw();
	}
}

void XYWnd::onGLWindowScroll(wxMouseEvent& ev)
{
	if (ev.GetWheelRotation() > 0)
	{
		zoomIn();
	}
	else if (ev.GetWheelRotation() < 0)
	{
		zoomOut();
	}
}

void XYWnd::onGLMouseButtonPress(wxMouseEvent& ev)
{
	// The focus might be on some editable child window - since the
	// GL widget cannot be focused itself, let's reset the focus on the toplevel window
	// which will propagate any key events accordingly.
	GlobalMainFrame().getWxTopLevelWindow()->SetFocus();

	// Mark this XY view as active
	GlobalXYWnd().setActiveXY(_id);

	_wxMouseButtonState = wxutil::MouseButton::GetStateForMouseEvent(ev);

	handleGLMouseDown(ev);

	queueDraw();
}

void XYWnd::onGLMouseButtonRelease(wxMouseEvent& ev)
{
	// Call the according mouseUp method
	handleGLMouseUp(ev);

	// Clear the buttons that the button_release has been called with
	_wxMouseButtonState = wxutil::MouseButton::GetStateForMouseEvent(ev);

	queueDraw();
}

void XYWnd::onGLMouseMove(int x, int y, unsigned int state)
{
	// Call the chaseMouse method
	if (chaseMouseMotion(x, y, state))
	{
		return;
	}

	handleGLMouseMove(x, y, state);
}

void XYWnd::onGLMouseMoveDelta(int x, int y, unsigned int state)
{
    EntityCreate_MouseMove(x, y);
    scroll(-x, y);
}

void XYWnd::onGLMouseCaptureLost()
{
	endMove();
}

/* STATICS */
ShaderPtr XYWnd::_selectedShader;
