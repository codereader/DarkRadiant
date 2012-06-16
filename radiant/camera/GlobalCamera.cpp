#include "GlobalCamera.h"

#include "ieventmanager.h"
#include "iselection.h"
#include "gdk/gdkkeysyms.h"
#include "xmlutil/Node.h"

#include "Camera.h"
#include "CameraSettings.h"

#include "registry/registry.h"
#include "gtkutil/window/PersistentTransientWindow.h"
#include "gtkutil/FramedWidget.h"
#include "modulesystem/StaticModule.h"

#include "FloatingCamWnd.h"
#include <boost/bind.hpp>

// Constructor
GlobalCameraManager::GlobalCameraManager() :
	_activeCam(-1)
{}

void GlobalCameraManager::registerCommands()
{
	GlobalCommandSystem().addCommand("CenterView", boost::bind(&GlobalCameraManager::resetCameraAngles, this, _1));
	GlobalCommandSystem().addCommand("CubicClipZoomIn", boost::bind(&GlobalCameraManager::farClipPlaneIn, this, _1));
	GlobalCommandSystem().addCommand("CubicClipZoomOut", boost::bind(&GlobalCameraManager::farClipPlaneOut, this, _1));

	GlobalCommandSystem().addCommand("UpFloor", boost::bind(&GlobalCameraManager::changeFloorUp, this, _1));
	GlobalCommandSystem().addCommand("DownFloor", boost::bind(&GlobalCameraManager::changeFloorDown, this, _1));

	// angua: increases and decreases the movement speed of the camera
	GlobalCommandSystem().addCommand("CamIncreaseMoveSpeed", boost::bind(&GlobalCameraManager::increaseCameraSpeed, this, _1));
	GlobalCommandSystem().addCommand("CamDecreaseMoveSpeed", boost::bind(&GlobalCameraManager::decreaseCameraSpeed, this, _1));

	GlobalCommandSystem().addCommand("TogglePreview", boost::bind(&GlobalCameraManager::toggleLightingMode, this, _1));

	// Insert movement commands
	GlobalCommandSystem().addCommand("CameraForward", boost::bind(&GlobalCameraManager::moveForwardDiscrete, this, _1));
	GlobalCommandSystem().addCommand("CameraBack", boost::bind(&GlobalCameraManager::moveBackDiscrete, this, _1));
	GlobalCommandSystem().addCommand("CameraLeft", boost::bind(&GlobalCameraManager::rotateLeftDiscrete, this, _1));
	GlobalCommandSystem().addCommand("CameraRight", boost::bind(&GlobalCameraManager::rotateRightDiscrete, this, _1));
	GlobalCommandSystem().addCommand("CameraStrafeRight", boost::bind(&GlobalCameraManager::moveRightDiscrete, this, _1));
	GlobalCommandSystem().addCommand("CameraStrafeLeft", boost::bind(&GlobalCameraManager::moveLeftDiscrete, this, _1));

	GlobalCommandSystem().addCommand("CameraUp", boost::bind(&GlobalCameraManager::moveUpDiscrete, this, _1));
	GlobalCommandSystem().addCommand("CameraDown", boost::bind(&GlobalCameraManager::moveDownDiscrete, this, _1));
	GlobalCommandSystem().addCommand("CameraAngleUp", boost::bind(&GlobalCameraManager::pitchUpDiscrete, this, _1));
	GlobalCommandSystem().addCommand("CameraAngleDown", boost::bind(&GlobalCameraManager::pitchDownDiscrete, this, _1));

	// Bind the events to the commands
	GlobalEventManager().addCommand("CenterView", "CenterView");

	GlobalEventManager().addToggle(
        "ToggleCubicClip",
        boost::bind(&CameraSettings::toggleFarClip, getCameraSettings(), _1)
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

	GlobalEventManager().addKeyEvent("CameraFreeMoveForward", boost::bind(&GlobalCameraManager::onFreelookMoveForwardKey, this, _1));
	GlobalEventManager().addKeyEvent("CameraFreeMoveBack", boost::bind(&GlobalCameraManager::onFreelookMoveBackKey, this, _1));
	GlobalEventManager().addKeyEvent("CameraFreeMoveLeft", boost::bind(&GlobalCameraManager::onFreelookMoveLeftKey, this, _1));
	GlobalEventManager().addKeyEvent("CameraFreeMoveRight", boost::bind(&GlobalCameraManager::onFreelookMoveRightKey, this, _1));
	GlobalEventManager().addKeyEvent("CameraFreeMoveUp", boost::bind(&GlobalCameraManager::onFreelookMoveUpKey, this, _1));
	GlobalEventManager().addKeyEvent("CameraFreeMoveDown", boost::bind(&GlobalCameraManager::onFreelookMoveDownKey, this, _1));
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

CamWndPtr GlobalCameraManager::createCamWnd() {
	// Instantantiate a new camera
	CamWndPtr cam(new CamWnd());

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
	FloatingCamWndPtr cam(new FloatingCamWnd(_parent));

	_cameras.insert(CamWndMap::value_type(cam->getId(), cam));

	if (_activeCam == -1) {
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

// Set the global parent window
void GlobalCameraManager::setParent(const Glib::RefPtr<Gtk::Window>& parent)
{
	_parent = parent;
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

	CamWnd::captureStates();
}

void GlobalCameraManager::shutdownModule()
{
	CamWnd::releaseStates();

	_cameras.clear();
}

// Define the static Camera module
module::StaticModule<GlobalCameraManager> cameraModule;

// ------------------------------------------------------------------------------------

// The accessor function to the GlobalCameraManager instance
GlobalCameraManager& GlobalCamera() {
	return *cameraModule.getModule();
}
