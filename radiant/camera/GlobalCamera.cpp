#include "GlobalCamera.h"

#include "iselection.h"
#include "CameraSettings.h"

// Constructor
GlobalCameraManager::GlobalCameraManager() : 
	_camWnd(NULL),
	_cameraModel(NULL),
	_cameraShown(true)
{}

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
	
	// Connect the ToggleShown class to the parent
	_cameraShown.connect(GTK_WIDGET(camwnd->getParent()));
}

void GlobalCameraManager::toggleCamera() {
	// pass the call to the ToggleShown class
	_cameraShown.toggle();
}

ToggleShown& GlobalCameraManager::getToggleShown() {
	return _cameraShown;
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

// --------------- Keyboard movement methods ------------------------------------------

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
