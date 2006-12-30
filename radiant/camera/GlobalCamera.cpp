#include "GlobalCamera.h"

#include "iselection.h"
#include "gdk/gdkkeysyms.h"

#include "commands.h"

#include "CameraSettings.h"

// Constructor
GlobalCameraManager::GlobalCameraManager() : 
	_camWnd(NULL),
	_cameraModel(NULL),
	_cameraShown(true)
{}

void GlobalCameraManager::registerShortcuts() {
	toggle_add_accelerator("ToggleCubicClip");

	if (g_pGameDescription->mGameType == "doom3") {
		command_connect_accelerator("TogglePreview");
	}
}

void GlobalCameraManager::constructPreferences(PrefPage* page) {
	// Add the sliders for the movement and angle speed and connect them to the observer   
    page->appendSlider("Movement Speed (game units)", RKEY_MOVEMENT_SPEED, TRUE, 100, 50, 300, 1, 10, 10);
    page->appendSlider("Rotation Speed", RKEY_ROTATION_SPEED, TRUE, 3, 1, 180, 1, 10, 10);
    
	// Add the checkboxes and connect them with the registry key and the according observer 
	page->appendCheckBox("", "Discrete movement (non-freelook mode)", RKEY_DISCRETE_MOVEMENT);
	page->appendCheckBox("", "Enable far-clip plane (hides distant objects)", RKEY_ENABLE_FARCLIP);
	
	// Add the "inverse mouse vertical axis in free-look mode" preference
	page->appendCheckBox("", "Invert mouse vertical axis (freelook mode)", RKEY_INVERT_MOUSE_VERTICAL_AXIS);

	// Create the string list containing the render mode captions
	std::list<std::string> renderModeDescriptions;
	
	renderModeDescriptions.push_back("WireFrame");
	renderModeDescriptions.push_back("Flatshade");
	renderModeDescriptions.push_back("Textured");
	
	if (g_pGameDescription->mGameType == "doom3") {
		renderModeDescriptions.push_back("Lighting");
	}
	
	page->appendCombo("Render Mode", RKEY_DRAWMODE, renderModeDescriptions);
}

void GlobalCameraManager::constructPreferencePage(PreferenceGroup& group) {
	
	// Add a page to the given group
	PreferencesPage* page(group.createPage("Camera", "Camera View Preferences"));
	
	// Now add the preferences to the newly created page
	constructPreferences(reinterpret_cast<PrefPage*>(page));
}

void GlobalCameraManager::registerPreferences() {
	// Tell the preference dialog to add a page by using the member method construcPreferencePage()
	PreferencesDialog_addSettingsPage(PreferencePageConstructor(*this));
}

void GlobalCameraManager::construct() {
	GlobalCommands_insert("CenterView", MemberCaller<GlobalCameraManager, &GlobalCameraManager::resetCameraAngles>(*this), Accelerator(GDK_End));

	GlobalToggles_insert("ToggleCubicClip", MemberCaller<CameraSettings, &CameraSettings::toggleFarClip>(*getCameraSettings()), 
						 ToggleItem::AddCallbackCaller(getCameraSettings()->farClipItem()), Accelerator('\\', (GdkModifierType)GDK_CONTROL_MASK));

	GlobalCommands_insert("CubicClipZoomIn", MemberCaller<GlobalCameraManager, &GlobalCameraManager::cubicScaleIn>(*this), Accelerator('[', (GdkModifierType)GDK_CONTROL_MASK));
	GlobalCommands_insert("CubicClipZoomOut", MemberCaller<GlobalCameraManager, &GlobalCameraManager::cubicScaleOut>(*this), Accelerator(']', (GdkModifierType)GDK_CONTROL_MASK));

	GlobalCommands_insert("UpFloor", MemberCaller<GlobalCameraManager, &GlobalCameraManager::changeFloorUp>(*this), Accelerator(GDK_Prior));
	GlobalCommands_insert("DownFloor", MemberCaller<GlobalCameraManager, &GlobalCameraManager::changeFloorDown>(*this), Accelerator(GDK_Prior));

	GlobalToggles_insert("ToggleCamera", ToggleShown::ToggleCaller(getToggleShown()),
	                     ToggleItem::AddCallbackCaller(getToggleShown().m_item), Accelerator('C', (GdkModifierType)(GDK_SHIFT_MASK|GDK_CONTROL_MASK)));
	GlobalCommands_insert("LookThroughSelected", MemberCaller<GlobalCameraManager, &GlobalCameraManager::lookThroughSelected>(*this));
	GlobalCommands_insert("LookThroughCamera", MemberCaller<GlobalCameraManager, &GlobalCameraManager::lookThroughCamera>(*this));

	if (g_pGameDescription->mGameType == "doom3") {
		GlobalCommands_insert("TogglePreview", MemberCaller<GlobalCameraManager, &GlobalCameraManager::toggleLightingMode>(*this), Accelerator(GDK_F3));
	}
	
	// Insert movement commands
	GlobalCommands_insert("CameraForward", MemberCaller<GlobalCameraManager, &GlobalCameraManager::moveForwardDiscrete>(*this), Accelerator(GDK_Up));
	GlobalCommands_insert("CameraBack", MemberCaller<GlobalCameraManager, &GlobalCameraManager::moveBackDiscrete>(*this), Accelerator(GDK_Down));
	GlobalCommands_insert("CameraLeft", MemberCaller<GlobalCameraManager, &GlobalCameraManager::rotateLeftDiscrete>(*this), Accelerator(GDK_Left));
	GlobalCommands_insert("CameraRight", MemberCaller<GlobalCameraManager, &GlobalCameraManager::rotateRightDiscrete>(*this), Accelerator(GDK_Right));
	GlobalCommands_insert("CameraStrafeRight", MemberCaller<GlobalCameraManager, &GlobalCameraManager::moveRightDiscrete>(*this), Accelerator(GDK_period));
	GlobalCommands_insert("CameraStrafeLeft", MemberCaller<GlobalCameraManager, &GlobalCameraManager::moveLeftDiscrete>(*this), Accelerator(GDK_comma));

	GlobalCommands_insert("CameraUp", MemberCaller<GlobalCameraManager, &GlobalCameraManager::moveUpDiscrete>(*this), Accelerator('D'));
	GlobalCommands_insert("CameraDown", MemberCaller<GlobalCameraManager, &GlobalCameraManager::moveDownDiscrete>(*this), Accelerator('C'));
	GlobalCommands_insert("CameraAngleUp", MemberCaller<GlobalCameraManager, &GlobalCameraManager::pitchUpDiscrete>(*this), Accelerator('A'));
	GlobalCommands_insert("CameraAngleDown", MemberCaller<GlobalCameraManager, &GlobalCameraManager::pitchDownDiscrete>(*this), Accelerator('Z'));
	
	CamWnd::captureStates();
	
	registerPreferences();
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
