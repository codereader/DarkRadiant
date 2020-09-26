#include "GlobalCameraWndManager.h"

#include "imousetoolmanager.h"
#include "ieventmanager.h"
#include "iselection.h"
#include "iuimanager.h"
#include "itextstream.h"
#include "xmlutil/Node.h"

#include "Camera.h"
#include "CameraSettings.h"

#include "registry/registry.h"
#include "module/StaticModule.h"
#include "wxutil/MouseButton.h"
#include "string/case_conv.h"

#include "tools/ShaderClipboardTools.h"
#include "tools/JumpToObjectTool.h"
#include "tools/FreeMoveTool.h"
#include "tools/PanViewTool.h"

#include "FloatingCamWnd.h"
#include <functional>

namespace ui
{

namespace
{
    const float DEFAULT_STRAFE_SPEED = 0.65f;
    const float DEFAULT_FORWARD_STRAFE_FACTOR = 1.0f;
}

// Constructor
GlobalCameraWndManager::GlobalCameraWndManager() :
	_activeCam(-1),
    _toggleStrafeModifierFlags(wxutil::Modifier::NONE),
    _toggleStrafeForwardModifierFlags(wxutil::Modifier::NONE),
    _strafeSpeed(DEFAULT_STRAFE_SPEED),
    _forwardStrafeFactor(DEFAULT_FORWARD_STRAFE_FACTOR)
{}

void GlobalCameraWndManager::registerCommands()
{
	GlobalCommandSystem().addCommand("CenterView", std::bind(&GlobalCameraWndManager::resetCameraAngles, this, std::placeholders::_1));
	GlobalCommandSystem().addCommand("CubicClipZoomIn", std::bind(&GlobalCameraWndManager::farClipPlaneIn, this, std::placeholders::_1));
	GlobalCommandSystem().addCommand("CubicClipZoomOut", std::bind(&GlobalCameraWndManager::farClipPlaneOut, this, std::placeholders::_1));

	GlobalCommandSystem().addCommand("UpFloor", std::bind(&GlobalCameraWndManager::changeFloorUp, this, std::placeholders::_1));
	GlobalCommandSystem().addCommand("DownFloor", std::bind(&GlobalCameraWndManager::changeFloorDown, this, std::placeholders::_1));

	// angua: increases and decreases the movement speed of the camera
	GlobalCommandSystem().addCommand("CamIncreaseMoveSpeed", std::bind(&GlobalCameraWndManager::increaseCameraSpeed, this, std::placeholders::_1));
	GlobalCommandSystem().addCommand("CamDecreaseMoveSpeed", std::bind(&GlobalCameraWndManager::decreaseCameraSpeed, this, std::placeholders::_1));

	GlobalCommandSystem().addCommand("TogglePreview", std::bind(&GlobalCameraWndManager::toggleLightingMode, this, std::placeholders::_1));

	GlobalCommandSystem().addCommand("MoveCamera", std::bind(&GlobalCameraWndManager::moveCameraCmd, this, std::placeholders::_1),
		{ cmd::ARGTYPE_STRING, cmd::ARGTYPE_DOUBLE });

	// Insert movement commands
	GlobalCommandSystem().addCommand("CameraStrafeRight", std::bind(&GlobalCameraWndManager::moveRightDiscrete, this, std::placeholders::_1));
	GlobalCommandSystem().addCommand("CameraStrafeLeft", std::bind(&GlobalCameraWndManager::moveLeftDiscrete, this, std::placeholders::_1));

	GlobalCommandSystem().addCommand("CameraAngleUp", std::bind(&GlobalCameraWndManager::pitchUpDiscrete, this, std::placeholders::_1));
	GlobalCommandSystem().addCommand("CameraAngleDown", std::bind(&GlobalCameraWndManager::pitchDownDiscrete, this, std::placeholders::_1));

	// Bind the events to the commands
	GlobalEventManager().addToggle(
        "ToggleCubicClip",
        std::bind(&CameraSettings::toggleFarClip, getCameraSettings(), std::placeholders::_1)
    );
	// Set the default status of the cubic clip
	GlobalEventManager().setToggled("ToggleCubicClip", getCameraSettings()->farClipEnabled());

	GlobalEventManager().addWidgetToggle("ToggleCamera");
	GlobalEventManager().setToggled("ToggleCamera", true);

	GlobalEventManager().addKeyEvent("CameraMoveForward", std::bind(&GlobalCameraWndManager::onFreelookMoveForwardKey, this, std::placeholders::_1));
	GlobalEventManager().addKeyEvent("CameraMoveBack", std::bind(&GlobalCameraWndManager::onFreelookMoveBackKey, this, std::placeholders::_1));
	GlobalEventManager().addKeyEvent("CameraMoveLeft", std::bind(&GlobalCameraWndManager::onFreelookMoveLeftKey, this, std::placeholders::_1));
	GlobalEventManager().addKeyEvent("CameraMoveRight", std::bind(&GlobalCameraWndManager::onFreelookMoveRightKey, this, std::placeholders::_1));
	GlobalEventManager().addKeyEvent("CameraMoveUp", std::bind(&GlobalCameraWndManager::onFreelookMoveUpKey, this, std::placeholders::_1));
	GlobalEventManager().addKeyEvent("CameraMoveDown", std::bind(&GlobalCameraWndManager::onFreelookMoveDownKey, this, std::placeholders::_1));
}

CamWndPtr GlobalCameraWndManager::getActiveCamWnd()
{
	// Sanity check in debug builds
	assert(_activeCam == -1 || _cameras.find(_activeCam) != _cameras.end());

	if (_activeCam == -1) return CamWndPtr();

	CamWndPtr cam = _cameras[_activeCam].lock();

	if (!cam)
	{
		// Camera is not used anymore, remove it
		removeCamWnd(_activeCam);

		// Find a new active camera
		if (!_cameras.empty())
		{
			_activeCam = _cameras.begin()->first;
		}
		else
		{
			// No more cameras available
			_activeCam = -1;
		}

		if (_activeCam != -1)
		{
			cam = _cameras[_activeCam].lock();
		}
	}

	return cam;
}

CamWndPtr GlobalCameraWndManager::createCamWnd(wxWindow* parent)
{
	// Instantantiate a new camera
	CamWndPtr cam(new CamWnd(parent));

	_cameras.insert(CamWndMap::value_type(cam->getId(), cam));

	if (_activeCam == -1) {
		_activeCam = cam->getId();
	}

	return cam;
}

void GlobalCameraWndManager::removeCamWnd(int id) {
	// Find and remove the CamWnd
	CamWndMap::iterator i = _cameras.find(id);

	if (i != _cameras.end()) {
		_cameras.erase(i);
	}

	if (_activeCam == id) {
		// Find a new active camera
		if (!_cameras.empty()) {
			_activeCam = _cameras.begin()->first;
		}
		else {
			// No more cameras available
			_activeCam = -1;
		}
	}
}

// Construct/return a floating window containing the CamWnd widget
FloatingCamWndPtr GlobalCameraWndManager::createFloatingWindow()
{
	// Create a new floating camera window widget and return it
	auto cam = std::make_shared<FloatingCamWnd>();

	_cameras.emplace(cam->getId(), cam);

	if (_activeCam == -1)
	{
		_activeCam = cam->getId();
	}

	return cam;
}

void GlobalCameraWndManager::resetCameraAngles(const cmd::ArgumentList& args)
{
	doWithActiveCamWnd([](CamWnd& camWnd) 
	{
		Vector3 angles;
		angles[camera::CAMERA_ROLL] = angles[camera::CAMERA_PITCH] = 0;
		angles[camera::CAMERA_YAW] = 22.5 * floor((camWnd.getCameraAngles()[camera::CAMERA_YAW] + 11) / 22.5);
		camWnd.setCameraAngles(angles);
	});
}

void GlobalCameraWndManager::increaseCameraSpeed(const cmd::ArgumentList& args) {

	int movementSpeed = registry::getValue<int>(RKEY_MOVEMENT_SPEED);
	movementSpeed *= 2;

	if (movementSpeed > MAX_CAMERA_SPEED){
		movementSpeed = MAX_CAMERA_SPEED;
	}

	registry::setValue(RKEY_MOVEMENT_SPEED, movementSpeed);
}

void GlobalCameraWndManager::decreaseCameraSpeed(const cmd::ArgumentList& args) {

	int movementSpeed = registry::getValue<int>(RKEY_MOVEMENT_SPEED);
	movementSpeed /= 2;

	if (movementSpeed < 1){
		movementSpeed = 1;
	}

	registry::setValue(RKEY_MOVEMENT_SPEED, movementSpeed);
}

void GlobalCameraWndManager::benchmark() 
{
	doWithActiveCamWnd([](CamWnd& camWnd) { camWnd.benchmark(); });
}

void GlobalCameraWndManager::update() {
	// Issue the update call to all cameras
	for (CamWndMap::iterator i = _cameras.begin(); i != _cameras.end(); /* in-loop */ ) {
		CamWndPtr cam = i->second.lock();

		if (cam != NULL) {
			cam->update();
			++i;
		}
		else {
			_cameras.erase(i++);
		}
	}
}

void GlobalCameraWndManager::forceDraw()
{
    // Issue the update call to all cameras
    for (CamWndMap::iterator i = _cameras.begin(); i != _cameras.end(); /* in-loop */)
    {
        CamWndPtr cam = i->second.lock();

        if (cam)
        {
            cam->forceRedraw();
            ++i;
        }
        else
        {
            _cameras.erase(i++);
        }
    }
}

void GlobalCameraWndManager::changeFloorUp(const cmd::ArgumentList& args)
{
	doWithActiveCamWnd([](CamWnd& camWnd) { camWnd.changeFloor(true); });
}

void GlobalCameraWndManager::changeFloorDown(const cmd::ArgumentList& args)
{
	doWithActiveCamWnd([](CamWnd& camWnd) { camWnd.changeFloor(false); });
}

void GlobalCameraWndManager::addCameraObserver(CameraObserver* observer) {
	if (observer != NULL) {
		// Add the passed observer to the list
		_cameraObservers.push_back(observer);
	}
}

void GlobalCameraWndManager::removeCameraObserver(CameraObserver* observer)
{
	// Cycle through the list of observers and call the moved method
	for (CameraObserverList::iterator i = _cameraObservers.begin(); i != _cameraObservers.end(); i++)
	{
		CameraObserver* registered = *i;

		if (registered == observer) {
			_cameraObservers.erase(i++);
			return; // Don't continue the loop, the iterator is obsolete
		}
	}
}

void GlobalCameraWndManager::movedNotify()
{
	// Cycle through the list of observers and call the moved method
	for (CameraObserver* observer : _cameraObservers)
	{
		if (observer != nullptr)
		{
			observer->cameraMoved();
		}
	}
}

void GlobalCameraWndManager::toggleLightingMode(const cmd::ArgumentList& args) {
	getCameraSettings()->toggleLightingMode();
}

void GlobalCameraWndManager::farClipPlaneIn(const cmd::ArgumentList& args)
{
	doWithActiveCamWnd([](CamWnd& camWnd) { camWnd.farClipPlaneIn(); });
}

void GlobalCameraWndManager::farClipPlaneOut(const cmd::ArgumentList& args)
{
	doWithActiveCamWnd([](CamWnd& camWnd) { camWnd.farClipPlaneOut(); });
}

void GlobalCameraWndManager::focusCamera(const Vector3& point, const Vector3& angles)
{
	doWithActiveCamWnd([&](CamWnd& camWnd)
	{
		camWnd.setCameraOrigin(point);
		camWnd.setCameraAngles(angles);
	});
}

ICameraView& GlobalCameraWndManager::getActiveView()
{
	auto camWnd = getActiveCamWnd();

	if (!camWnd) throw std::runtime_error("No active camera view present");

	return *camWnd;
}

// --------------- Keyboard movement methods ------------------------------------------

void GlobalCameraWndManager::moveCameraCmd(const cmd::ArgumentList& args)
{
	auto camWnd = getActiveCamWnd();
	if (!camWnd) return;

	if (args.size() != 2)
	{
		rMessage() << "Usage: MoveCamera <up|down|forward|back|left|right> <units>" << std::endl;
		rMessage() << "Example: MoveCamera forward '20' performs"
			<< " a 20 unit move in the forward direction." << std::endl;
		return;
	}

	std::string arg = string::to_lower_copy(args[0].getString());
	double amount = args[1].getDouble();

	if (amount <= 0)
	{
		rWarning() << "Unit amount must be > 0" << std::endl;
		return;
	}

	if (arg == "up") 
	{
		camWnd->getCamera().moveUpDiscrete(amount);
	}
	else if (arg == "down") 
	{
		camWnd->getCamera().moveDownDiscrete(amount);
	}
	else if (arg == "left") 
	{
		camWnd->getCamera().moveLeftDiscrete(amount);
	}
	if (arg == "right") 
	{
		camWnd->getCamera().moveRightDiscrete(amount);
	}
	else if (arg == "forward")
	{
		camWnd->getCamera().moveForwardDiscrete(amount);
	}
	else if (arg == "back")
	{
		camWnd->getCamera().moveBackDiscrete(amount);
	}
	else
	{
		rWarning() << "Unknown direction: " << arg << std::endl;
		rMessage() << "Possible dDirections are: <up|down|forward|back|left|right>" << std::endl;
	}
}

void GlobalCameraWndManager::doWithActiveCamWnd(const std::function<void(CamWnd&)>& action)
{
	auto camWnd = getActiveCamWnd();
	
	if (camWnd)
	{
		action(*camWnd);
	}
}

void GlobalCameraWndManager::onFreelookMoveForwardKey(ui::KeyEventType eventType)
{
	doWithActiveCamWnd([&](CamWnd& cam) { cam.onForwardKey(eventType); });
}

void GlobalCameraWndManager::onFreelookMoveBackKey(ui::KeyEventType eventType)
{
	doWithActiveCamWnd([&](CamWnd& cam) { cam.onBackwardKey(eventType); });
}

void GlobalCameraWndManager::onFreelookMoveLeftKey(ui::KeyEventType eventType)
{
	doWithActiveCamWnd([&](CamWnd& cam) { cam.onLeftKey(eventType); });
}

void GlobalCameraWndManager::onFreelookMoveRightKey(ui::KeyEventType eventType)
{
	doWithActiveCamWnd([&](CamWnd& cam) { cam.onRightKey(eventType); });
}

void GlobalCameraWndManager::onFreelookMoveUpKey(ui::KeyEventType eventType)
{
	doWithActiveCamWnd([&](CamWnd& cam) { cam.onUpKey(eventType); });
}

void GlobalCameraWndManager::onFreelookMoveDownKey(ui::KeyEventType eventType)
{
	doWithActiveCamWnd([&](CamWnd& cam) { cam.onDownKey(eventType); });
}

void GlobalCameraWndManager::moveLeftDiscrete(const cmd::ArgumentList& args)
{
	doWithActiveCamWnd([](CamWnd& cam) { cam.getCamera().moveLeftDiscrete(SPEED_MOVE); });
}

void GlobalCameraWndManager::moveRightDiscrete(const cmd::ArgumentList& args)
{
	doWithActiveCamWnd([](CamWnd& cam) { cam.getCamera().moveRightDiscrete(SPEED_MOVE); });
}

void GlobalCameraWndManager::pitchUpDiscrete(const cmd::ArgumentList& args)
{
	doWithActiveCamWnd([](CamWnd& cam) { cam.getCamera().pitchUpDiscrete(); });
}

void GlobalCameraWndManager::pitchDownDiscrete(const cmd::ArgumentList& args)
{
	doWithActiveCamWnd([](CamWnd& cam) { cam.getCamera().pitchDownDiscrete(); });
}

float GlobalCameraWndManager::getCameraStrafeSpeed()
{
    return _strafeSpeed;
}

float GlobalCameraWndManager::getCameraForwardStrafeFactor()
{
    return _forwardStrafeFactor;
}

unsigned int GlobalCameraWndManager::getStrafeModifierFlags()
{
    return _toggleStrafeModifierFlags;
}

unsigned int GlobalCameraWndManager::getStrafeForwardModifierFlags()
{
    return _toggleStrafeForwardModifierFlags;
}

ui::MouseToolStack GlobalCameraWndManager::getMouseToolsForEvent(wxMouseEvent& ev)
{
    unsigned int state = wxutil::MouseButton::GetButtonStateChangeForMouseEvent(ev);
    return GlobalMouseToolManager().getMouseToolsForEvent(ui::IMouseToolGroup::Type::CameraView, state);
}

void GlobalCameraWndManager::foreachMouseTool(const std::function<void(const ui::MouseToolPtr&)>& func)
{
    GlobalMouseToolManager().getGroup(ui::IMouseToolGroup::Type::CameraView).foreachMouseTool(func);
}

void GlobalCameraWndManager::loadCameraStrafeDefinitions()
{
    // Find all the camera strafe definitions
    xml::NodeList strafeList = GlobalRegistry().findXPath("user/ui/input/cameraview/strafemode");

    if (!strafeList.empty())
    {
        const xml::Node& node = strafeList[0];

        // Get the strafe condition flags
        _toggleStrafeModifierFlags = wxutil::Modifier::GetStateFromModifierString(node.getAttributeValue("toggle"));
        _toggleStrafeForwardModifierFlags = wxutil::Modifier::GetStateFromModifierString(node.getAttributeValue("forward"));

        _strafeSpeed = string::convert<float>(node.getAttributeValue("speed"), DEFAULT_STRAFE_SPEED);
        _forwardStrafeFactor = string::convert<float>(node.getAttributeValue("forwardFactor"), DEFAULT_FORWARD_STRAFE_FACTOR);
    }
    else
    {
        // No Camera strafe definitions found!
        rWarning() << "GlobalCameraWndManager: No camera strafe definitions found!" << std::endl;
    }
}

// RegisterableModule implementation
const std::string& GlobalCameraWndManager::getName() const {
	static std::string _name(MODULE_CAMERA_MANAGER);
	return _name;
}

const StringSet& GlobalCameraWndManager::getDependencies() const
{
	static StringSet _dependencies;

	if (_dependencies.empty())
	{
		_dependencies.insert(MODULE_XMLREGISTRY);
		_dependencies.insert(MODULE_EVENTMANAGER);
		_dependencies.insert(MODULE_RENDERSYSTEM);
		_dependencies.insert(MODULE_COMMANDSYSTEM);
        _dependencies.insert(MODULE_MOUSETOOLMANAGER);
		_dependencies.insert(MODULE_UIMANAGER);
	}

	return _dependencies;
}

void GlobalCameraWndManager::initialiseModule(const IApplicationContext& ctx)
{
	rMessage() << getName() << "::initialiseModule called." << std::endl;

	// greebo: If at startup time the render mode is set to LIGHTING, fall back
	// to textured. During startup the openGL contexts are not realised yet and the
	// openGL module is tricked into believing there are no GLSL shader programs supported.
	// Later on, when switching back to TEXTURED the rendersystem will attempt to destroy
	// program objects it never created.
	if (registry::getValue<int>(RKEY_DRAWMODE) == RENDER_MODE_LIGHTING)
	{
		registry::setValue(RKEY_DRAWMODE, RENDER_MODE_TEXTURED);
	}

	registerCommands();
    loadCameraStrafeDefinitions();

	CamWnd::captureStates();

    IMouseToolGroup& toolGroup = GlobalMouseToolManager().getGroup(IMouseToolGroup::Type::CameraView);

    toolGroup.registerMouseTool(std::make_shared<FreeMoveTool>());
	toolGroup.registerMouseTool(std::make_shared<PanViewTool>());
    toolGroup.registerMouseTool(std::make_shared<PickShaderTool>());
    toolGroup.registerMouseTool(std::make_shared<PasteShaderProjectedTool>());
    toolGroup.registerMouseTool(std::make_shared<PasteShaderNaturalTool>());
    toolGroup.registerMouseTool(std::make_shared<PasteShaderCoordsTool>());
    toolGroup.registerMouseTool(std::make_shared<PasteShaderToBrushTool>());
    toolGroup.registerMouseTool(std::make_shared<PasteShaderNameTool>());
    toolGroup.registerMouseTool(std::make_shared<JumpToObjectTool>());
}

void GlobalCameraWndManager::shutdownModule()
{
	CamWnd::releaseStates();

	_cameras.clear();
}

// Define the static Camera module
module::StaticModule<GlobalCameraWndManager> cameraWndManagerModule;

} // namespace

// The accessor function to the GlobalCameraWndManager instance
ui::GlobalCameraWndManager& GlobalCamera()
{
	return *std::static_pointer_cast<ui::GlobalCameraWndManager>(
		module::GlobalModuleRegistry().getModule(MODULE_CAMERA_MANAGER)
	);
}
