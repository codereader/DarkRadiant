#ifndef GLOBALCAMERA_H_
#define GLOBALCAMERA_H_

#include <list>
#include "icamera.h"

#include "gtkutil/widget.h"

#include "preferences.h"
#include "CamWnd.h"
#include "CameraObserver.h"

/* greebo: This is the gateway class to access the currently active CamWindow
 * 
 * This class provides an interface for creating and deleting CamWnd instances
 * as well as some methods that are passed to the currently active CamWnd, like
 * resetCameraAngles() or lookThroughSelected().
 * 
 * The active CamWnd class is referenced by the _camWnd member pointer. */

class GlobalCameraManager {
	
	// The currently active camera window
	CamWnd* _camWnd;
	
	CameraModel* _cameraModel;
	ToggleShown _cameraShown;
	
	// The connected callbacks (get invoked when movedNotify() is called)	
	CameraObserverList _cameraObservers;
	
public:

	// Constructor
	GlobalCameraManager();
	
	// greebo: The construct method registers all the commands and preferences 
	// plus initialises the shader states of the camera window. 
	void construct();
	
	// Activates the shortcuts for some commands
	void registerShortcuts();
	
	// This releases the shader states of the CamWnd class
	void destroy();
	
	// Creates a new CamWnd class and returns the according pointer 
	CamWnd* newCamWnd();
	
	// Specifies the parent window of the given CamWnd
	void setParent(CamWnd* camwnd, GtkWindow* parent);
	
	// Frees the created CamWnd class
	void deleteCamWnd(CamWnd* camWnd);
	
	// Retrieves/Sets the pointer to the current CamWnd
	CamWnd* getCamWnd();
	void setCamWnd(CamWnd* camWnd);
	
	// Shows/hides the currently active camera window
	void toggleCamera();
	
	// Return the ToggleShown class (needed to connect the GlobalToggle command)
	ToggleShown& getToggleShown();
	
	// Resets the camera angles of the currently active Camera
	void resetCameraAngles();

	// Toggles between lighting and solid rendering mode (passes the call to the CameraSettings class)
	void toggleLightingMode();
	
	// Increases/decreases the far clip plane distance (passes the call to CamWnd)
	void cubicScaleIn();
	void cubicScaleOut();

	// Change the floor up/down, passes the call on to the CamWnd class
	void changeFloorUp();
	void changeFloorDown();

	/* greebo: Tries to get a CameraModel from the most recently selected instance
	 * Note: Currently NO instances are supporting the InstanceTypeCast onto a
	 * CameraModel, so actually these functions don't do anything. I'll leave them
	 * where they are, they should work in principle... */
	void lookThroughSelected();
	void lookThroughCamera();
	
	// greebo: This measures the rendering time for a full 360 degrees turn of the camera
	// Note: unused at the moment
	void benchmark();
	
	void update();
	
	// Add a "CameraMoved" callback to the signal member
	void addCameraObserver(CameraObserver* observer);
	
	// Notify the attached "CameraMoved" callbacks
	void movedNotify();
	
	// Movement commands (the calls are passed on to the Camera class)
	void moveForwardDiscrete();
	void moveBackDiscrete();
	void moveUpDiscrete();
	void moveDownDiscrete();
	void moveLeftDiscrete();
	void moveRightDiscrete();
	void rotateLeftDiscrete();
	void rotateRightDiscrete();
	void pitchUpDiscrete();
	void pitchDownDiscrete();

private:

	// This constructs the preference page "Camera"
	void registerPreferences();
	
	// This gets called by the preference dialog to add the "camera" page to the given <group>
	void constructPreferencePage(PreferenceGroup& group);
	typedef MemberCaller1<GlobalCameraManager, PreferenceGroup&, &GlobalCameraManager::constructPreferencePage> PreferencePageConstructor;
	
	// This actually adds the settings widgets to the preference page created by constructPreferencePage() 
	void constructPreferences(PrefPage* page);
	
}; // class GlobalCameraManager

// The accessor function that contains the static instance of the GlobalCameraManager class 
GlobalCameraManager& GlobalCamera();

#endif /*GLOBALCAMERA_H_*/
