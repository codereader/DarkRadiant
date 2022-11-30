#include "GlobalXYWnd.h"

#include "i18n.h"
#include "ui/ieventmanager.h"
#include "ui/istatusbarmanager.h"
#include "ui/imainframe.h"
#include "ipreferencesystem.h"
#include "ui/iuserinterface.h"

#include "registry/registry.h"

#include "module/StaticModule.h"
#include "camera/CameraWndManager.h"
#include "wxutil/MouseButton.h"

#include "tools/BrushCreatorTool.h"
#include "tools/ClipperTool.h"
#include "tools/ZoomTool.h"
#include "tools/CameraAngleTool.h"
#include "tools/CameraMoveTool.h"
#include "tools/MoveViewTool.h"
#include "tools/MeasurementTool.h"
#include "debugging/ScopedDebugTimer.h"

#include <functional>

namespace ui
{

namespace
{
	const std::string RKEY_XYVIEW_ROOT = "user/ui/xyview";

	const std::string RKEY_CHASE_MOUSE = RKEY_XYVIEW_ROOT + "/chaseMouse";
    const std::string RKEY_CHASE_MOUSE_CAP = RKEY_XYVIEW_ROOT + "/chaseMouseCap";
	const std::string RKEY_CAMERA_XY_UPDATE = RKEY_XYVIEW_ROOT + "/camXYUpdate";
	const std::string RKEY_SHOW_CROSSHAIRS = RKEY_XYVIEW_ROOT + "/showCrossHairs";
	const std::string RKEY_SHOW_GRID = RKEY_XYVIEW_ROOT + "/showGrid";
	const std::string RKEY_SHOW_SIZE_INFO = RKEY_XYVIEW_ROOT + "/showSizeInfo";
	const std::string RKEY_SHOW_ENTITY_ANGLES = RKEY_XYVIEW_ROOT + "/showEntityAngles";
	const std::string RKEY_SHOW_ENTITY_NAMES = RKEY_XYVIEW_ROOT + "/showEntityNames";
	const std::string RKEY_SHOW_BLOCKS = RKEY_XYVIEW_ROOT + "/showBlocks";
	const std::string RKEY_SHOW_COORDINATES = RKEY_XYVIEW_ROOT + "/showCoordinates";
	const std::string RKEY_SHOW_OUTLINE = RKEY_XYVIEW_ROOT + "/showOutline";
	const std::string RKEY_SHOW_AXES = RKEY_XYVIEW_ROOT + "/showAxes";
	const std::string RKEY_SHOW_WORKZONE = RKEY_XYVIEW_ROOT + "/showWorkzone";
	const std::string RKEY_DEFAULT_BLOCKSIZE = "user/ui/xyview/defaultBlockSize";
	const std::string RKEY_TRANSLATE_CONSTRAINED = "user/ui/xyview/translateConstrained";
	const std::string RKEY_FONT_SIZE = "user/ui/xyview/fontSize";
	const std::string RKEY_FONT_STYLE = "user/ui/xyview/fontStyle";
	const std::string RKEY_MAX_ZOOM_FACTOR = "user/ui/xyview/maxZoomFactor";
	const std::string RKEY_CURSOR_CENTERED_ZOOM = "user/ui/xyview/cursorCenteredZoom";

    const int DEFAULT_CHASE_MOUSE_CAP = 32; // pixels per chase moue timer interval
}

class OrthoviewControl :
    public IUserControl
{
private:
    XYWndManager& _owner;

public:
    OrthoviewControl(XYWndManager& owner) :
        _owner(owner)
    {}

    std::string getControlName() override
    {
        return UserControl::OrthoView;
    }

    std::string getDisplayName() override
    {
        return _("2D View");
    }

    wxWindow* createWidget(wxWindow* parent) override
    {
        return new XYWnd(parent, _owner);
    }
};

XYWndManager::XYWndManager() :
    _maxZoomFactor(1024),
    _activeXYWndId(-1)
{}

void XYWndManager::registerXYWnd(XYWnd* view)
{
    if (!_xyWnds.emplace(view->getId(), view).second)
    {
        throw std::invalid_argument("XY Wnd ID has already been registered");
    }

    // Activate this view if this is the first one
    if (_activeXYWndId == -1)
    {
        setActiveXY(view->getId());
    }
}

void XYWndManager::unregisterXYWnd(XYWnd* view)
{
    auto id = view->getId();
    _xyWnds.erase(id);

    if (_activeXYWndId == id)
    {
        setActiveXY(_xyWnds.empty() ? -1 : _xyWnds.begin()->first);
    }
}

void XYWndManager::registerCommands()
{
    GlobalMainFrame().signal_MainFrameConstructed().connect([&]()
    {
        GlobalMainFrame().addControl(UserControl::OrthoView, { IMainFrame::Location::FloatingWindow, false });
    });

    GlobalCommandSystem().addStatement("NewOrthoView", fmt::format("{0} {1}", CREATE_CONTROL_COMMAND, UserControl::OrthoView), false);
	GlobalCommandSystem().addCommand("NextView", std::bind(&XYWndManager::toggleActiveView, this, std::placeholders::_1));
	GlobalCommandSystem().addCommand("ZoomIn", std::bind(&XYWndManager::zoomIn, this, std::placeholders::_1));
	GlobalCommandSystem().addCommand("ZoomOut", std::bind(&XYWndManager::zoomOut, this, std::placeholders::_1));
	GlobalCommandSystem().addCommand("ViewTop", std::bind(&XYWndManager::setActiveViewXY, this, std::placeholders::_1));
	GlobalCommandSystem().addCommand("ViewSide", std::bind(&XYWndManager::setActiveViewXZ, this, std::placeholders::_1));
	GlobalCommandSystem().addCommand("ViewFront", std::bind(&XYWndManager::setActiveViewYZ, this, std::placeholders::_1));
	GlobalCommandSystem().addCommand("CenterXYViews", std::bind(&XYWndManager::splitViewFocus, this, std::placeholders::_1));
	GlobalCommandSystem().addCommand("CenterXYView", std::bind(&XYWndManager::focusActiveView, this, std::placeholders::_1));
	GlobalCommandSystem().addCommand("Zoom100", std::bind(&XYWndManager::zoom100, this, std::placeholders::_1));
	GlobalCommandSystem().addCommand("RunBenchmark", std::bind(&XYWndManager::runBenchmark, this, std::placeholders::_1), 
        { cmd::ARGTYPE_INT | cmd::ARGTYPE_OPTIONAL });

	GlobalEventManager().addRegistryToggle("ToggleCrosshairs", RKEY_SHOW_CROSSHAIRS);
	GlobalEventManager().addRegistryToggle("ToggleGrid", RKEY_SHOW_GRID);
	GlobalEventManager().addRegistryToggle("ShowAngles", RKEY_SHOW_ENTITY_ANGLES);
	GlobalEventManager().addRegistryToggle("ShowNames", RKEY_SHOW_ENTITY_NAMES);
	GlobalEventManager().addRegistryToggle("ShowBlocks", RKEY_SHOW_BLOCKS);
	GlobalEventManager().addRegistryToggle("ShowCoordinates", RKEY_SHOW_COORDINATES);
	GlobalEventManager().addRegistryToggle("ShowWindowOutline", RKEY_SHOW_OUTLINE);
	GlobalEventManager().addRegistryToggle("ShowAxes", RKEY_SHOW_AXES);
	GlobalEventManager().addRegistryToggle("ShowWorkzone", RKEY_SHOW_WORKZONE);
	GlobalEventManager().addRegistryToggle("ToggleShowSizeInfo", RKEY_SHOW_SIZE_INFO);
}

void XYWndManager::constructPreferences()
{
	IPreferencePage& page = GlobalPreferenceSystem().getPage(_("Settings/Orthoview"));

	page.appendCheckBox(_("View chases Mouse Cursor during Drags"), RKEY_CHASE_MOUSE);
    page.appendSlider(_("Maximum Chase Mouse Speed"), RKEY_CHASE_MOUSE_CAP, 0, 512, 1, 16);
	page.appendCheckBox(_("Update Views on Camera Movement"), RKEY_CAMERA_XY_UPDATE);
	page.appendCheckBox(_("Show Crosshairs"), RKEY_SHOW_CROSSHAIRS);
	page.appendCheckBox(_("Show Grid"), RKEY_SHOW_GRID);
	page.appendCheckBox(_("Show Size Info"), RKEY_SHOW_SIZE_INFO);
	page.appendCheckBox(_("Show Entity Angle Arrow"), RKEY_SHOW_ENTITY_ANGLES);
	page.appendCheckBox(_("Show Entity Names"), RKEY_SHOW_ENTITY_NAMES);
	page.appendCheckBox(_("Show Blocks"), RKEY_SHOW_BLOCKS);
	page.appendCheckBox(_("Show Coordinates"), RKEY_SHOW_COORDINATES);
	page.appendCheckBox(_("Show Axes"), RKEY_SHOW_AXES);
	page.appendCheckBox(_("Show Window Outline"), RKEY_SHOW_OUTLINE);
	page.appendCheckBox(_("Show Workzone"), RKEY_SHOW_WORKZONE);
	page.appendCheckBox(_("Translate Manipulator always constrained to Axis"), RKEY_TRANSLATE_CONSTRAINED);
	page.appendCheckBox(_("Higher Selection Priority for Entities"), RKEY_HIGHER_ENTITY_PRIORITY);
    page.appendSpinner(_("Maximum Zoom Factor"), RKEY_MAX_ZOOM_FACTOR, 1, 65536, 0);
	page.appendCheckBox(_("Zoom centers on Mouse Cursor"), RKEY_CURSOR_CENTERED_ZOOM);
    page.appendCombo(_("Font Style"), RKEY_FONT_STYLE, { "Sans", "Mono" }, true);
    page.appendSpinner(_("Font Size"), RKEY_FONT_SIZE, 4, 48, 0);
}

// Load/Reload the values from the registry
void XYWndManager::refreshFromRegistry()
{
	_chaseMouse = registry::getValue<bool>(RKEY_CHASE_MOUSE);
    _chaseMouseCap = registry::getValue<int>(RKEY_CHASE_MOUSE_CAP, DEFAULT_CHASE_MOUSE_CAP);
	_camXYUpdate = registry::getValue<bool>(RKEY_CAMERA_XY_UPDATE);
	_showCrossHairs = registry::getValue<bool>(RKEY_SHOW_CROSSHAIRS);
	_showGrid = registry::getValue<bool>(RKEY_SHOW_GRID);
	_showSizeInfo = registry::getValue<bool>(RKEY_SHOW_SIZE_INFO);
	_showBlocks = registry::getValue<bool>(RKEY_SHOW_BLOCKS);
	_showCoordinates = registry::getValue<bool>(RKEY_SHOW_COORDINATES);
	_showOutline = registry::getValue<bool>(RKEY_SHOW_OUTLINE);
	_showAxes = registry::getValue<bool>(RKEY_SHOW_AXES);
	_showWorkzone = registry::getValue<bool>(RKEY_SHOW_WORKZONE);
	_defaultBlockSize = registry::getValue<int>(RKEY_DEFAULT_BLOCKSIZE);
    _fontSize = registry::getValue<int>(RKEY_FONT_SIZE);
    _fontStyle = registry::getValue<std::string>(RKEY_FONT_STYLE) == "Sans" ? IGLFont::Style::Sans : IGLFont::Style::Mono;
    _maxZoomFactor = registry::getValue<float>(RKEY_MAX_ZOOM_FACTOR);
    _zoomCenteredOnMouseCursor = registry::getValue<bool>(RKEY_CURSOR_CENTERED_ZOOM);

	updateAllViews();

    for (const auto& xyWnd : _xyWnds)
    {
        xyWnd.second->updateFont();
    }
}

bool XYWndManager::chaseMouse() const {
	return _chaseMouse;
}

int XYWndManager::chaseMouseCap() const
{
    return _chaseMouseCap;
}

bool XYWndManager::camXYUpdate() const {
	return _camXYUpdate;
}

bool XYWndManager::showCrossHairs() const {
	return _showCrossHairs;
}

bool XYWndManager::showBlocks() const {
	return _showBlocks;
}

unsigned int XYWndManager::defaultBlockSize() const {
	return _defaultBlockSize;
}

bool XYWndManager::showCoordinates() const {
	return _showCoordinates;
}

bool XYWndManager::showOutline() const  {
	return _showOutline;
}

bool XYWndManager::showAxes() const {
	return _showAxes;
}

bool XYWndManager::showWorkzone() const {
	return _showWorkzone;
}

bool XYWndManager::showGrid() const {
	return _showGrid;
}

bool XYWndManager::showSizeInfo() const {
	return _showSizeInfo;
}

int XYWndManager::fontSize() const
{
    return _fontSize;
}

IGLFont::Style XYWndManager::fontStyle() const
{
    return _fontStyle;
}

float XYWndManager::maxZoomFactor() const
{
    return _maxZoomFactor;
}

bool XYWndManager::zoomCenteredOnMouseCursor() const
{
    return _zoomCenteredOnMouseCursor;
}

void XYWndManager::updateAllViews(bool force)
{
	for (const auto& i : _xyWnds)
	{
        if (force)
        {
            i.second->forceRedraw();
        }
        else
        {
            i.second->queueDraw();
        }
	}
}

void XYWndManager::doWithActiveXyWnd(const std::function<void(XYWnd&)>& action) const
{
    auto window = _xyWnds.find(_activeXYWndId);

    if (window != _xyWnds.end())
    {
        action(*window->second);
    }
}

void XYWndManager::zoomIn(const cmd::ArgumentList& args)
{
    doWithActiveXyWnd([](auto& activeXyWnd) { activeXyWnd.zoomIn(); });
}

void XYWndManager::zoomOut(const cmd::ArgumentList& args)
{
    doWithActiveXyWnd([](auto& activeXyWnd) { activeXyWnd.zoomOut(); });
}

void XYWndManager::setOrigin(const Vector3& origin)
{
	// Cycle through the list of views and set the origin
	for (auto& [_, xyWnd] : _xyWnds)
	{
		xyWnd->setOrigin(origin);
	}
}

Vector3 XYWndManager::getActiveViewOrigin()
{
    Vector3 result;

    doWithActiveXyWnd([&](auto& activeXyWnd) { result = activeXyWnd.getOrigin(); });

    return result;
}

IOrthoView& XYWndManager::getActiveView()
{
    auto window = _xyWnds.find(_activeXYWndId);

    if (window == _xyWnds.end())
    {
		throw std::runtime_error("No active view found");
    }

	return *window->second;
}

// Return the first view matching the given viewType
IOrthoView& XYWndManager::getViewByType(EViewType viewType)
{
	for (auto& pair : _xyWnds)
	{
		if (pair.second->getViewType() == viewType)
		{
			return *pair.second;
		}
	}

	throw std::runtime_error("No matching view found");
}

void XYWndManager::setScale(float scale)
{
    for (auto& [_, xyWnd] : _xyWnds)
	{
		xyWnd->setScale(scale);
	}
}

void XYWndManager::positionAllViews(const Vector3& origin)
{
    for (auto& [_, xyWnd] : _xyWnds)
	{
		xyWnd->positionView(origin);
	}
}

void XYWndManager::positionActiveView(const Vector3& origin)
{
    doWithActiveXyWnd([&](auto& activeXyWnd) { activeXyWnd.positionView(origin); });
}

EViewType XYWndManager::getActiveViewType() const
{
    auto viewType = XY;

    doWithActiveXyWnd([&](auto& activeXyWnd) { viewType = activeXyWnd.getViewType(); });

    return viewType;
}

void XYWndManager::setActiveViewType(EViewType viewType)
{
    doWithActiveXyWnd([&](auto& activeXyWnd) { activeXyWnd.setViewType(viewType); });
}

void XYWndManager::toggleActiveView(const cmd::ArgumentList& args)
{
    doWithActiveXyWnd([&](auto& activeXyWnd)
    {
        if (activeXyWnd.getViewType() == XY)
        {
            activeXyWnd.setViewType(XZ);
        }
        else if (activeXyWnd.getViewType() == XZ)
        {
            activeXyWnd.setViewType(YZ);
        }
        else
        {
            activeXyWnd.setViewType(XY);
        }

        // Re-focus the view when toggling projections
        activeXyWnd.setOrigin(getFocusPosition());
    });
}

void XYWndManager::setActiveViewXY(const cmd::ArgumentList& args)
{
	setActiveViewType(XY);
	positionActiveView(getFocusPosition());
}

void XYWndManager::setActiveViewXZ(const cmd::ArgumentList& args)
{
	setActiveViewType(XZ);
	positionActiveView(getFocusPosition());
}

void XYWndManager::setActiveViewYZ(const cmd::ArgumentList& args)
{
	setActiveViewType(YZ);
	positionActiveView(getFocusPosition());
}

void XYWndManager::splitViewFocus(const cmd::ArgumentList& args)
{
	positionAllViews(getFocusPosition());
}

void XYWndManager::zoom100(const cmd::ArgumentList& args)
{
	setScale(1);
}

void XYWndManager::focusActiveView(const cmd::ArgumentList& args)
{
	positionActiveView(getFocusPosition());
}

void XYWndManager::setActiveXY(int id)
{
    // Notify the currently active XYView that is has been deactivated
    doWithActiveXyWnd([](auto& activeXyWnd) { activeXyWnd.setActive(false); });

    // Find the ID in the map and update the active pointer
    auto view = _xyWnds.find(id);

    if (view != _xyWnds.end())
    {
        _activeXYWndId = view->first;
        view->second->setActive(true);
    }
    else
    {
        _activeXYWndId = -1;
    }
}

int XYWndManager::getUniqueID() const
{
	for (int i = 0; i < INT_MAX; ++i)
	{
		if (_xyWnds.count(i) == 0)
			return i;
	}

	throw std::runtime_error(
		"Cannot create unique ID for ortho view: no more IDs."
	);
}

/* greebo: This function determines the point currently being "looked" at, it is used for toggling the ortho views
 * If something is selected the center of the selection is taken as new origin, otherwise the camera
 * position is considered to be the new origin of the toggled orthoview.
*/
Vector3 XYWndManager::getFocusPosition()
{
    Vector3 position(0,0,0);

    if (GlobalSelectionSystem().countSelected() != 0)
    {
        position = GlobalSelectionSystem().getCurrentSelectionCenter();
    }
    else if (GlobalSelectionSystem().selectionFocusIsActive())
    {
        // Use the selection focus area if nothing is selected
        position = GlobalSelectionSystem().getSelectionFocusBounds().getOrigin();
    }
    else
    {
        auto cam = GlobalCamera().getActiveCamWnd();

        if (cam != NULL) {
            position = cam->getCameraOrigin();
        }
    }

    return position;
}

const std::string& XYWndManager::getName() const
{
	static std::string _name(MODULE_ORTHOVIEWMANAGER);
	return _name;
}

const StringSet& XYWndManager::getDependencies() const
{
    static StringSet _dependencies
    {
        MODULE_XMLREGISTRY,
        MODULE_EVENTMANAGER,
        MODULE_RENDERSYSTEM,
        MODULE_PREFERENCESYSTEM,
        MODULE_COMMANDSYSTEM,
        MODULE_MOUSETOOLMANAGER,
        MODULE_STATUSBARMANAGER,
        MODULE_USERINTERFACE,
        MODULE_MAINFRAME,
    };

	return _dependencies;
}

void XYWndManager::observeKey(const std::string& key)
{
    GlobalRegistry().signalForKey(key).connect(
        sigc::mem_fun(this, &XYWndManager::refreshFromRegistry)
    );
}

void XYWndManager::initialiseModule(const IApplicationContext& ctx)
{
	// Connect self to the according registry keys
	observeKey(RKEY_CHASE_MOUSE);
    observeKey(RKEY_CHASE_MOUSE_CAP);
	observeKey(RKEY_CAMERA_XY_UPDATE);
	observeKey(RKEY_SHOW_CROSSHAIRS);
	observeKey(RKEY_SHOW_GRID);
	observeKey(RKEY_SHOW_SIZE_INFO);
	observeKey(RKEY_SHOW_ENTITY_ANGLES);
	observeKey(RKEY_SHOW_ENTITY_NAMES);
	observeKey(RKEY_SHOW_BLOCKS);
	observeKey(RKEY_SHOW_COORDINATES);
	observeKey(RKEY_SHOW_OUTLINE);
	observeKey(RKEY_SHOW_AXES);
	observeKey(RKEY_SHOW_WORKZONE);
	observeKey(RKEY_DEFAULT_BLOCKSIZE);
	observeKey(RKEY_FONT_SIZE);
	observeKey(RKEY_FONT_STYLE);
	observeKey(RKEY_MAX_ZOOM_FACTOR);
	observeKey(RKEY_CURSOR_CENTERED_ZOOM);

	// Trigger loading the values of the observed registry keys
	refreshFromRegistry();

	// Construct the preference settings widgets
	constructPreferences();

	// Add the commands to the EventManager
	registerCommands();

	GlobalStatusBarManager().addTextElement(
		"XYZPos",
		"",  // no icon
		statusbar::StandardPosition::OrthoViewPosition,
		_("Shows the mouse position in the orthoview")
	);

	XYWnd::captureStates();

    // Add default XY tools
    IMouseToolGroup& toolGroup = GlobalMouseToolManager().getGroup(IMouseToolGroup::Type::OrthoView);

    toolGroup.registerMouseTool(std::make_shared<BrushCreatorTool>());
    toolGroup.registerMouseTool(std::make_shared<ClipperTool>());
    toolGroup.registerMouseTool(std::make_shared<ZoomTool>());
    toolGroup.registerMouseTool(std::make_shared<CameraAngleTool>());
    toolGroup.registerMouseTool(std::make_shared<CameraMoveTool>());
    toolGroup.registerMouseTool(std::make_shared<MoveViewTool>());
	toolGroup.registerMouseTool(std::make_shared<MeasurementTool>());

    GlobalUserInterface().registerControl(std::make_shared<OrthoviewControl>(*this));
}

void XYWndManager::shutdownModule()
{
    GlobalUserInterface().unregisterControl(UserControl::OrthoView);

	// Clear all tracked references
    _xyWnds.clear();

	XYWnd::releaseStates();
}

MouseToolStack XYWndManager::getMouseToolsForEvent(wxMouseEvent& ev)
{
    unsigned int state = wxutil::MouseButton::GetButtonStateChangeForMouseEvent(ev);
    return GlobalMouseToolManager().getMouseToolsForEvent(IMouseToolGroup::Type::OrthoView, state);
}

void XYWndManager::foreachMouseTool(const std::function<void(const MouseToolPtr&)>& func)
{
    GlobalMouseToolManager().getGroup(IMouseToolGroup::Type::OrthoView).foreachMouseTool(func);
}

void XYWndManager::runBenchmark(const cmd::ArgumentList& args)
{
    auto cam = GlobalCamera().getActiveCamWnd();

    cam->getCamera().setCameraOrigin({ 2517, -4511, 713 });
    cam->getCamera().setCameraAngles({ -2.1, 123.91, 0 });

    int numRuns = args.empty() ? 1 : args[0].getInt();

    ScopedDebugTimer timer("Camera refresh, " + string::to_string(numRuns) + " runs");
    for (int i = 0; i < numRuns; ++i)
    {
        cam->getCamera().forceRedraw();
    }
}

// Define the static GlobalXYWnd module
module::StaticModuleRegistration<XYWndManager> xyWndModule;

} // namespace

// Accessor function returning the reference
ui::XYWndManager& GlobalXYWnd()
{
	return *std::static_pointer_cast<ui::XYWndManager>(
		module::GlobalModuleRegistry().getModule(MODULE_ORTHOVIEWMANAGER));
}
