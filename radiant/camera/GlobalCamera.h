#ifndef GLOBALCAMERA_H_
#define GLOBALCAMERA_H_

#include <list>
#include "icamera.h"

#include "CamWnd.h"
#include "CameraObserver.h"

/* FORWARD DECLS */
namespace gtkutil {
	class PersistentTransientWindow;
	typedef boost::shared_ptr<PersistentTransientWindow> 
			PersistentTransientWindowPtr;
}

/* greebo: This is the gateway class to access the currently active CamWindow
 * 
 * This class provides an interface for creating and deleting CamWnd instances
 * as well as some methods that are passed to the currently active CamWnd, like
 * resetCameraAngles() or lookThroughSelected().
 * 
 * The active CamWnd class is referenced by the _camWnd member pointer. */

class GlobalCameraManager :
	public ICamera
{
	// The currently active camera window
	CamWndPtr _camWnd;
	
	// The PersistentTransientWindow containing the CamWnd's GTK widget. This
	// may not be set, since the splitpane view styles do not have the camera
	// in its own window
	gtkutil::PersistentTransientWindowPtr _floatingCamWindow;
	
	// The parent widget for the camera window (this should be the main frame)
	GtkWindow* _parent;
	
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
	
	// Saves the current state of the camera window to the registry
	void saveCamWndState();

	/**
	 * Specifies the parent window which should be used for the CamWnd.
	 */
	void setParent(GtkWindow* parent);
	
	/**
	 * Get the single CamWnd instance, creating it if necessary. A parent window 
	 * must have been set with setParent() before the instance can be created. 
	 */
	CamWndPtr getCamWnd();
	
	/**
	 * Get a PersistentFloatingWindow containing the CamWnd widget, creating
	 * it if necessary.
	 */
	gtkutil::PersistentTransientWindowPtr getFloatingWindow();
	
	// Resets the camera angles of the currently active Camera
	void resetCameraAngles();

	/** greebo: Sets the camera to the given point/angle.
	 */
	void focusCamera(const Vector3& point, const Vector3& angles);

	// Toggles between lighting and solid rendering mode (passes the call to the CameraSettings class)
	void toggleLightingMode();

	// Toggles the maximised/restored state of the camera window
	void toggleFullscreen();
	
	// Increases/decreases the far clip plane distance (passes the call to CamWnd)
	void cubicScaleIn();
	void cubicScaleOut();

	// Change the floor up/down, passes the call on to the CamWnd class
	void changeFloorUp();
	void changeFloorDown();

	// angua: increases and decreases the movement speed of the camera
	void increaseCameraSpeed();
	void decreaseCameraSpeed();
	
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
	
public:
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
	
	// RegisterableModule implementation
	virtual const std::string& getName() const;
	virtual const StringSet& getDependencies() const;
	virtual void initialiseModule(const ApplicationContext& ctx);
	
}; // class GlobalCameraManager

// The accessor function that contains the static instance of the GlobalCameraManager class 
GlobalCameraManager& GlobalCamera();

#endif /*GLOBALCAMERA_H_*/
