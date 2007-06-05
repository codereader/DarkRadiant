#ifndef GLOBALCAMERA_H_
#define GLOBALCAMERA_H_

#include <list>
#include "icamera.h"

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
		
	// The connected callbacks (get invoked when movedNotify() is called)	
	CameraObserverList _cameraObservers;
	
	// The window position tracker
	gtkutil::WindowPosition _windowPosition;
	
public:

	// Constructor
	GlobalCameraManager();
	
	// greebo: The construct method registers all the commands and preferences 
	// plus initialises the shader states of the camera window. 
	void construct();
	
	// This releases the shader states of the CamWnd class
	void destroy();
	
	// Creates a new CamWnd class and returns the according pointer 
	CamWnd* newCamWnd();
	
	// Specifies the parent window of the given CamWnd
	void setParent(CamWnd* camwnd, GtkWindow* parent);
	
	// Frees the created CamWnd class
	void deleteCamWnd(CamWnd* camWnd);
	
	// Saves the current state of the camera window to the registry
	void saveCamWndState();

	// Restores the state of the given camera window according to the stored registry values 
	void restoreCamWndState(GtkWindow* window);
	
	// Retrieves/Sets the pointer to the current CamWnd
	CamWnd* getCamWnd();
	void setCamWnd(CamWnd* camWnd);
	
	// Resets the camera angles of the currently active Camera
	void resetCameraAngles();

	/** greebo: Sets the camera to the given point/angle.
	 */
	void focusCamera(const Vector3& point, const Vector3& angles);

	// Toggles between lighting and solid rendering mode (passes the call to the CameraSettings class)
	void toggleLightingMode();
	
	// Increases/decreases the far clip plane distance (passes the call to CamWnd)
	void cubicScaleIn();
	void cubicScaleOut();

	// Change the floor up/down, passes the call on to the CamWnd class
	void changeFloorUp();
	void changeFloorDown();

	/* greebo: Tries to get a CameraModel from the most recently selected instance
	 * Note: Currently NO instances are supporting the dynamic_cast<> onto a
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
	void removeCameraObserver(CameraObserver* observer);
	
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
	void freelookMoveForwardKeyUp();
	void freelookMoveForwardKeyDown();
	
	void freelookMoveBackKeyUp();
	void freelookMoveBackKeyDown();
	
	void freelookMoveLeftKeyUp();
	void freelookMoveLeftKeyDown();
	
	void freelookMoveRightKeyUp();
	void freelookMoveRightKeyDown();
	
	void freelookMoveUpKeyUp();
	void freelookMoveUpKeyDown();
	
	void freelookMoveDownKeyUp();
	void freelookMoveDownKeyDown();
	
}; // class GlobalCameraManager

// The accessor function that contains the static instance of the GlobalCameraManager class 
GlobalCameraManager& GlobalCamera();

#endif /*GLOBALCAMERA_H_*/
