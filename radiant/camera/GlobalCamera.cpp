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

// Constructor
GlobalCameraManager::GlobalCameraManager() :
	_camWnd(),
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
	
	// Release windows, destroy the CamWnd class before destroying the PersistentWindow
	_camWnd = CamWndPtr();
	// The TransientWindow destructor will destroy the window widget
	_floatingCamWindow = gtkutil::PersistentTransientWindowPtr();
	
	// Release shaders
	CamWnd::releaseStates();
}

// Construct and return the CamWnd instance
CamWndPtr GlobalCameraManager::getCamWnd() {
	if (!_camWnd) {
		// Create and initialise the CamWnd
		_camWnd = CamWndPtr(new CamWnd());
	}
	return _camWnd;
}

// Construct/return a floating window containing the CamWnd widget
gtkutil::PersistentTransientWindowPtr GlobalCameraManager::getFloatingWindow() {
	if (!_floatingCamWindow) {
		
		// Create the floating window
		_floatingCamWindow = gtkutil::PersistentTransientWindowPtr(
			new gtkutil::PersistentTransientWindow("Camera", _parent, true)
		);
		
		// Pack in the CamWnd widget
		CamWndPtr camWnd = getCamWnd();
		camWnd->setContainer(GTK_WINDOW(_floatingCamWindow->getWindow()));
		gtk_container_add(
			GTK_CONTAINER(_floatingCamWindow->getWindow()),
			gtkutil::FramedWidget(_camWnd->getWidget())
		);
		
		gtk_window_set_type_hint(
			GTK_WINDOW(_floatingCamWindow->getWindow()), GDK_WINDOW_TYPE_HINT_NORMAL
	    );
		
		// Restore the window position from the registry if possible
		xml::NodeList windowStateList = 
			GlobalRegistry().findXPath(RKEY_CAMERA_WINDOW_STATE);
		
		if (!windowStateList.empty()) {
			_windowPosition.loadFromNode(windowStateList[0]);
			_windowPosition.connect(
				GTK_WINDOW(_floatingCamWindow->getWindow())
			);
		}

		// Connect up the toggle camera event
		IEventPtr event = GlobalEventManager().findEvent("ToggleCamera");
		if (!event->empty()) {
			event->connectWidget(_floatingCamWindow->getWindow());
			event->updateWidgets();
		}
		else {
			globalErrorStream() << "Could not connect ToggleCamera event\n";
		}

		// Add the toggle max/min command for floating windows
		GlobalEventManager().addCommand("ToggleCameraFullScreen", MemberCaller<GlobalCameraManager, &GlobalCameraManager::toggleFullscreen>(*this));
	}
	return _floatingCamWindow;
}

void GlobalCameraManager::toggleFullscreen() {
	gtkutil::PersistentTransientWindowPtr window = getFloatingWindow();

	if (window == NULL) {
		return;
	}

	window->toggleFullscreen();
}

void GlobalCameraManager::resetCameraAngles() {
	if (_camWnd != NULL) {
		Vector3 angles;
		angles[CAMERA_ROLL] = angles[CAMERA_PITCH] = 0;
		angles[CAMERA_YAW] = 22.5 * floor((_camWnd->getCameraAngles()[CAMERA_YAW]+11)/22.5);
		_camWnd->setCameraAngles(angles);
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
	_camWnd->benchmark();
}

void GlobalCameraManager::update() {
	_camWnd->update();
}

// Set the global parent window
void GlobalCameraManager::setParent(GtkWindow* parent) {
	_parent = parent;
}

void GlobalCameraManager::changeFloorUp() {
	// Pass the call to the currently active CamWnd 
	_camWnd->changeFloor(true);
}

void GlobalCameraManager::changeFloorDown() {
	// Pass the call to the currently active CamWnd 
	_camWnd->changeFloor(false);
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
	_camWnd->cubicScaleIn();
}

void GlobalCameraManager::cubicScaleOut() {
	_camWnd->cubicScaleOut();
}

void GlobalCameraManager::focusCamera(const Vector3& point, const Vector3& angles) {
	
	if (_camWnd != NULL) {
		_camWnd->setCameraOrigin(point);
		_camWnd->setCameraAngles(angles);
	}
}

void GlobalCameraManager::saveCamWndState() {
	// Delete all the current window states from the registry  
	GlobalRegistry().deleteXPath(RKEY_CAMERA_WINDOW_STATE);
	
	// Create a new node
	xml::Node node(GlobalRegistry().createKey(RKEY_CAMERA_WINDOW_STATE));
	
	_windowPosition.saveToNode(node);
}

// --------------- Keyboard movement methods ------------------------------------------

void GlobalCameraManager::freelookMoveForwardKeyUp() {
	_camWnd->getCamera().clearMovementFlags(MOVE_FORWARD);
}

void GlobalCameraManager::freelookMoveForwardKeyDown() {
	_camWnd->getCamera().setMovementFlags(MOVE_FORWARD);
}

void GlobalCameraManager::freelookMoveBackKeyUp() {
	_camWnd->getCamera().clearMovementFlags(MOVE_BACK);
}

void GlobalCameraManager::freelookMoveBackKeyDown() {
	_camWnd->getCamera().setMovementFlags(MOVE_BACK);
}

void GlobalCameraManager::freelookMoveLeftKeyUp() {
	_camWnd->getCamera().clearMovementFlags(MOVE_STRAFELEFT);
}

void GlobalCameraManager::freelookMoveLeftKeyDown() {
	_camWnd->getCamera().setMovementFlags(MOVE_STRAFELEFT);
}

void GlobalCameraManager::freelookMoveRightKeyUp() {
	_camWnd->getCamera().clearMovementFlags(MOVE_STRAFERIGHT);
}

void GlobalCameraManager::freelookMoveRightKeyDown() {
	_camWnd->getCamera().setMovementFlags(MOVE_STRAFERIGHT);
}

void GlobalCameraManager::freelookMoveUpKeyUp() {
	_camWnd->getCamera().clearMovementFlags(MOVE_UP);
}

void GlobalCameraManager::freelookMoveUpKeyDown() {
	_camWnd->getCamera().setMovementFlags(MOVE_UP);
}

void GlobalCameraManager::freelookMoveDownKeyUp() {
	_camWnd->getCamera().clearMovementFlags(MOVE_DOWN);
}

void GlobalCameraManager::freelookMoveDownKeyDown() {
	_camWnd->getCamera().setMovementFlags(MOVE_DOWN);
}

void GlobalCameraManager::moveForwardDiscrete() {
	_camWnd->getCamera().moveForwardDiscrete();
}

void GlobalCameraManager::moveBackDiscrete() {
	_camWnd->getCamera().moveBackDiscrete();
}

void GlobalCameraManager::moveUpDiscrete() {
	_camWnd->getCamera().moveUpDiscrete();
}

void GlobalCameraManager::moveDownDiscrete() {
	_camWnd->getCamera().moveDownDiscrete();
}

void GlobalCameraManager::moveLeftDiscrete() {
	_camWnd->getCamera().moveLeftDiscrete();
}

void GlobalCameraManager::moveRightDiscrete() {
	_camWnd->getCamera().moveRightDiscrete();
}

void GlobalCameraManager::rotateLeftDiscrete() {
	_camWnd->getCamera().rotateLeftDiscrete();
}

void GlobalCameraManager::rotateRightDiscrete() {
	_camWnd->getCamera().rotateRightDiscrete();
}

void GlobalCameraManager::pitchUpDiscrete() {
	_camWnd->getCamera().pitchUpDiscrete();
}

void GlobalCameraManager::pitchDownDiscrete() {
	_camWnd->getCamera().pitchDownDiscrete();
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
