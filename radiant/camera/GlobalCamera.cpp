#include "GlobalCamera.h"

#include "ieventmanager.h"
#include "iselection.h"
#include "iregistry.h"
#include "gdk/gdkkeysyms.h"
#include "xmlutil/Node.h"

#include "Camera.h"
#include "CameraSettings.h"

#include "gtkutil/window/PersistentTransientWindow.h"
#include "gtkutil/FramedWidget.h"
#include "modulesystem/StaticModule.h"

#include "FloatingCamWnd.h"

// Constructor
GlobalCameraManager::GlobalCameraManager() :
	_activeCam(-1),
	_parent(NULL)
{}

void GlobalCameraManager::construct() {
	GlobalEventManager().addCommand("CenterView", MemberCaller<GlobalCameraManager, &GlobalCameraManager::resetCameraAngles>(*this));

	GlobalEventManager().addToggle("ToggleCubicClip", MemberCaller<CameraSettings, &CameraSettings::toggleFarClip>(*getCameraSettings()));
	// Set the default status of the cubic clip
	GlobalEventManager().setToggled("ToggleCubicClip", getCameraSettings()->farClipEnabled());

	GlobalEventManager().addCommand("CubicClipZoomIn", MemberCaller<GlobalCameraManager, &GlobalCameraManager::cubicScaleIn>(*this));
	GlobalEventManager().addCommand("CubicClipZoomOut", MemberCaller<GlobalCameraManager, &GlobalCameraManager::cubicScaleOut>(*this));

	GlobalEventManager().addCommand("UpFloor", MemberCaller<GlobalCameraManager, &GlobalCameraManager::changeFloorUp>(*this));
	GlobalEventManager().addCommand("DownFloor", MemberCaller<GlobalCameraManager, &GlobalCameraManager::changeFloorDown>(*this));

	GlobalEventManager().addWidgetToggle("ToggleCamera");
	GlobalEventManager().setToggled("ToggleCamera", true);

	// angua: increases and decreases the movement speed of the camera
	GlobalEventManager().addCommand("CamIncreaseMoveSpeed", MemberCaller<GlobalCameraManager, &GlobalCameraManager::increaseCameraSpeed>(*this));
	GlobalEventManager().addCommand("CamDecreaseMoveSpeed", MemberCaller<GlobalCameraManager, &GlobalCameraManager::decreaseCameraSpeed>(*this));
	
	GlobalEventManager().addCommand("TogglePreview", MemberCaller<GlobalCameraManager, &GlobalCameraManager::toggleLightingMode>(*this));
	
	// Insert movement commands
	GlobalEventManager().addCommand("CameraForward", MemberCaller<GlobalCameraManager, &GlobalCameraManager::moveForwardDiscrete>(*this));
	GlobalEventManager().addCommand("CameraBack", MemberCaller<GlobalCameraManager, &GlobalCameraManager::moveBackDiscrete>(*this));
	GlobalEventManager().addCommand("CameraLeft", MemberCaller<GlobalCameraManager, &GlobalCameraManager::rotateLeftDiscrete>(*this));
	GlobalEventManager().addCommand("CameraRight", MemberCaller<GlobalCameraManager, &GlobalCameraManager::rotateRightDiscrete>(*this));
	GlobalEventManager().addCommand("CameraStrafeRight", MemberCaller<GlobalCameraManager, &GlobalCameraManager::moveRightDiscrete>(*this));
	GlobalEventManager().addCommand("CameraStrafeLeft", MemberCaller<GlobalCameraManager, &GlobalCameraManager::moveLeftDiscrete>(*this));

	GlobalEventManager().addCommand("CameraUp", MemberCaller<GlobalCameraManager, &GlobalCameraManager::moveUpDiscrete>(*this));
	GlobalEventManager().addCommand("CameraDown", MemberCaller<GlobalCameraManager, &GlobalCameraManager::moveDownDiscrete>(*this));
	GlobalEventManager().addCommand("CameraAngleUp", MemberCaller<GlobalCameraManager, &GlobalCameraManager::pitchUpDiscrete>(*this));
	GlobalEventManager().addCommand("CameraAngleDown", MemberCaller<GlobalCameraManager, &GlobalCameraManager::pitchDownDiscrete>(*this));
	
	GlobalEventManager().addKeyEvent("CameraFreeMoveForward",
	                       MemberCaller<GlobalCameraManager, &GlobalCameraManager::freelookMoveForwardKeyUp>(*this),
	                       MemberCaller<GlobalCameraManager, &GlobalCameraManager::freelookMoveForwardKeyDown>(*this));
	GlobalEventManager().addKeyEvent("CameraFreeMoveBack",
	                       MemberCaller<GlobalCameraManager, &GlobalCameraManager::freelookMoveBackKeyUp>(*this),
	                       MemberCaller<GlobalCameraManager, &GlobalCameraManager::freelookMoveBackKeyDown>(*this));
	GlobalEventManager().addKeyEvent("CameraFreeMoveLeft",
	                       MemberCaller<GlobalCameraManager, &GlobalCameraManager::freelookMoveLeftKeyUp>(*this),
	                       MemberCaller<GlobalCameraManager, &GlobalCameraManager::freelookMoveLeftKeyDown>(*this));
	GlobalEventManager().addKeyEvent("CameraFreeMoveRight",
	                       MemberCaller<GlobalCameraManager, &GlobalCameraManager::freelookMoveRightKeyUp>(*this),
	                       MemberCaller<GlobalCameraManager, &GlobalCameraManager::freelookMoveRightKeyDown>(*this));
	GlobalEventManager().addKeyEvent("CameraFreeMoveUp",
	                      MemberCaller<GlobalCameraManager, &GlobalCameraManager::freelookMoveUpKeyUp>(*this),
	                      MemberCaller<GlobalCameraManager, &GlobalCameraManager::freelookMoveUpKeyDown>(*this));
	GlobalEventManager().addKeyEvent("CameraFreeMoveDown",
	                      MemberCaller<GlobalCameraManager, &GlobalCameraManager::freelookMoveDownKeyUp>(*this),
	                      MemberCaller<GlobalCameraManager, &GlobalCameraManager::freelookMoveDownKeyDown>(*this));
	
	CamWnd::captureStates();
}

void GlobalCameraManager::destroy() {
	// Release windows, destroy the CamWnd classes
	_cameras.clear();
	
	// Release shaders
	CamWnd::releaseStates();
}

CamWndPtr GlobalCameraManager::getActiveCamWnd() {
	// Sanity check in debug builds
	assert(_cameras.find(_activeCam) != _cameras.end());

	return (_activeCam != -1) ? _cameras[_activeCam] : CamWndPtr();
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

void GlobalCameraManager::removeCamWnd(std::size_t id) {
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
FloatingCamWndPtr GlobalCameraManager::createFloatingWindow() {
	// Create a new floating camera window widget and return it
	FloatingCamWndPtr cam(new FloatingCamWnd(_parent));

	_cameras.insert(CamWndMap::value_type(cam->getId(), cam));

	if (_activeCam == -1) {
		_activeCam = cam->getId();
	}

	return cam;
}

void GlobalCameraManager::resetCameraAngles() {
	CamWndPtr camWnd = getActiveCamWnd();

	if (camWnd != NULL) {
		Vector3 angles;
		angles[CAMERA_ROLL] = angles[CAMERA_PITCH] = 0;
		angles[CAMERA_YAW] = 22.5 * floor((camWnd->getCameraAngles()[CAMERA_YAW]+11)/22.5);
		camWnd->setCameraAngles(angles);
	}
}

void GlobalCameraManager::increaseCameraSpeed() {

	int movementSpeed = GlobalRegistry().getInt(RKEY_MOVEMENT_SPEED);
	movementSpeed *= 2;
	
	if (movementSpeed > MAX_CAMERA_SPEED){
		movementSpeed = MAX_CAMERA_SPEED;
	}
	
	GlobalRegistry().setInt(RKEY_MOVEMENT_SPEED, movementSpeed);
}

void GlobalCameraManager::decreaseCameraSpeed() {

	int movementSpeed = GlobalRegistry().getInt(RKEY_MOVEMENT_SPEED);
	movementSpeed /= 2;
	
	if (movementSpeed < 1){
		movementSpeed = 1;
	}
	
	GlobalRegistry().setInt(RKEY_MOVEMENT_SPEED, movementSpeed);
}

void GlobalCameraManager::benchmark() {
	CamWndPtr camWnd = getActiveCamWnd();

	if (camWnd != NULL) {
		camWnd->benchmark();
	}
}

void GlobalCameraManager::update() {
	// Issue the update call to all cameras
	for (CamWndMap::const_iterator i = _cameras.begin(); i != _cameras.end(); ++i) {
		i->second->update();
	}
}

// Set the global parent window
void GlobalCameraManager::setParent(GtkWindow* parent) {
	_parent = parent;
}

void GlobalCameraManager::changeFloorUp() {
	CamWndPtr camWnd = getActiveCamWnd();
	if (camWnd == NULL) return;

	// Pass the call to the currently active CamWnd 
	camWnd->changeFloor(true);
}

void GlobalCameraManager::changeFloorDown() {
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

void GlobalCameraManager::toggleLightingMode() {
	getCameraSettings()->toggleLightingMode();
}

void GlobalCameraManager::cubicScaleIn() {
	CamWndPtr camWnd = getActiveCamWnd();
	if (camWnd == NULL) return;

	camWnd->cubicScaleIn();
}

void GlobalCameraManager::cubicScaleOut() {
	CamWndPtr camWnd = getActiveCamWnd();
	if (camWnd == NULL) return;

	camWnd->cubicScaleOut();
}

void GlobalCameraManager::focusCamera(const Vector3& point, const Vector3& angles) {
	CamWndPtr camWnd = getActiveCamWnd();
	if (camWnd == NULL) return;
	
	camWnd->setCameraOrigin(point);
	camWnd->setCameraAngles(angles);
}

// --------------- Keyboard movement methods ------------------------------------------

void GlobalCameraManager::freelookMoveForwardKeyUp() {
	CamWndPtr camWnd = getActiveCamWnd();
	if (camWnd == NULL) return;

	camWnd->getCamera().clearMovementFlags(MOVE_FORWARD);
}

void GlobalCameraManager::freelookMoveForwardKeyDown() {
	CamWndPtr camWnd = getActiveCamWnd();
	if (camWnd == NULL) return;

	camWnd->getCamera().setMovementFlags(MOVE_FORWARD);
}

void GlobalCameraManager::freelookMoveBackKeyUp() {
	CamWndPtr camWnd = getActiveCamWnd();
	if (camWnd == NULL) return;

	camWnd->getCamera().clearMovementFlags(MOVE_BACK);
}

void GlobalCameraManager::freelookMoveBackKeyDown() {
	CamWndPtr camWnd = getActiveCamWnd();
	if (camWnd == NULL) return;

	camWnd->getCamera().setMovementFlags(MOVE_BACK);
}

void GlobalCameraManager::freelookMoveLeftKeyUp() {
	CamWndPtr camWnd = getActiveCamWnd();
	if (camWnd == NULL) return;

	camWnd->getCamera().clearMovementFlags(MOVE_STRAFELEFT);
}

void GlobalCameraManager::freelookMoveLeftKeyDown() {
	CamWndPtr camWnd = getActiveCamWnd();
	if (camWnd == NULL) return;

	camWnd->getCamera().setMovementFlags(MOVE_STRAFELEFT);
}

void GlobalCameraManager::freelookMoveRightKeyUp() {
	CamWndPtr camWnd = getActiveCamWnd();
	if (camWnd == NULL) return;

	camWnd->getCamera().clearMovementFlags(MOVE_STRAFERIGHT);
}

void GlobalCameraManager::freelookMoveRightKeyDown() {
	CamWndPtr camWnd = getActiveCamWnd();
	if (camWnd == NULL) return;

	camWnd->getCamera().setMovementFlags(MOVE_STRAFERIGHT);
}

void GlobalCameraManager::freelookMoveUpKeyUp() {
	CamWndPtr camWnd = getActiveCamWnd();
	if (camWnd == NULL) return;

	camWnd->getCamera().clearMovementFlags(MOVE_UP);
}

void GlobalCameraManager::freelookMoveUpKeyDown() {
	CamWndPtr camWnd = getActiveCamWnd();
	if (camWnd == NULL) return;

	camWnd->getCamera().setMovementFlags(MOVE_UP);
}

void GlobalCameraManager::freelookMoveDownKeyUp() {
	CamWndPtr camWnd = getActiveCamWnd();
	if (camWnd == NULL) return;

	camWnd->getCamera().clearMovementFlags(MOVE_DOWN);
}

void GlobalCameraManager::freelookMoveDownKeyDown() {
	CamWndPtr camWnd = getActiveCamWnd();
	if (camWnd == NULL) return;

	camWnd->getCamera().setMovementFlags(MOVE_DOWN);
}

void GlobalCameraManager::moveForwardDiscrete() {
	CamWndPtr camWnd = getActiveCamWnd();
	if (camWnd == NULL) return;

	camWnd->getCamera().moveForwardDiscrete();
}

void GlobalCameraManager::moveBackDiscrete() {
	CamWndPtr camWnd = getActiveCamWnd();
	if (camWnd == NULL) return;

	camWnd->getCamera().moveBackDiscrete();
}

void GlobalCameraManager::moveUpDiscrete() {
	CamWndPtr camWnd = getActiveCamWnd();
	if (camWnd == NULL) return;

	camWnd->getCamera().moveUpDiscrete();
}

void GlobalCameraManager::moveDownDiscrete() {
	CamWndPtr camWnd = getActiveCamWnd();
	if (camWnd == NULL) return;

	camWnd->getCamera().moveDownDiscrete();
}

void GlobalCameraManager::moveLeftDiscrete() {
	CamWndPtr camWnd = getActiveCamWnd();
	if (camWnd == NULL) return;

	camWnd->getCamera().moveLeftDiscrete();
}

void GlobalCameraManager::moveRightDiscrete() {
	CamWndPtr camWnd = getActiveCamWnd();
	if (camWnd == NULL) return;

	camWnd->getCamera().moveRightDiscrete();
}

void GlobalCameraManager::rotateLeftDiscrete() {
	CamWndPtr camWnd = getActiveCamWnd();
	if (camWnd == NULL) return;

	camWnd->getCamera().rotateLeftDiscrete();
}

void GlobalCameraManager::rotateRightDiscrete() {
	CamWndPtr camWnd = getActiveCamWnd();
	if (camWnd == NULL) return;

	camWnd->getCamera().rotateRightDiscrete();
}

void GlobalCameraManager::pitchUpDiscrete() {
	CamWndPtr camWnd = getActiveCamWnd();
	if (camWnd == NULL) return;

	camWnd->getCamera().pitchUpDiscrete();
}

void GlobalCameraManager::pitchDownDiscrete() {
	CamWndPtr camWnd = getActiveCamWnd();
	if (camWnd == NULL) return;

	camWnd->getCamera().pitchDownDiscrete();
}

// RegisterableModule implementation
const std::string& GlobalCameraManager::getName() const {
	static std::string _name(MODULE_CAMERA);
	return _name;
}

const StringSet& GlobalCameraManager::getDependencies() const {
	static StringSet _dependencies;
	
	if (_dependencies.empty()) {
		_dependencies.insert(MODULE_XMLREGISTRY);
		_dependencies.insert(MODULE_EVENTMANAGER);
		_dependencies.insert(MODULE_SHADERCACHE);
	}
	
	return _dependencies;
}

void GlobalCameraManager::initialiseModule(const ApplicationContext& ctx) {
	globalOutputStream() << "GlobalCameraManager::initialiseModule called.\n";
}

// Define the static SelectionSystem module
module::StaticModule<GlobalCameraManager> cameraModule;

// ------------------------------------------------------------------------------------

// The accessor function to the GlobalCameraManager instance 
GlobalCameraManager& GlobalCamera() {
	return *cameraModule.getModule();
}
