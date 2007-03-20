#include "GlobalCamera.h"

#include "ieventmanager.h"
#include "iselection.h"
#include "gdk/gdkkeysyms.h"
#include "xmlutil/Node.h"

#include "Camera.h"
#include "CameraSettings.h"

// Constructor
GlobalCameraManager::GlobalCameraManager() : 
	_camWnd(NULL),
	_cameraModel(NULL)
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

	GlobalEventManager().addCommand("LookThroughSelected", MemberCaller<GlobalCameraManager, &GlobalCameraManager::lookThroughSelected>(*this));
	GlobalEventManager().addCommand("LookThroughCamera", MemberCaller<GlobalCameraManager, &GlobalCameraManager::lookThroughCamera>(*this));

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
	CamWnd::releaseStates();
}

// Creates a new CamWnd class on the heap and returns the according pointer 
CamWnd* GlobalCameraManager::newCamWnd() {
	return new CamWnd;
}

// Frees the specified CamWnd class
void GlobalCameraManager::deleteCamWnd(CamWnd* camWnd) {
	if (camWnd != NULL) {
		delete camWnd;
	}
}

// Retrieves the pointer to the current CamWnd
CamWnd* GlobalCameraManager::getCamWnd() {
	return _camWnd;
}

void GlobalCameraManager::setCamWnd(CamWnd* camWnd) {
	_camWnd = camWnd;
}

void GlobalCameraManager::resetCameraAngles() {
	if (_camWnd != NULL) {
		Vector3 angles;
		angles[CAMERA_ROLL] = angles[CAMERA_PITCH] = 0;
		angles[CAMERA_YAW] = static_cast<float>(22.5 * floor((_camWnd->getCameraAngles()[CAMERA_YAW]+11)/22.5));
		_camWnd->setCameraAngles(angles);
	}
}

void GlobalCameraManager::lookThroughCamera() {
	if (_cameraModel != 0) {
		_camWnd->addHandlersMove();
		
		_cameraModel->setCameraView(NULL, Callback());
		_cameraModel = NULL;
		
		_camWnd->getCamera().updateModelview();
		_camWnd->getCamera().updateProjection();
		_camWnd->update();
	}
}

void GlobalCameraManager::lookThroughSelected() {
	if (_cameraModel != 0) {
		lookThroughCamera();
	}

	if (GlobalSelectionSystem().countSelected() != 0) {
		// Get the instance that was most recently selected
		scene::Instance& instance = GlobalSelectionSystem().ultimateSelected();
		
		CameraModel* cameraModel = Instance_getCameraModel(instance);
		
		if (cameraModel != 0) {
			_camWnd->removeHandlersMove();
			_cameraModel = cameraModel;
			_cameraModel->setCameraView(_camWnd->getCameraView(), MemberCaller<GlobalCameraManager, &GlobalCameraManager::lookThroughCamera>(*this));
		}
	}
}

void GlobalCameraManager::benchmark() {
	_camWnd->benchmark();
}

void GlobalCameraManager::update() {
	_camWnd->update();
}

void GlobalCameraManager::setParent(CamWnd* camwnd, GtkWindow* parent) {
	camwnd->setParent(parent);
	
	IEventPtr event = GlobalEventManager().findEvent("ToggleCamera");
	
	if (!event->empty()) {
		event->connectWidget(GTK_WIDGET(camwnd->getParent()));
		event->updateWidgets();
	}
	else {
		globalErrorStream() << "Could not connect ToggleCamera event\n";
	}
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

void GlobalCameraManager::restoreCamWndState(GtkWindow* window) {
	xml::NodeList windowStateList = GlobalRegistry().findXPath(RKEY_CAMERA_WINDOW_STATE);
	
	if (windowStateList.size() > 0) {
		xml::Node windowState = windowStateList[0];
	
		if (window != NULL) {
			_windowPosition.loadFromNode(windowState);
			_windowPosition.connect(window);
		}
	}
	else {
		// Nothing found, nothing to do
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

// ------------------------------------------------------------------------------------

// The accessor function to the GlobalCameraManager instance 
GlobalCameraManager& GlobalCamera() {
	static GlobalCameraManager _cameraManager;
	return _cameraManager;
}
