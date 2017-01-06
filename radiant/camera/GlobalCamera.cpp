#include "GlobalCamera.h"

#include "imousetoolmanager.h"
#include "ieventmanager.h"
#include "iselection.h"
#include "iuimanager.h"
#include "itextstream.h"
#include "xmlutil/Node.h"

#include "Camera.h"
#include "CameraSettings.h"

#include "registry/registry.h"
#include "modulesystem/StaticModule.h"
#include "wxutil/MouseButton.h"

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
GlobalCameraManager::GlobalCameraManager() :
	_activeCam(-1),
    _toggleStrafeModifierFlags(wxutil::Modifier::NONE),
    _toggleStrafeForwardModifierFlags(wxutil::Modifier::NONE),
    _strafeSpeed(DEFAULT_STRAFE_SPEED),
    _forwardStrafeFactor(DEFAULT_FORWARD_STRAFE_FACTOR)
{}

void GlobalCameraManager::registerCommands()
{
	GlobalCommandSystem().addCommand("CenterView", std::bind(&GlobalCameraManager::resetCameraAngles, this, std::placeholders::_1));
	GlobalCommandSystem().addCommand("CubicClipZoomIn", std::bind(&GlobalCameraManager::farClipPlaneIn, this, std::placeholders::_1));
	GlobalCommandSystem().addCommand("CubicClipZoomOut", std::bind(&GlobalCameraManager::farClipPlaneOut, this, std::placeholders::_1));

	GlobalCommandSystem().addCommand("UpFloor", std::bind(&GlobalCameraManager::changeFloorUp, this, std::placeholders::_1));
	GlobalCommandSystem().addCommand("DownFloor", std::bind(&GlobalCameraManager::changeFloorDown, this, std::placeholders::_1));

	// angua: increases and decreases the movement speed of the camera
	GlobalCommandSystem().addCommand("CamIncreaseMoveSpeed", std::bind(&GlobalCameraManager::increaseCameraSpeed, this, std::placeholders::_1));
	GlobalCommandSystem().addCommand("CamDecreaseMoveSpeed", std::bind(&GlobalCameraManager::decreaseCameraSpeed, this, std::placeholders::_1));

	GlobalCommandSystem().addCommand("TogglePreview", std::bind(&GlobalCameraManager::toggleLightingMode, this, std::placeholders::_1));

	// Insert movement commands
	GlobalCommandSystem().addCommand("CameraForward", std::bind(&GlobalCameraManager::moveForwardDiscrete, this, std::placeholders::_1));
	GlobalCommandSystem().addCommand("CameraBack", std::bind(&GlobalCameraManager::moveBackDiscrete, this, std::placeholders::_1));
	GlobalCommandSystem().addCommand("CameraLeft", std::bind(&GlobalCameraManager::rotateLeftDiscrete, this, std::placeholders::_1));
	GlobalCommandSystem().addCommand("CameraRight", std::bind(&GlobalCameraManager::rotateRightDiscrete, this, std::placeholders::_1));
	GlobalCommandSystem().addCommand("CameraStrafeRight", std::bind(&GlobalCameraManager::moveRightDiscrete, this, std::placeholders::_1));
	GlobalCommandSystem().addCommand("CameraStrafeLeft", std::bind(&GlobalCameraManager::moveLeftDiscrete, this, std::placeholders::_1));

	GlobalCommandSystem().addCommand("CameraUp", std::bind(&GlobalCameraManager::moveUpDiscrete, this, std::placeholders::_1));
	GlobalCommandSystem().addCommand("CameraDown", std::bind(&GlobalCameraManager::moveDownDiscrete, this, std::placeholders::_1));
	GlobalCommandSystem().addCommand("CameraAngleUp", std::bind(&GlobalCameraManager::pitchUpDiscrete, this, std::placeholders::_1));
	GlobalCommandSystem().addCommand("CameraAngleDown", std::bind(&GlobalCameraManager::pitchDownDiscrete, this, std::placeholders::_1));

	// Bind the events to the commands
	GlobalEventManager().addCommand("CenterView", "CenterView");

	GlobalEventManager().addToggle(
        "ToggleCubicClip",
        std::bind(&CameraSettings::toggleFarClip, getCameraSettings(), std::placeholders::_1)
    );
	// Set the default status of the cubic clip
	GlobalEventManager().setToggled("ToggleCubicClip", getCameraSettings()->farClipEnabled());

	GlobalEventManager().addCommand("CubicClipZoomIn", "CubicClipZoomIn");
	GlobalEventManager().addCommand("CubicClipZoomOut", "CubicClipZoomOut");

	GlobalEventManager().addCommand("UpFloor", "UpFloor");
	GlobalEventManager().addCommand("DownFloor", "DownFloor");

	GlobalEventManager().addWidgetToggle("ToggleCamera");
	GlobalEventManager().setToggled("ToggleCamera", true);

	// angua: increases and decreases the movement speed of the camera
	GlobalEventManager().addCommand("CamIncreaseMoveSpeed", "CamIncreaseMoveSpeed");
	GlobalEventManager().addCommand("CamDecreaseMoveSpeed", "CamDecreaseMoveSpeed");

	GlobalEventManager().addCommand("TogglePreview", "TogglePreview");

	// Insert movement commands
	GlobalEventManager().addCommand("CameraForward", "CameraForward");
	GlobalEventManager().addCommand("CameraBack", "CameraBack");
	GlobalEventManager().addCommand("CameraLeft", "CameraLeft");
	GlobalEventManager().addCommand("CameraRight", "CameraRight");
	GlobalEventManager().addCommand("CameraStrafeRight", "CameraStrafeRight");
	GlobalEventManager().addCommand("CameraStrafeLeft", "CameraStrafeLeft");

	GlobalEventManager().addCommand("CameraUp", "CameraUp");
	GlobalEventManager().addCommand("CameraDown", "CameraDown");
	GlobalEventManager().addCommand("CameraAngleUp", "CameraAngleUp");
	GlobalEventManager().addCommand("CameraAngleDown", "CameraAngleDown");

	GlobalEventManager().addKeyEvent("CameraFreeMoveForward", std::bind(&GlobalCameraManager::onFreelookMoveForwardKey, this, std::placeholders::_1));
	GlobalEventManager().addKeyEvent("CameraFreeMoveBack", std::bind(&GlobalCameraManager::onFreelookMoveBackKey, this, std::placeholders::_1));
	GlobalEventManager().addKeyEvent("CameraFreeMoveLeft", std::bind(&GlobalCameraManager::onFreelookMoveLeftKey, this, std::placeholders::_1));
	GlobalEventManager().addKeyEvent("CameraFreeMoveRight", std::bind(&GlobalCameraManager::onFreelookMoveRightKey, this, std::placeholders::_1));
	GlobalEventManager().addKeyEvent("CameraFreeMoveUp", std::bind(&GlobalCameraManager::onFreelookMoveUpKey, this, std::placeholders::_1));
	GlobalEventManager().addKeyEvent("CameraFreeMoveDown", std::bind(&GlobalCameraManager::onFreelookMoveDownKey, this, std::placeholders::_1));
}

CamWndPtr GlobalCameraManager::getActiveCamWnd() {
	// Sanity check in debug builds
	assert(_cameras.find(_activeCam) != _cameras.end());

	CamWndPtr cam = _cameras[_activeCam].lock();

	if (cam == NULL) {
		// Camera is not used anymore, remove it
		removeCamWnd(_activeCam);

		// Find a new active camera
		if (!_cameras.empty()) {
			_activeCam = _cameras.begin()->first;
		}
		else {
			// No more cameras available
			_activeCam = -1;
		}

		if (_activeCam != -1) {
			cam = _cameras[_activeCam].lock();
		}
	}

	return (_activeCam != -1) ? cam : CamWndPtr();
}

CamWndPtr GlobalCameraManager::createCamWnd(wxWindow* parent)
{
	// Instantantiate a new camera
	CamWndPtr cam(new CamWnd(parent));

	_cameras.insert(CamWndMap::value_type(cam->getId(), cam));

	if (_activeCam == -1) {
		_activeCam = cam->getId();
	}

	return cam;
}

void GlobalCameraManager::removeCamWnd(int id) {
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
FloatingCamWndPtr GlobalCameraManager::createFloatingWindow()
{
	// Create a new floating camera window widget and return it
	FloatingCamWndPtr cam(new FloatingCamWnd);

	_cameras.insert(CamWndMap::value_type(cam->getId(), cam));

	if (_activeCam == -1)
	{
		_activeCam = cam->getId();
	}

	return cam;
}

void GlobalCameraManager::resetCameraAngles(const cmd::ArgumentList& args)
{
	CamWndPtr camWnd = getActiveCamWnd();

	if (camWnd != NULL) {
		Vector3 angles;
		angles[CAMERA_ROLL] = angles[CAMERA_PITCH] = 0;
		angles[CAMERA_YAW] = 22.5 * floor((camWnd->getCameraAngles()[CAMERA_YAW]+11)/22.5);
		camWnd->setCameraAngles(angles);
	}
}

void GlobalCameraManager::increaseCameraSpeed(const cmd::ArgumentList& args) {

	int movementSpeed = registry::getValue<int>(RKEY_MOVEMENT_SPEED);
	movementSpeed *= 2;

	if (movementSpeed > MAX_CAMERA_SPEED){
		movementSpeed = MAX_CAMERA_SPEED;
	}

	registry::setValue(RKEY_MOVEMENT_SPEED, movementSpeed);
}

void GlobalCameraManager::decreaseCameraSpeed(const cmd::ArgumentList& args) {

	int movementSpeed = registry::getValue<int>(RKEY_MOVEMENT_SPEED);
	movementSpeed /= 2;

	if (movementSpeed < 1){
		movementSpeed = 1;
	}

	registry::setValue(RKEY_MOVEMENT_SPEED, movementSpeed);
}

void GlobalCameraManager::benchmark() {
	CamWndPtr camWnd = getActiveCamWnd();

	if (camWnd != NULL) {
		camWnd->benchmark();
	}
}

void GlobalCameraManager::update() {
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

void GlobalCameraManager::forceDraw()
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

void GlobalCameraManager::changeFloorUp(const cmd::ArgumentList& args) {
	CamWndPtr camWnd = getActiveCamWnd();
	if (camWnd == NULL) return;

	// Pass the call to the currently active CamWnd
	camWnd->changeFloor(true);
}

void GlobalCameraManager::changeFloorDown(const cmd::ArgumentList& args) {
	CamWndPtr camWnd = getActiveCamWnd();
	if (camWnd == NULL) return;

	// Pass the call to the currently active CamWnd
	camWnd->changeFloor(false);
}

void GlobalCameraManager::addCameraObserver(CameraObserver* observer) {
	if (observer != NULL) {
		// Add the passed observer to the list
		_cameraObservers.push_back(observer);
	}
}

void GlobalCameraManager::removeCameraObserver(CameraObserver* observer) {
	// Cycle through the list of observers and call the moved method
	for (CameraObserverList::iterator i = _cameraObservers.begin(); i != _cameraObservers.end(); i++) {
		CameraObserver* registered = *i;

		if (registered == observer) {
			_cameraObservers.erase(i++);
			return; // Don't continue the loop, the iterator is obsolete
		}
	}
}

void GlobalCameraManager::movedNotify() {

	// Cycle through the list of observers and call the moved method
	for (CameraObserverList::iterator i = _cameraObservers.begin(); i != _cameraObservers.end(); i++) {
		CameraObserver* observer = *i;

		if (observer != NULL) {
			observer->cameraMoved();
		}
	}
}

void GlobalCameraManager::toggleLightingMode(const cmd::ArgumentList& args) {
	getCameraSettings()->toggleLightingMode();
}

void GlobalCameraManager::farClipPlaneIn(const cmd::ArgumentList& args) {
	CamWndPtr camWnd = getActiveCamWnd();
	if (camWnd == NULL) return;

	camWnd->farClipPlaneIn();
}

void GlobalCameraManager::farClipPlaneOut(const cmd::ArgumentList& args) {
	CamWndPtr camWnd = getActiveCamWnd();
	if (camWnd == NULL) return;

	camWnd->farClipPlaneOut();
}

void GlobalCameraManager::focusCamera(const Vector3& point, const Vector3& angles) {
	CamWndPtr camWnd = getActiveCamWnd();
	if (camWnd == NULL) return;

	camWnd->setCameraOrigin(point);
	camWnd->setCameraAngles(angles);
}

// --------------- Keyboard movement methods ------------------------------------------

void GlobalCameraManager::onFreelookMoveForwardKey(ui::KeyEventType eventType)
{
	CamWndPtr camWnd = getActiveCamWnd();
	if (camWnd == NULL) return;

	if (eventType == ui::KeyPressed)
	{
		camWnd->getCamera().setMovementFlags(MOVE_FORWARD);
	}
	else
	{
		camWnd->getCamera().clearMovementFlags(MOVE_FORWARD);
	}
}

void GlobalCameraManager::onFreelookMoveBackKey(ui::KeyEventType eventType)
{
	CamWndPtr camWnd = getActiveCamWnd();
	if (camWnd == NULL) return;

	if (eventType == ui::KeyPressed)
	{
		camWnd->getCamera().setMovementFlags(MOVE_BACK);
	}
	else
	{
		camWnd->getCamera().clearMovementFlags(MOVE_BACK);
	}
}

void GlobalCameraManager::onFreelookMoveLeftKey(ui::KeyEventType eventType)
{
	CamWndPtr camWnd = getActiveCamWnd();
	if (camWnd == NULL) return;

	if (eventType == ui::KeyPressed)
	{
		camWnd->getCamera().setMovementFlags(MOVE_STRAFELEFT);
	}
	else
	{
		camWnd->getCamera().clearMovementFlags(MOVE_STRAFELEFT);
	}
}

void GlobalCameraManager::onFreelookMoveRightKey(ui::KeyEventType eventType)
{
	CamWndPtr camWnd = getActiveCamWnd();
	if (camWnd == NULL) return;

	if (eventType == ui::KeyPressed)
	{
		camWnd->getCamera().setMovementFlags(MOVE_STRAFERIGHT);
	}
	else
	{
		camWnd->getCamera().clearMovementFlags(MOVE_STRAFERIGHT);
	}
}

void GlobalCameraManager::onFreelookMoveUpKey(ui::KeyEventType eventType)
{
	CamWndPtr camWnd = getActiveCamWnd();
	if (camWnd == NULL) return;

	if (eventType == ui::KeyPressed)
	{
		camWnd->getCamera().setMovementFlags(MOVE_UP);
	}
	else
	{
		camWnd->getCamera().clearMovementFlags(MOVE_UP);
	}
}

void GlobalCameraManager::onFreelookMoveDownKey(ui::KeyEventType eventType)
{
	CamWndPtr camWnd = getActiveCamWnd();
	if (camWnd == NULL) return;

	if (eventType == ui::KeyPressed)
	{
		camWnd->getCamera().setMovementFlags(MOVE_DOWN);
	}
	else
	{
		camWnd->getCamera().clearMovementFlags(MOVE_DOWN);
	}
}

void GlobalCameraManager::moveForwardDiscrete(const cmd::ArgumentList& args) {
	CamWndPtr camWnd = getActiveCamWnd();
	if (camWnd == NULL) return;

	camWnd->getCamera().moveForwardDiscrete();
}

void GlobalCameraManager::moveBackDiscrete(const cmd::ArgumentList& args) {
	CamWndPtr camWnd = getActiveCamWnd();
	if (camWnd == NULL) return;

	camWnd->getCamera().moveBackDiscrete();
}

void GlobalCameraManager::moveUpDiscrete(const cmd::ArgumentList& args) {
	CamWndPtr camWnd = getActiveCamWnd();
	if (camWnd == NULL) return;

	camWnd->getCamera().moveUpDiscrete();
}

void GlobalCameraManager::moveDownDiscrete(const cmd::ArgumentList& args) {
	CamWndPtr camWnd = getActiveCamWnd();
	if (camWnd == NULL) return;

	camWnd->getCamera().moveDownDiscrete();
}

void GlobalCameraManager::moveLeftDiscrete(const cmd::ArgumentList& args) {
	CamWndPtr camWnd = getActiveCamWnd();
	if (camWnd == NULL) return;

	camWnd->getCamera().moveLeftDiscrete();
}

void GlobalCameraManager::moveRightDiscrete(const cmd::ArgumentList& args) {
	CamWndPtr camWnd = getActiveCamWnd();
	if (camWnd == NULL) return;

	camWnd->getCamera().moveRightDiscrete();
}

void GlobalCameraManager::rotateLeftDiscrete(const cmd::ArgumentList& args) {
	CamWndPtr camWnd = getActiveCamWnd();
	if (camWnd == NULL) return;

	camWnd->getCamera().rotateLeftDiscrete();
}

void GlobalCameraManager::rotateRightDiscrete(const cmd::ArgumentList& args) {
	CamWndPtr camWnd = getActiveCamWnd();
	if (camWnd == NULL) return;

	camWnd->getCamera().rotateRightDiscrete();
}

void GlobalCameraManager::pitchUpDiscrete(const cmd::ArgumentList& args) {
	CamWndPtr camWnd = getActiveCamWnd();
	if (camWnd == NULL) return;

	camWnd->getCamera().pitchUpDiscrete();
}

void GlobalCameraManager::pitchDownDiscrete(const cmd::ArgumentList& args) {
	CamWndPtr camWnd = getActiveCamWnd();
	if (camWnd == NULL) return;

	camWnd->getCamera().pitchDownDiscrete();
}

float GlobalCameraManager::getCameraStrafeSpeed()
{
    return _strafeSpeed;
}

float GlobalCameraManager::getCameraForwardStrafeFactor()
{
    return _forwardStrafeFactor;
}

unsigned int GlobalCameraManager::getStrafeModifierFlags()
{
    return _toggleStrafeModifierFlags;
}

unsigned int GlobalCameraManager::getStrafeForwardModifierFlags()
{
    return _toggleStrafeForwardModifierFlags;
}

ui::MouseToolStack GlobalCameraManager::getMouseToolsForEvent(wxMouseEvent& ev)
{
    unsigned int state = wxutil::MouseButton::GetButtonStateChangeForMouseEvent(ev);
    return GlobalMouseToolManager().getMouseToolsForEvent(ui::IMouseToolGroup::Type::CameraView, state);
}

void GlobalCameraManager::foreachMouseTool(const std::function<void(const ui::MouseToolPtr&)>& func)
{
    GlobalMouseToolManager().getGroup(ui::IMouseToolGroup::Type::CameraView).foreachMouseTool(func);
}

void GlobalCameraManager::loadCameraStrafeDefinitions()
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
        rWarning() << "GlobalCameraManager: No camera strafe definitions found!" << std::endl;
    }
}

// RegisterableModule implementation
const std::string& GlobalCameraManager::getName() const {
	static std::string _name(MODULE_CAMERA);
	return _name;
}

const StringSet& GlobalCameraManager::getDependencies() const
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

void GlobalCameraManager::initialiseModule(const ApplicationContext& ctx)
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

void GlobalCameraManager::shutdownModule()
{
	CamWnd::releaseStates();

	_cameras.clear();
}

// Define the static Camera module
module::StaticModule<GlobalCameraManager> cameraModule;

} // namespace

// The accessor function to the GlobalCameraManager instance
ui::GlobalCameraManager& GlobalCamera()
{
	return *ui::cameraModule.getModule();
}
