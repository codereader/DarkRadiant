#include "XYWnd.h"

#include "i18n.h"
#include "iscenegraph.h"
#include "iundo.h"
#include "ui/imainframe.h"
#include "icolourscheme.h"
#include "ientity.h"
#include "igrid.h"
#include "iregion.h"
#include "ui/istatusbarmanager.h"

#include "wxutil/MouseButton.h"
#include "wxutil/GLWidget.h"
#include "string/string.h"
#include "selectionlib.h"

#include "ibrush.h"
#include "camera/CameraSettings.h"
#include "ui/ortho/OrthoContextMenu.h"
#include "ui/overlay/Overlay.h"
#include "registry/registry.h"
#include "selection/Device.h"
#include "selection/SelectionVolume.h"
#include "util/ScopedBoolLock.h"
#include "debugging/gl.h"

#include "GlobalXYWnd.h"
#include "XYRenderer.h"
#include "gamelib.h"
#include "scenelib.h"
#include "maplib.h"
#include "render/RenderableCollectionWalker.h"

#include <wx/frame.h>
#include <fmt/format.h>
#include <sigc++/functors/mem_fun.h>
#include <functional>

namespace ui
{

inline float Betwixt(float f1, float f2) {
    return (f1 + f2) * 0.5f;
}

double two_to_the_power(int power) {
    return pow(2.0f, power);
}

// Converts a pixel dimensions into device coords, maps [0..size] to [-1..+1]
inline float screen_normalised(int pos, unsigned int size)
{
    return ((2.0f * pos) / size) - 1.0f;
}

inline float normalised_to_world(float normalised, float world_origin, float normalised2world_scale) {
    return world_origin + normalised * normalised2world_scale;
}

namespace
{
    constexpr const char* const RKEY_XYVIEW_ROOT = "user/ui/xyview";
    constexpr const char* const RKEY_SELECT_EPSILON = "user/ui/selectionEpsilon";
}

int XYWnd::_nextId = 1;

XYWnd::XYWnd(wxWindow* parent, XYWndManager& owner) :
    DockablePanel(parent),
    MouseToolHandler(IMouseToolGroup::Type::OrthoView),
    _owner(owner),
    _id(_nextId++),
	_wxGLWidget(new wxutil::GLWidget(this, std::bind(&XYWnd::onRender, this), "XYWnd")),
    _drawing(false),
    _updateRequested(false),
	_minWorldCoord(game::current::getValue<float>("/defaults/minWorldCoord")),
	_maxWorldCoord(game::current::getValue<float>("/defaults/maxWorldCoord")),
	_defaultCursor(wxCURSOR_DEFAULT),
	_crossHairCursor(wxCURSOR_CROSS),
	_chasingMouse(false),
	_isActive(false)
{
    _owner.registerXYWnd(this);

    SetSizer(new wxBoxSizer(wxVERTICAL));
    GetSizer()->Add(_wxGLWidget, 1, wxEXPAND);

    _width = 0;
    _height = 0;

    // Try to retrieve a recently used origin and scale from the registry
    std::string recentPath = std::string(RKEY_XYVIEW_ROOT) + "/recent";
    _origin = string::convert<Vector3>(
        GlobalRegistry().getAttribute(recentPath, "origin")
    );
    _scale = string::convert<double>(
        GlobalRegistry().getAttribute(recentPath, "scale")
    );

    if (_scale == 0)
    {
        _scale = 1;
    }

    _viewType = XY;

	_wxGLWidget->SetCanFocus(false);
	// Don't set a minimum size, to allow for cam window maximisation
	//_wxGLWidget->SetMinClientSize(wxSize(XYWND_MINSIZE_X, XYWND_MINSIZE_Y));

	// wxGLWidget wireup
	_wxGLWidget->Bind(wxEVT_SIZE, &XYWnd::onGLResize, this);

	_wxGLWidget->Bind(wxEVT_MOUSEWHEEL, &XYWnd::onGLWindowScroll, this);
    _wxGLWidget->Bind(wxEVT_MOTION, &XYWnd::onGLMouseMove, this);

	_wxGLWidget->Bind(wxEVT_LEFT_DOWN, &XYWnd::onGLMouseButtonPress, this);
    _wxGLWidget->Bind(wxEVT_LEFT_DCLICK, &XYWnd::onGLMouseButtonPress, this);
	_wxGLWidget->Bind(wxEVT_LEFT_UP, &XYWnd::onGLMouseButtonRelease, this);
	_wxGLWidget->Bind(wxEVT_RIGHT_DOWN, &XYWnd::onGLMouseButtonPress, this);
    _wxGLWidget->Bind(wxEVT_RIGHT_DCLICK, &XYWnd::onGLMouseButtonPress, this);
	_wxGLWidget->Bind(wxEVT_RIGHT_UP, &XYWnd::onGLMouseButtonRelease, this);
	_wxGLWidget->Bind(wxEVT_MIDDLE_DOWN, &XYWnd::onGLMouseButtonPress, this);
    _wxGLWidget->Bind(wxEVT_MIDDLE_DCLICK, &XYWnd::onGLMouseButtonPress, this);
	_wxGLWidget->Bind(wxEVT_MIDDLE_UP, &XYWnd::onGLMouseButtonRelease, this);
	_wxGLWidget->Bind(wxEVT_AUX1_DOWN, &XYWnd::onGLMouseButtonPress, this);
	_wxGLWidget->Bind(wxEVT_AUX1_DCLICK, &XYWnd::onGLMouseButtonPress, this);
	_wxGLWidget->Bind(wxEVT_AUX1_UP, &XYWnd::onGLMouseButtonRelease, this);
	_wxGLWidget->Bind(wxEVT_AUX2_DOWN, &XYWnd::onGLMouseButtonPress, this);
	_wxGLWidget->Bind(wxEVT_AUX2_DCLICK, &XYWnd::onGLMouseButtonPress, this);
	_wxGLWidget->Bind(wxEVT_AUX2_UP, &XYWnd::onGLMouseButtonRelease, this);

	_wxGLWidget->Bind(wxEVT_IDLE, &XYWnd::onIdle, this);

	_freezePointer.connectMouseEvents(
		std::bind(&XYWnd::onGLMouseButtonPress, this, std::placeholders::_1),
		std::bind(&XYWnd::onGLMouseButtonRelease, this, std::placeholders::_1));

    updateProjection();
    updateModelview();

    // Add self to the scenechange callbacks
    GlobalSceneGraph().addSceneObserver(this);

    // greebo: Connect <self> as CameraObserver to the CamWindow. This way this class gets notified on camera change
    _sigCameraChanged = GlobalCameraManager().signal_cameraChanged().connect(
        sigc::mem_fun(this, &XYWnd::onCameraMoved));
}

int XYWnd::getId() const
{
    return _id;
}

XYWnd::~XYWnd()
{
    _owner.unregisterXYWnd(this);

    _font.reset();

    destroyXYView();

    // Store the current position and scale to the registry, so that it may be
    // picked up again when creating XYViews after switching layouts
    std::string recentPath = std::string(RKEY_XYVIEW_ROOT) + "/recent";
    GlobalRegistry().setAttribute(recentPath, "origin",
                                  string::to_string(_origin));
    GlobalRegistry().setAttribute(recentPath, "scale",
                                  string::to_string(_scale));
}

void XYWnd::destroyXYView()
{
    // Remove <self> from the scene change callback list
    GlobalSceneGraph().removeSceneObserver(this);

    _sigCameraChanged.disconnect();
    _wxGLWidget = nullptr;
}

void XYWnd::setScale(float f)
{
    _scale = f;
    updateProjection();
    updateModelview();
    queueDraw();
}

float XYWnd::getScale() const {
    return _scale;
}

int XYWnd::getWidth() const {
    return _width;
}

int XYWnd::getHeight() const {
    return _height;
}

SelectionTestPtr XYWnd::createSelectionTestForPoint(const Vector2& point)
{
    float selectEpsilon = registry::getValue<float>(RKEY_SELECT_EPSILON);

    // Generate the epsilon
    Vector2 deviceEpsilon(selectEpsilon / getWidth(), selectEpsilon / getHeight());

    // Copy the current view and constrain it to a small rectangle
    render::View scissored(_view);
    ConstructSelectionTest(scissored, selection::Rectangle::ConstructFromPoint(point, deviceEpsilon));

    return SelectionTestPtr(new SelectionVolume(scissored));
}

const VolumeTest& XYWnd::getVolumeTest() const
{
    return _view;
}

bool XYWnd::supportsDragSelections()
{
    return true;
}

int XYWnd::getDeviceWidth() const
{
    return getWidth();
}

int XYWnd::getDeviceHeight() const
{
    return getHeight();
}

void XYWnd::captureStates()
{
    _highlightShaders.selectedShader = GlobalRenderSystem().capture(BuiltInShaderType::WireframeSelectionOverlay);
    _highlightShaders.selectedShaderGroup = GlobalRenderSystem().capture(BuiltInShaderType::WireframeSelectionOverlayOfGroups);
    _highlightShaders.mergeActionShaderAdd = GlobalRenderSystem().capture(BuiltInShaderType::OrthoMergeActionOverlayAdd);
    _highlightShaders.mergeActionShaderChange = GlobalRenderSystem().capture(BuiltInShaderType::OrthoMergeActionOverlayChange);
    _highlightShaders.mergeActionShaderRemove = GlobalRenderSystem().capture(BuiltInShaderType::OrthoMergeActionOverlayRemove);
    _highlightShaders.mergeActionShaderConflict = GlobalRenderSystem().capture(BuiltInShaderType::OrthoMergeActionOverlayConflict);
}

void XYWnd::releaseStates()
{
	_highlightShaders.selectedShader.reset();
	_highlightShaders.selectedShaderGroup.reset();
    _highlightShaders.mergeActionShaderAdd.reset();
    _highlightShaders.mergeActionShaderChange.reset();
    _highlightShaders.mergeActionShaderRemove.reset();
    _highlightShaders.mergeActionShaderConflict.reset();
}

void XYWnd::ensureFont()
{
    if (_font)
    {
        return;
    }

    const auto& manager = GlobalXYWnd();
    _font = GlobalOpenGL().getFont(manager.fontStyle(), manager.fontSize());
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

void XYWnd::forceRedraw()
{
    if (_drawing)
    {
        return;
    }

	_wxGLWidget->Refresh(false);
    _wxGLWidget->Update();
}

void XYWnd::queueDraw()
{
    _updateRequested = true;
}

void XYWnd::onSceneGraphChange()
{
    // Pass the call to queueDraw.
    queueDraw();
}

void XYWnd::updateFont()
{
    // Clear out the font reference, it will be re-acquired
    // during the next draw call
    _font.reset();
}

void XYWnd::setActive(bool b) {
    _isActive = b;
};

bool XYWnd::isActive() const {
    return _isActive;
};

const Vector3& XYWnd::getOrigin() const
{
    return _origin;
}

void XYWnd::setOrigin(const Vector3& origin) {
    _origin = origin;
    updateModelview();
    queueDraw();
}

void XYWnd::scrollByPixels(int x, int y)
{
    int nDim1 = (_viewType == YZ) ? 1 : 0;
    int nDim2 = (_viewType == XY) ? 1 : 2;
    _origin[nDim1] -= x / _scale;
    _origin[nDim2] += y / _scale;
    updateModelview();
    queueDraw();
}

/* greebo: This gets repeatedly called during a mouse chase operation.
 * The method is making use of a timer to determine the amount of time that has
 * passed since the chaseMouse has been started
 */
void XYWnd::performChaseMouse()
{
	float multiplier = _chaseMouseTimer.Time() / 10.0f;
    scrollByPixels(float_to_integer(-multiplier * _chasemouseDeltaX), float_to_integer(multiplier * -_chasemouseDeltaY));

	handleGLMouseMotion(_chasemouseCurrentX, _chasemouseCurrentY, _eventState, false);

    // Check if we should still be in chase mouse mode
    if (_chasingMouse && checkChaseMouse(_eventState))
    {
        // greebo: Restart the timer
        _chaseMouseTimer.Start();
    }
}

/* greebo: This handles the "chase mouse" behaviour, if the user drags something
 * beyond the XY window boundaries. If the chaseMouse option (currently a global)
 * is set true, the view origin gets relocated along with the mouse movements.
 *
 * @returns: true, if the mousechase has been performed, false if no mouse chase was necessary
 */
bool XYWnd::checkChaseMouse(unsigned int state)
{
    wxPoint windowMousePos = _wxGLWidget->ScreenToClient(wxGetMousePosition());

    int x = windowMousePos.x;
    int y = windowMousePos.y;

	_chasemouseDeltaX = 0;
	_chasemouseDeltaY = 0;
    _eventState = state;

	// greebo: The mouse chase is only active when the corresponding setting is active
    if (GlobalXYWnd().chaseMouse())
	{
        // If the cursor moves close enough to the window borders, chase mouse will kick in
        // The chase mouse delta is capped between 0 and a value that depends on how much
        // the mouse cursor exceeds that imaginary border.
		const int epsilon = 16;

		// Calculate the X delta
		if (x < epsilon)
		{
            _chasemouseDeltaX = std::max(x - epsilon, -GlobalXYWnd().chaseMouseCap());
		}
		else if (x > _width - epsilon)
		{
            _chasemouseDeltaX = std::min(x - _width + epsilon, GlobalXYWnd().chaseMouseCap());
		}

		// Calculate the Y delta
		if (y < epsilon)
		{
            _chasemouseDeltaY = std::max(y - epsilon, -GlobalXYWnd().chaseMouseCap());
		}
		else if (y > _height - epsilon)
		{
            _chasemouseDeltaY = std::min(y - _height + epsilon, GlobalXYWnd().chaseMouseCap());
		}

		// If any of the deltas is uneqal to zero the mouse chase is to be performed
		if (_chasemouseDeltaY != 0 || _chasemouseDeltaX != 0)
		{
			_chasemouseCurrentX = x;
			_chasemouseCurrentY = y;

			// Start the timer, if there isn't one already connected
			if (!_chasingMouse)
			{
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
			}
		}
	}
	else
	{
		if (_chasingMouse)
		{
			// Remove the handlers, the user has probably released the mouse button during chase
			_chasingMouse = false;
		}
	}

    // No mouse chasing has been performed, return false
    return false;
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

void XYWnd::onCameraMoved()
{
    if (GlobalXYWnd().camXYUpdate())
    {
        queueDraw();
    }
}

void XYWnd::onContextMenu()
{
	// Get the click point in 3D space
	Vector3 point;
	mouseToPoint(_rightClickPos->initial.x(), _rightClickPos->initial.y(), point);

	// Display the menu, passing the coordinates for creation
	OrthoContextMenu::Instance().Show(_wxGLWidget, point);
}

// makes sure the selected brush or camera is in view
void XYWnd::positionView(const Vector3& position) {
    int nDim1 = (_viewType == YZ) ? 1 : 0;
    int nDim2 = (_viewType == XY) ? 1 : 2;

    _origin[nDim1] = position[nDim1];
    _origin[nDim2] = position[nDim2];

    updateModelview();

    queueDraw();
}

void XYWnd::setViewType(EViewType viewType) {
    _viewType = viewType;
    updateModelview();
}

EViewType XYWnd::getViewType() const {
    return _viewType;
}

// This gets called by the wx mousemoved callback or the periodical mousechase event
void XYWnd::handleGLMouseMotion(int x, int y, unsigned int state, bool isDelta)
{
    // Context menu handling
    if (state == wxutil::MouseButton::RIGHT && _rightClickPos) // Only RMB, nothing else
    {
        if (isDelta)
            _rightClickPos->current += Vector2i{x, y};
        else
            _rightClickPos->current = {x, y};

        // If the user moved the pointer away from the point the RMB was pressed, this is a drag
        // not a context menu event
        static const std::string RKEY_THRESHOLD = std::string(RKEY_XYVIEW_ROOT)
                                                + "/contextMenuThreshold";
        static const int DIST_THRESHOLD_SQ = std::pow(
            registry::getValue<int>(RKEY_THRESHOLD, 15), 2
        );
        auto distanceVec = _rightClickPos->current - _rightClickPos->initial;
        if (distanceVec.getLengthSquared() > DIST_THRESHOLD_SQ)
            _rightClickPos.reset();
    }

    _mousePosition = convertXYToWorld(x, y);
    snapToGrid(_mousePosition);

    GlobalStatusBarManager().setText(
        "XYZPos",
        fmt::format(_("x: {0:6.1f} y: {1:6.1f} z: {2:6.1f}"),
            _mousePosition[0],
            _mousePosition[1],
            _mousePosition[2]),
        true // force UI update
    );

	if (GlobalXYWnd().showCrossHairs())
	{
		queueDraw();
	}
}

void XYWnd::handleGLCapturedMouseMotion(const MouseToolPtr& tool, int x, int y, unsigned int mouseState)
{
    if (!tool) return;

    bool mouseToolReceivesDeltas = (tool->getPointerMode() & MouseTool::PointerMode::MotionDeltas) != 0;
    bool pointerFrozen = (tool->getPointerMode() & MouseTool::PointerMode::Freeze) != 0;

    // Check if the mouse has reached exceeded the window borders for chase mouse behaviour
    // In FreezePointer mode there's no need to check for chase since the cursor is fixed anyway
    if (tool->allowChaseMouse() && !pointerFrozen && checkChaseMouse(mouseState))
    {
        // Chase mouse activated, an idle callback will kick in soon
        return;
    }

    // Send mouse move events to the active tool and all inactive tools that want them
    MouseToolHandler::onGLCapturedMouseMove(x, y, mouseState);

    handleGLMouseMotion(x, y, mouseState, mouseToolReceivesDeltas);
}

XYMouseToolEvent XYWnd::createMouseEvent(const Vector2& point, const Vector2& delta)
{
    Vector2 normalisedDeviceCoords = device_constrained(
        window_to_normalised_device(point, _width, _height));

    return XYMouseToolEvent(*this, convertXYToWorld(point.x(), point.y()), normalisedDeviceCoords, delta);
}

Vector3 XYWnd::convertXYToWorld(int x, int y)
{
    float normalised2world_scale_x = _width / 2 / _scale;
    float normalised2world_scale_y = _height / 2 / _scale;

    if (_viewType == XY)
    {
        return Vector3(
            normalised_to_world(screen_normalised(x, _width), _origin[0], normalised2world_scale_x),
            normalised_to_world(-screen_normalised(y, _height), _origin[1], normalised2world_scale_y),
            0);
    }
    else if (_viewType == YZ)
    {
        return Vector3(
            0,
            normalised_to_world(screen_normalised(x, _width), _origin[1], normalised2world_scale_x),
            normalised_to_world(-screen_normalised(y, _height), _origin[2], normalised2world_scale_y));
    }
    else // XZ
    {
        return Vector3(
            normalised_to_world(screen_normalised(x, _width), _origin[0], normalised2world_scale_x),
            0,
            normalised_to_world(-screen_normalised(y, _height), _origin[2], normalised2world_scale_y));
    }
}

void XYWnd::snapToGrid(Vector3& point)
{
    if (_viewType == XY)
    {
        point[0] = float_snapped(point[0], GlobalGrid().getGridSize());
        point[1] = float_snapped(point[1], GlobalGrid().getGridSize());
    }
    else if (_viewType == YZ)
    {
        point[1] = float_snapped(point[1], GlobalGrid().getGridSize());
        point[2] = float_snapped(point[2], GlobalGrid().getGridSize());
    }
    else
    {
        point[0] = float_snapped(point[0], GlobalGrid().getGridSize());
        point[2] = float_snapped(point[2], GlobalGrid().getGridSize());
    }
}

/* greebo: This calculates the coordinates of the xy view window corners.
 *
 * @returns: Vector4( xbegin, xend, ybegin, yend);
 */
Vector4 XYWnd::getWindowCoordinates() {
    int nDim1 = (_viewType == YZ) ? 1 : 0;
    int nDim2 = (_viewType == XY) ? 1 : 2;

    double w = (_width / 2 / _scale);
    double h = (_height / 2 / _scale);

    // Query the region minimum/maximum vectors
    auto regionBounds = GlobalRegionManager().getRegionBounds();

    Vector3 regionMin = regionBounds.origin - regionBounds.extents;
    Vector3 regionMax = regionBounds.origin + regionBounds.extents;

    double xb = _origin[nDim1] - w;
    // Constrain this value to the region minimum
    if (xb < regionMin[nDim1])
        xb = regionMin[nDim1];

    double xe = _origin[nDim1] + w;
    // Constrain this value to the region maximum
    if (xe > regionMax[nDim1])
        xe = regionMax[nDim1];

    double yb = _origin[nDim2] - h;
    // Constrain this value to the region minimum
    if (yb < regionMin[nDim2])
        yb = regionMin[nDim2];

    double ye = _origin[nDim2] + h;
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

    while (minor_step * _scale <= 4.0f) // make sure minor grid spacing is at least 4 pixels on the screen
    {
        ++minor_power;
        minor_step *= 2;
    }

    int power = minor_power;

    while (power % 3 != 0 || step * _scale <= 32.0f) // make sure major grid spacing is at least 32 pixels on the screen
    {
        ++power;
        step = double(two_to_the_power(power));
    }

    int mask = (1 << (power - minor_power)) - 1;

    while (stepx * _scale <= 32.0f) // text step x must be at least 32
    {
        stepx *= 2;
    }

    while (stepy * _scale <= 32.0f) // text step y must be at least 32
    {
        stepy *= 2;
    }

    glDisable(GL_TEXTURE_2D);
    glDisable(GL_TEXTURE_1D);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glLineWidth(1);

    double w = _width / 2 / _scale;
    double h = _height / 2 / _scale;

    Vector4 windowCoords = getWindowCoordinates();

    double xb = step * floor(windowCoords[0]/step);
    double xe = step * ceil(windowCoords[1]/step);
    double yb = step * floor(windowCoords[2]/step);
    double ye = step * ceil(windowCoords[3]/step);

    if (GlobalXYWnd().showGrid())
    {
        Vector3 colourGridBack = GlobalColourSchemeManager().getColour("grid_background");
        Vector3 colourGridMinor = GlobalColourSchemeManager().getColour("grid_minor");
        Vector3 colourGridMajor = GlobalColourSchemeManager().getColour("grid_major");

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
                            glVertex2d (x - sizeFactor / _scale, y);
                            glVertex2d (x + sizeFactor / _scale, y);
                            glVertex2d (x, y - sizeFactor / _scale);
                            glVertex2d (x, y + sizeFactor / _scale);
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

    int nDim1 = (_viewType == YZ) ? 1 : 0;
    int nDim2 = (_viewType == XY) ? 1 : 2;

    // draw coordinate text if needed
    if (GlobalXYWnd().showCoordinates())
    {
        char text[32];

        glColor3dv(GlobalColourSchemeManager().getColour("grid_text"));

        double offx = _origin[nDim2] + h - 12 / _scale;
        double offy = _origin[nDim1] - w + 1 / _scale;

        for (double x = xb - fmod(xb, stepx); x <= xe ; x += stepx)
        {
            glRasterPos2d (x, offx);
            sprintf(text, "%g", x);
            _font->drawString(text);
        }

        for (double y = yb - fmod(yb, stepy); y <= ye ; y += stepy)
        {
            glRasterPos2f (offy, y);
            sprintf (text, "%g", y);
            _font->drawString(text);
        }

        if (isActive())
        {
            glColor3dv(GlobalColourSchemeManager().getColour("active_view_name"));
        }

        // we do this part (the old way) only if show_axis is disabled
        if (!GlobalXYWnd().showAxes())
        {
            glRasterPos2d( _origin[nDim1] - w + 35 / _scale, _origin[nDim2] + h - 20 / _scale );

            _font->drawString(getViewTypeTitle(_viewType));
        }
    }

    if (GlobalMapModule().getEditMode() == IMap::EditMode::Merge)
    {
        glColor3d(0.9, 0, 0);
        glRasterPos2d(_origin[nDim1] - 50 / _scale, _origin[nDim2] + h - 30 / _scale);
        _font->drawString("Merge Mode");
    }

    if (GlobalXYWnd().showAxes())
    {
        const char* g_AxisName[3] = { "X", "Y", "Z" };

        const std::string colourNameX = (_viewType == YZ) ? "axis_y" : "axis_x";
        const std::string colourNameY = (_viewType == XY) ? "axis_y" : "axis_z";
        const Vector3& colourX = GlobalColourSchemeManager().getColour(colourNameX);
        const Vector3& colourY = GlobalColourSchemeManager().getColour(colourNameY);

        // draw two lines with corresponding axis colors to highlight current view
        // horizontal line: nDim1 color
        glLineWidth(2);
        glBegin( GL_LINES );
        glColor3dv (colourX);
        glVertex2f( _origin[nDim1] - w + 40 / _scale, _origin[nDim2] + h - 45 / _scale );
        glVertex2f( _origin[nDim1] - w + 65 / _scale, _origin[nDim2] + h - 45 / _scale );
        glVertex2f( 0, 0 );
        glVertex2f( 32 / _scale, 0 );
        glColor3dv (colourY);
        glVertex2f( _origin[nDim1] - w + 40 / _scale, _origin[nDim2] + h - 45 / _scale );
        glVertex2f( _origin[nDim1] - w + 40 / _scale, _origin[nDim2] + h - 20 / _scale );
        glVertex2f( 0, 0 );
        glVertex2f( 0, 32 / _scale );
        glEnd();
        glLineWidth(1);
        // now print axis symbols
        glColor3dv (colourX);
        glRasterPos2f ( _origin[nDim1] - w + 55 / _scale, _origin[nDim2] + h - 55 / _scale );
        _font->drawString(g_AxisName[nDim1]);
        glRasterPos2f (28 / _scale, -10 / _scale );
        _font->drawString(g_AxisName[nDim1]);
        glColor3dv (colourY);
        glRasterPos2f ( _origin[nDim1] - w + 25 / _scale, _origin[nDim2] + h - 30 / _scale );
        _font->drawString(g_AxisName[nDim2]);
        glRasterPos2f ( -10 / _scale, 28 / _scale );
        _font->drawString(g_AxisName[nDim2]);

    }

    // show current work zone?
    // the work zone is used to place dropped points and brushes
    if (GlobalXYWnd().showWorkzone())
    {
        const selection::WorkZone& wz = GlobalSelectionSystem().getWorkZone();

        if (wz.bounds.isValid())
        {
            glColor3dv(GlobalColourSchemeManager().getColour("workzone"));
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

    drawSelectionFocusArea(nDim1, nDim2, xb, xe, yb, ye);
}

void XYWnd::drawSelectionFocusArea(int nDim1, int nDim2, float xMin, float xMax, float yMin, float yMax)
{
    if (!GlobalSelectionSystem().selectionFocusIsActive()) return;

    auto bounds = GlobalSelectionSystem().getSelectionFocusBounds();

    if (!bounds.isValid()) return;

    Vector3f focusMin = bounds.getOrigin() - bounds.getExtents();
    Vector3f focusMax = bounds.getOrigin() + bounds.getExtents();

    auto toggleAccel = GlobalEventManager().findAcceleratorForEvent("ToggleSelectionFocus");
    auto unselectAccel = GlobalEventManager().findAcceleratorForEvent("UnSelectSelection");

    std::string accelInfo;

    if (unselectAccel)
    {
        accelInfo += unselectAccel->getString(false);
    }

    if (toggleAccel)
    {
        accelInfo += accelInfo.empty() ? "" : _(" or ");
        accelInfo += toggleAccel->getString(false);
    }

    std::string focusHint = _("Selection Focus Active");

    if (!accelInfo.empty())
    {
        focusHint += fmt::format(_(" (press {0} to exit)"), accelInfo);
    }

    glColor3d(0.3, 0.3, 0.3);
    glRasterPos2d((focusMax[nDim1] + focusMin[nDim1])*0.5 - focusHint.length()*3.2/_scale, focusMax[nDim2] + 10 / _scale);
    _font->drawString(focusHint);

    // Define the blend function for transparency
    glEnable(GL_BLEND);
    glBlendColor(0, 0, 0, 0.15f);
    glBlendFunc(GL_CONSTANT_ALPHA_EXT, GL_ONE_MINUS_CONSTANT_ALPHA_EXT);

    Vector3 dragBoxColour(0, 0, 0);
    glColor3dv(dragBoxColour);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    // Flood everything outside the bounds with an overlay
    glBegin(GL_QUADS);

    glVertex2f(xMin, yMin);
    glVertex2f(xMin, focusMin[nDim2]);
    glVertex2f(xMax, focusMin[nDim2]);
    glVertex2f(xMax, yMin);

    glVertex2f(xMin, yMax);
    glVertex2f(xMax, yMax);
    glVertex2f(xMax, focusMax[nDim2]);
    glVertex2f(xMin, focusMax[nDim2]);

    glVertex2f(xMin, focusMax[nDim2]);
    glVertex2f(focusMin[nDim1], focusMax[nDim2]);
    glVertex2f(focusMin[nDim1], focusMin[nDim2]);
    glVertex2f(xMin, focusMin[nDim2]);

    glVertex2f(focusMax[nDim1], focusMax[nDim2]);
    glVertex2f(xMax, focusMax[nDim2]);
    glVertex2f(xMax, focusMin[nDim2]);
    glVertex2f(focusMax[nDim1], focusMin[nDim2]);

    glEnd();
#if 0
    // The solid borders
    glBlendColor(0, 0, 0, 0.4f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(focusMax[nDim1] + 2, focusMax[nDim2] + 2);
    glVertex2f(focusMax[nDim1] + 2, focusMin[nDim2] - 2);
    glVertex2f(focusMin[nDim1] - 2, focusMin[nDim2] - 2);
    glVertex2f(focusMin[nDim1] - 2, focusMax[nDim2] + 2);
    glEnd();
#endif
    glDisable(GL_BLEND);
}

void XYWnd::drawBlockGrid()
{
    // Do nothing if there is no worldspawn yet
    Entity* worldSpawn = map::current::getWorldspawn();
    if (!worldSpawn)
        return;

    // Set a default blocksize of 1024
    int blockSize = GlobalXYWnd().defaultBlockSize();

    // Check the worldspawn for a custom blocksize
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

    glColor3dv(GlobalColourSchemeManager().getColour("grid_block"));
    glLineWidth (2);

    glBegin (GL_LINES);

    for (x=xb ; x<=xe ; x+=blockSize) {
        glVertex2f (x, yb);
        glVertex2f (x, ye);
    }

    if (_viewType == XY) {
        for (y=yb ; y<=ye ; y+=blockSize) {
            glVertex2f (xb, y);
            glVertex2f (xe, y);
        }
    }

    glEnd();
    glLineWidth (1);

    // draw coordinate text if needed

    if (_viewType == XY && _scale > .1) {
        for (x=xb ; x<xe ; x+=blockSize)
            for (y=yb ; y<ye ; y+=blockSize) {
                glRasterPos2f (x+(blockSize/2), y+(blockSize/2));
                sprintf (text, "%i,%i",(int)floor(x/blockSize), (int)floor(y/blockSize) );
                _font->drawString(text);
            }
    }

    glColor4f(0, 0, 0, 0);
}

void XYWnd::drawCameraIcon()
{
    try
    {
        auto& camera = GlobalCameraManager().getActiveView();

        const auto& origin = camera.getCameraOrigin();
        const auto& angles = camera.getCameraAngles();

        float x, y, fov, box;
        float a;

        fov = 48 / _scale;
        box = 16 / _scale;

        if (_viewType == XY) {
            x = origin[0];
            y = origin[1];
            a = degrees_to_radians(angles[camera::CAMERA_YAW]);
        }
        else if (_viewType == YZ) {
            x = origin[1];
            y = origin[2];
            a = degrees_to_radians(angles[camera::CAMERA_PITCH]);
        }
        else {
            x = origin[0];
            y = origin[2];
            a = degrees_to_radians(angles[camera::CAMERA_PITCH]);
        }

        glColor3dv(GlobalColourSchemeManager().getColour("camera_icon"));
        glBegin(GL_LINE_STRIP);
        glVertex3f(x - box, y, 0);
        glVertex3f(x, y + (box / 2), 0);
        glVertex3f(x + box, y, 0);
        glVertex3f(x, y - (box / 2), 0);
        glVertex3f(x - box, y, 0);
        glVertex3f(x + box, y, 0);
        glEnd();

        glBegin(GL_LINE_STRIP);
        glVertex3f(x + static_cast<float>(fov * cos(a + math::PI / 4)), y + static_cast<float>(fov * sin(a + math::PI / 4)), 0);
        glVertex3f(x, y, 0);
        glVertex3f(x + static_cast<float>(fov * cos(a - math::PI / 4)), y + static_cast<float>(fov * sin(a - math::PI / 4)), 0);
        glEnd();
    }
    catch (const std::runtime_error&)
    {
        return; // no camera present
    }
}

// can be greatly simplified but per usual i am in a hurry
// which is not an excuse, just a fact
void XYWnd::drawSizeInfo(int nDim1, int nDim2, const Vector3& vMinBounds, const Vector3& vMaxBounds)
{
    if (GlobalMapModule().getEditMode() == IMap::EditMode::Merge)
    {
        return;
    }

  if (vMinBounds == vMaxBounds) {
    return;
  }
  const char* g_pDimStrings[] = {"x:", "y:", "z:"};
  typedef const char* OrgStrings[2];
  const OrgStrings g_pOrgStrings[] = { { "x:", "y:", }, { "x:", "z:", }, { "y:", "z:", } };

  Vector3 vSize(vMaxBounds - vMinBounds);

  glColor3dv(GlobalColourSchemeManager().getColour("brush_size_info"));

  std::ostringstream dimensions;

  if (_viewType == XY)
  {
    glBegin (GL_LINES);

    glVertex3d(vMinBounds[nDim1], vMinBounds[nDim2] - 6.0f  / _scale, 0.0f);
    glVertex3d(vMinBounds[nDim1], vMinBounds[nDim2] - 10.0f / _scale, 0.0f);

    glVertex3d(vMinBounds[nDim1], vMinBounds[nDim2] - 10.0f  / _scale, 0.0f);
    glVertex3d(vMaxBounds[nDim1], vMinBounds[nDim2] - 10.0f  / _scale, 0.0f);

    glVertex3d(vMaxBounds[nDim1], vMinBounds[nDim2] - 6.0f  / _scale, 0.0f);
    glVertex3d(vMaxBounds[nDim1], vMinBounds[nDim2] - 10.0f / _scale, 0.0f);


    glVertex3d(vMaxBounds[nDim1] + 6.0f  / _scale, vMinBounds[nDim2], 0.0f);
    glVertex3d(vMaxBounds[nDim1] + 10.0f  / _scale, vMinBounds[nDim2], 0.0f);

    glVertex3d(vMaxBounds[nDim1] + 10.0f  / _scale, vMinBounds[nDim2], 0.0f);
    glVertex3d(vMaxBounds[nDim1] + 10.0f  / _scale, vMaxBounds[nDim2], 0.0f);

    glVertex3d(vMaxBounds[nDim1] + 6.0f  / _scale, vMaxBounds[nDim2], 0.0f);
    glVertex3d(vMaxBounds[nDim1] + 10.0f  / _scale, vMaxBounds[nDim2], 0.0f);

    glEnd();

    glRasterPos3f (Betwixt(vMinBounds[nDim1], vMaxBounds[nDim1]),  vMinBounds[nDim2] - 20.0f  / _scale, 0.0f);
    dimensions << g_pDimStrings[nDim1] << vSize[nDim1];
    _font->drawString(dimensions.str());
    dimensions.str("");

    glRasterPos3f (vMaxBounds[nDim1] + 16.0f  / _scale, Betwixt(vMinBounds[nDim2], vMaxBounds[nDim2]), 0.0f);
    dimensions << g_pDimStrings[nDim2] << vSize[nDim2];
    _font->drawString(dimensions.str());
    dimensions.str("");

    glRasterPos3f (vMinBounds[nDim1] + 4, vMaxBounds[nDim2] + 8 / _scale, 0.0f);
    dimensions << "(" << g_pOrgStrings[0][0] << vMinBounds[nDim1] << "  " << g_pOrgStrings[0][1] << vMaxBounds[nDim2] << ")";
    _font->drawString(dimensions.str());
  }
  else if (_viewType == XZ)
  {
    glBegin (GL_LINES);

    glVertex3f(vMinBounds[nDim1], 0, vMinBounds[nDim2] - 6.0f  / _scale);
    glVertex3f(vMinBounds[nDim1], 0, vMinBounds[nDim2] - 10.0f / _scale);

    glVertex3f(vMinBounds[nDim1], 0,vMinBounds[nDim2] - 10.0f  / _scale);
    glVertex3f(vMaxBounds[nDim1], 0,vMinBounds[nDim2] - 10.0f  / _scale);

    glVertex3f(vMaxBounds[nDim1], 0,vMinBounds[nDim2] - 6.0f  / _scale);
    glVertex3f(vMaxBounds[nDim1], 0,vMinBounds[nDim2] - 10.0f / _scale);


    glVertex3f(vMaxBounds[nDim1] + 6.0f  / _scale, 0,vMinBounds[nDim2]);
    glVertex3f(vMaxBounds[nDim1] + 10.0f  / _scale, 0,vMinBounds[nDim2]);

    glVertex3f(vMaxBounds[nDim1] + 10.0f  / _scale, 0,vMinBounds[nDim2]);
    glVertex3f(vMaxBounds[nDim1] + 10.0f  / _scale, 0,vMaxBounds[nDim2]);

    glVertex3f(vMaxBounds[nDim1] + 6.0f  / _scale, 0,vMaxBounds[nDim2]);
    glVertex3f(vMaxBounds[nDim1] + 10.0f  / _scale, 0,vMaxBounds[nDim2]);

    glEnd();

    glRasterPos3f (Betwixt(vMinBounds[nDim1], vMaxBounds[nDim1]), 0, vMinBounds[nDim2] - 20.0f  / _scale);
    dimensions << g_pDimStrings[nDim1] << vSize[nDim1];
    _font->drawString(dimensions.str());
    dimensions.str("");

    glRasterPos3f (vMaxBounds[nDim1] + 16.0f  / _scale, 0, Betwixt(vMinBounds[nDim2], vMaxBounds[nDim2]));
    dimensions << g_pDimStrings[nDim2] << vSize[nDim2];
    _font->drawString(dimensions.str());
    dimensions.str("");

    glRasterPos3f (vMinBounds[nDim1] + 4, 0, vMaxBounds[nDim2] + 8 / _scale);
    dimensions << "(" << g_pOrgStrings[1][0] << vMinBounds[nDim1] << "  " << g_pOrgStrings[1][1] << vMaxBounds[nDim2] << ")";
    _font->drawString(dimensions.str());
  }
  else
  {
    glBegin (GL_LINES);

    glVertex3f(0, vMinBounds[nDim1], vMinBounds[nDim2] - 6.0f  / _scale);
    glVertex3f(0, vMinBounds[nDim1], vMinBounds[nDim2] - 10.0f / _scale);

    glVertex3f(0, vMinBounds[nDim1], vMinBounds[nDim2] - 10.0f  / _scale);
    glVertex3f(0, vMaxBounds[nDim1], vMinBounds[nDim2] - 10.0f  / _scale);

    glVertex3f(0, vMaxBounds[nDim1], vMinBounds[nDim2] - 6.0f  / _scale);
    glVertex3f(0, vMaxBounds[nDim1], vMinBounds[nDim2] - 10.0f / _scale);


    glVertex3f(0, vMaxBounds[nDim1] + 6.0f  / _scale, vMinBounds[nDim2]);
    glVertex3f(0, vMaxBounds[nDim1] + 10.0f  / _scale, vMinBounds[nDim2]);

    glVertex3f(0, vMaxBounds[nDim1] + 10.0f  / _scale, vMinBounds[nDim2]);
    glVertex3f(0, vMaxBounds[nDim1] + 10.0f  / _scale, vMaxBounds[nDim2]);

    glVertex3f(0, vMaxBounds[nDim1] + 6.0f  / _scale, vMaxBounds[nDim2]);
    glVertex3f(0, vMaxBounds[nDim1] + 10.0f  / _scale, vMaxBounds[nDim2]);

    glEnd();

    glRasterPos3f (0, Betwixt(vMinBounds[nDim1], vMaxBounds[nDim1]),  vMinBounds[nDim2] - 20.0f  / _scale);
    dimensions << g_pDimStrings[nDim1] << vSize[nDim1];
    _font->drawString(dimensions.str());
    dimensions.str("");

    glRasterPos3f (0, vMaxBounds[nDim1] + 16.0f  / _scale, Betwixt(vMinBounds[nDim2], vMaxBounds[nDim2]));
    dimensions << g_pDimStrings[nDim2] << vSize[nDim2];
    _font->drawString(dimensions.str());
    dimensions.str("");

    glRasterPos3f (0, vMinBounds[nDim1] + 4.0f, vMaxBounds[nDim2] + 8 / _scale);
    dimensions << "(" << g_pOrgStrings[2][0] << vMinBounds[nDim1] << "  " << g_pOrgStrings[2][1] << vMaxBounds[nDim2] << ")";
    _font->drawString(dimensions.str());
  }
}

void XYWnd::updateProjection() {
    _projection[0] = 1.0f / static_cast<float>(_width / 2);
    _projection[5] = 1.0f / static_cast<float>(_height / 2);
    _projection[10] = 1.0f / (_maxWorldCoord * _scale);

    _projection[12] = 0.0f;
    _projection[13] = 0.0f;
    _projection[14] = -1.0f;

    _projection[1] = _projection[2] = _projection[3] =
    _projection[4] = _projection[6] = _projection[7] =
    _projection[8] = _projection[9] = _projection[11] = 0.0f;

    _projection[15] = 1.0f;

    _view.construct(_projection, _modelView, _width, _height);
}

// note: modelview matrix must have a uniform scale, otherwise strange things happen when rendering the rotation manipulator.
void XYWnd::updateModelview() {
    int nDim1 = (_viewType == YZ) ? 1 : 0;
    int nDim2 = (_viewType == XY) ? 1 : 2;

    // translation
    _modelView[12] = -_origin[nDim1] * _scale;
    _modelView[13] = -_origin[nDim2] * _scale;
    _modelView[14] = _maxWorldCoord * _scale;

    // axis base
    switch (_viewType) {
        case XY:
            _modelView[0]  =  _scale;
            _modelView[1]  =  0;
            _modelView[2]  =  0;

            _modelView[4]  =  0;
            _modelView[5]  =  _scale;
            _modelView[6]  =  0;

            _modelView[8]  =  0;
            _modelView[9]  =  0;
            _modelView[10] = -_scale;
            break;
        case XZ:
            _modelView[0]  =  _scale;
            _modelView[1]  =  0;
            _modelView[2]  =  0;

            _modelView[4]  =  0;
            _modelView[5]  =  0;
            _modelView[6]  =  _scale;

            _modelView[8]  =  0;
            _modelView[9]  =  _scale;
            _modelView[10] =  0;
            break;
        case YZ:
            _modelView[0]  =  0;
            _modelView[1]  =  0;
            _modelView[2]  = -_scale;

            _modelView[4]  =  _scale;
            _modelView[5]  =  0;
            _modelView[6]  =  0;

            _modelView[8]  =  0;
            _modelView[9]  =  _scale;
            _modelView[10] =  0;
            break;
    }

    _modelView[3] = _modelView[7] = _modelView[11] = 0;
    _modelView[15] = 1;

    _view.construct(_projection, _modelView, _width, _height);
}

void XYWnd::draw()
{
    ensureFont();

    // clear
    glViewport(0, 0, _width, _height);
    Vector3 colourGridBack = GlobalColourSchemeManager().getColour("grid_background");
    glClearColor (colourGridBack[0], colourGridBack[1], colourGridBack[2], 0);

    glClear(GL_COLOR_BUFFER_BIT);

    // set up viewpoint
    glMatrixMode(GL_PROJECTION);
    glLoadMatrixd(_projection);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glScalef(_scale, _scale, 1);
    int nDim1 = (_viewType == YZ) ? 1 : 0;
    int nDim2 = (_viewType == XY) ? 1 : 2;
    glTranslatef(-_origin[nDim1], -_origin[nDim2], 0);

    // Call the image overlay draw method with the window coordinates
    Vector4 windowCoords = getWindowCoordinates();
    Overlay::Instance().draw(
        windowCoords[0], windowCoords[1], windowCoords[2], windowCoords[3],
        _scale);

    glDisable (GL_LINE_STIPPLE);
    glLineWidth(1);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_NORMAL_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_LIGHTING);
    glDisable(GL_COLOR_MATERIAL);
    glDisable(GL_DEPTH_TEST);

    XYWndManager& xyWndManager = GlobalXYWnd();

    drawGrid();
    if (xyWndManager.showBlocks())
        drawBlockGrid();

    glLoadMatrixd(_modelView);

    unsigned int flagsMask = RENDER_VERTEX_COLOUR | RENDER_BLEND;

    if (!getCameraSettings()->solidSelectionBoxes())
    {
        flagsMask |= RENDER_LINESTIPPLE;
    }

    {
        GlobalRenderSystem().startFrame();

        // Construct the renderer and render the scene
        XYRenderer renderer(flagsMask, _highlightShaders);

        // First pass (scenegraph traversal)
        render::RenderableCollectionWalker::CollectRenderablesInScene(renderer,
                                                                      _view);


		// Render any active mousetools
		for (const ActiveMouseTools::value_type& i : _activeMouseTools)
		{
			i.second->render(GlobalRenderSystem(), renderer, _view);
		}

        // Second pass (GL calls)
        renderer.render(_modelView, _projection, _view);

        GlobalRenderSystem().endFrame();
    }

    glDepthMask(GL_FALSE);

    debug::assertNoGlErrors();

    glLoadMatrixd(_modelView);

    debug::assertNoGlErrors();
    glDisable(GL_LINE_STIPPLE);
    debug::assertNoGlErrors();
    glLineWidth(1);
    debug::assertNoGlErrors();
    if (GLEW_VERSION_1_3) {
        glActiveTexture(GL_TEXTURE0);
        glClientActiveTexture(GL_TEXTURE0);
    }
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    debug::assertNoGlErrors();
    glDisableClientState(GL_NORMAL_ARRAY);
    debug::assertNoGlErrors();
    glDisableClientState(GL_COLOR_ARRAY);
    debug::assertNoGlErrors();
    glDisable(GL_TEXTURE_2D);
    debug::assertNoGlErrors();
    glDisable(GL_LIGHTING);
    debug::assertNoGlErrors();
    glDisable(GL_COLOR_MATERIAL);
    debug::assertNoGlErrors();

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
        Vector3 colour = GlobalColourSchemeManager().getColour("xyview_crosshairs");
        glColor4d(colour[0], colour[1], colour[2], 0.8f);
        glBegin (GL_LINES);
        if (_viewType == XY) {
            glVertex2f(2.0f * _minWorldCoord, _mousePosition[1]);
            glVertex2f(2.0f * _maxWorldCoord, _mousePosition[1]);
            glVertex2f(_mousePosition[0], 2.0f * _minWorldCoord);
            glVertex2f(_mousePosition[0], 2.0f * _maxWorldCoord);
        }
        else if (_viewType == YZ) {
            glVertex3f(_mousePosition[0], 2.0f * _minWorldCoord, _mousePosition[2]);
            glVertex3f(_mousePosition[0], 2.0f * _maxWorldCoord, _mousePosition[2]);
            glVertex3f(_mousePosition[0], _mousePosition[1], 2.0f * _minWorldCoord);
            glVertex3f(_mousePosition[0], _mousePosition[1], 2.0f * _maxWorldCoord);
        }
        else {
            glVertex3f (2.0f * _minWorldCoord, _mousePosition[1], _mousePosition[2]);
            glVertex3f (2.0f * _maxWorldCoord, _mousePosition[1], _mousePosition[2]);
            glVertex3f(_mousePosition[0], _mousePosition[1], 2.0f * _minWorldCoord);
            glVertex3f(_mousePosition[0], _mousePosition[1], 2.0f * _maxWorldCoord);
        }
        glEnd();
    }

    // greebo: Draw the clipper's control points
    {
        glColor3dv(GlobalColourSchemeManager().getColour("clipper"));
        glPointSize(4);

        if (GlobalClipper().clipMode()) {
            GlobalClipper().draw(_scale);
        }

        glPointSize(1);
    }

    debug::assertNoGlErrors();

    // reset modelview
    glLoadIdentity();
    glScalef(_scale, _scale, 1);
    glTranslatef(-_origin[nDim1], -_origin[nDim2], 0);

    drawCameraIcon();

    if (!_activeMouseTools.empty())
    {
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(0, _width, 0, _height, 0, 1);

        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        for (const ActiveMouseTools::value_type& i : _activeMouseTools)
        {
            i.second->renderOverlay();
        }
    }

    if (xyWndManager.showOutline()) {
        if (isActive()) {
            glMatrixMode (GL_PROJECTION);
            glLoadIdentity();
            glOrtho (0, _width, 0, _height, 0, 1);

            glMatrixMode (GL_MODELVIEW);
            glLoadIdentity();

            switch (_viewType) {
                case YZ:
                    glColor3dv(GlobalColourSchemeManager().getColour("axis_x"));
                    break;
                case XZ:
                    glColor3dv(GlobalColourSchemeManager().getColour("axis_y"));
                    break;
                case XY:
                    glColor3dv(GlobalColourSchemeManager().getColour("axis_z"));
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

    debug::assertNoGlErrors();

    // Reset the depth mask to its initial value (enabled)
    glDepthMask(GL_TRUE);
    debug::assertNoGlErrors();
}

void XYWnd::mouseToPoint(int x, int y, Vector3& point)
{
    point = convertXYToWorld(x, y);
    snapToGrid(point);

    int nDim = (getViewType() == XY) ? 2 : (getViewType() == YZ) ? 0 : 1;

    const selection::WorkZone& wz = GlobalSelectionSystem().getWorkZone();

    point[nDim] = float_snapped(wz.bounds.getOrigin()[nDim], GlobalGrid().getGridSize());
}

// NOTE: the zoom out factor is 4/5, we could think about customizing it
//  we don't go below a zoom factor corresponding to 10% of the max world size
//  (this has to be computed against the window size)
float XYWnd::getZoomedScale(int steps)
{
    const float min_scale = std::min(getWidth(), getHeight()) / (1.1f * (_maxWorldCoord - _minWorldCoord));
    const float max_scale = GlobalXYWnd().maxZoomFactor();
    const float fZoom = pow(5.0f / 4.0f, steps);
    const float scale = getScale() * fZoom;

    if (scale > max_scale)
    {
        return max_scale;
    }

    if (scale < min_scale)
    {
        return min_scale;
    }

    return scale;
}

void XYWnd::zoomOut()
{
    setScale( getZoomedScale( -1 ) );
}

void XYWnd::zoomIn()
{
    setScale( getZoomedScale( 1 ) );
}

void XYWnd::zoomInOn(wxPoint cursor, int zoom)
{
    const float newScale = getZoomedScale(zoom);

    int dim1 = _viewType == YZ ? 1 : 0;
    int dim2 = _viewType == XY ? 1 : 2;

    // worldPos = origin + devicePos * device2WorldScale
    // devicePos and worldPos should remain constant. device2WorldScale is known before
    // and after zooming, so the origin delta can be calculated from what we have
    auto scaleAdjustment = (1 / _scale - 1 / newScale);

    _origin[dim1] += screen_normalised(cursor.x, _width) * _width / 2 * scaleAdjustment;
    _origin[dim2] -= screen_normalised(cursor.y, _height) * _height / 2 * scaleAdjustment;

    setScale(newScale);
}

// ================ CALLBACKS ======================================

// This is the chase mouse handler that gets connected by XYWnd::chaseMouseMotion()
// It passes te call on to the XYWnd::chaseMouse() method.
void XYWnd::onIdle(wxIdleEvent& ev)
{
	if (_chasingMouse)
	{
		performChaseMouse();
	}

    if (_updateRequested)
    {
        _updateRequested = false;
        _wxGLWidget->Refresh(false);
    }
}

void XYWnd::onGLResize(wxSizeEvent& ev)
{
	const wxSize clientSize = _wxGLWidget->GetClientSize();

	_width = clientSize.GetWidth();
	_height = clientSize.GetHeight();

	updateProjection();

	ev.Skip();
}

bool XYWnd::onRender()
{
	if (GlobalMainFrame().screenUpdatesEnabled())
	{
        util::ScopedBoolLock drawLock(_drawing);

		draw();

        return true;
	}


    return false; // nothing rendered
}

void XYWnd::onGLWindowScroll(wxMouseEvent& ev)
{
    if (!ev.ShiftDown() && GlobalXYWnd().zoomCenteredOnMouseCursor())
    {
        // The mouse position passed in the scroll event is not accurate on some systems,
        // it fails to send widget-relative coordinates (even got negative numbers, see #5792)
        // Translate the mouse coordinates on our own
        auto clientPos = _wxGLWidget->ScreenToClient(wxGetMousePosition());

        zoomInOn(clientPos, ev.GetWheelRotation() > 0 ? 1 : -1);
        return;
    }

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

	// Context menu handling
    if (ev.RightDown() && !ev.HasAnyModifiers())
    {
        // Remember the RMB coordinates for use in the mouseup event
        Vector2i clickPos{ev.GetX(), ev.GetY()};
        _rightClickPos = { clickPos, clickPos };
    }

    // Send the event to the mouse tool handler
    MouseToolHandler::onGLMouseButtonPress(ev);

	queueDraw();
}

void XYWnd::onGLMouseButtonRelease(wxMouseEvent& ev)
{
	// Do the context menu handling first
    if (ev.RightUp() && !ev.HasAnyModifiers() && _rightClickPos)
    {
        // The user just pressed and released the RMB in the same place
        onContextMenu();
	}

    // Regular mouse tool processing
    MouseToolHandler::onGLMouseButtonRelease(ev);

	queueDraw();
}

void XYWnd::onGLMouseMove(wxMouseEvent& ev)
{
    MouseToolHandler::onGLMouseMove(ev);

    handleGLMouseMotion(ev.GetX(), ev.GetY(), wxutil::MouseButton::GetStateForMouseEvent(ev), false);
}

MouseTool::Result XYWnd::processMouseDownEvent(const MouseToolPtr& tool, const Vector2& point)
{
    XYMouseToolEvent ev = createMouseEvent(point);
    return tool->onMouseDown(ev);
}

MouseTool::Result XYWnd::processMouseUpEvent(const MouseToolPtr& tool, const Vector2& point)
{
    XYMouseToolEvent ev = createMouseEvent(point);
    return tool->onMouseUp(ev);
}

MouseTool::Result XYWnd::processMouseMoveEvent(const MouseToolPtr& tool, int x, int y)
{
    bool mouseToolReceivesDeltas = (tool->getPointerMode() & MouseTool::PointerMode::MotionDeltas) != 0;

    // New MouseTool event, optionally passing the delta only
    XYMouseToolEvent ev = mouseToolReceivesDeltas ?
        createMouseEvent(Vector2(0, 0), Vector2(x, y)) :
        createMouseEvent(Vector2(x, y));

    return tool->onMouseMove(ev);
}

void XYWnd::startCapture(const MouseToolPtr& tool)
{
    if (_freezePointer.isCapturing(_wxGLWidget))
    {
        return;
    }

    unsigned int pointerMode = tool->getPointerMode();

     _freezePointer.startCapture(_wxGLWidget,
        [&, tool](int x, int y, unsigned int state) { handleGLCapturedMouseMotion(tool, x, y, state); }, // Motion Functor
        [&, tool]() { MouseToolHandler::handleCaptureLost(tool); }, // called when the capture is lost.
        (pointerMode & MouseTool::PointerMode::Freeze) != 0,
        (pointerMode & MouseTool::PointerMode::Hidden) != 0,
        (pointerMode & MouseTool::PointerMode::MotionDeltas) != 0
    );
}

void XYWnd::endCapture()
{
    if (!_freezePointer.isCapturing(_wxGLWidget))
    {
        return;
    }

    _freezePointer.endCapture();
}

IInteractiveView& XYWnd::getInteractiveView()
{
    return *this;
}

/* STATICS */
XYRenderer::HighlightShaders XYWnd::_highlightShaders;

} // namespace
