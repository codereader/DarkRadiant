#ifndef GLOBALCAMERA_H_
#define GLOBALCAMERA_H_

#include <map>
#include "icamera.h"

#include "CamWnd.h"
#include "FloatingCamWnd.h"
#include "CameraObserver.h"

/**
 * greebo: This is the gateway class to access the currently active CamWindow
 * 
 * This class provides an interface for creating and deleting CamWnd instances
 * as well as some methods that are passed to the currently active CamWnd, like
 * resetCameraAngles() or lookThroughSelected().
 **/
class GlobalCameraManager :
	public ICamera
{
	typedef std::map<int, CamWndPtr> CamWndMap;
	CamWndMap _cameras;

	// The currently active camera window (-1 if no cam active)
	int _activeCam;
	
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
	
	/**
	 * Specifies the parent window which should be used for the CamWnd.
	 */
	void setParent(GtkWindow* parent);
	
	/**
	 * Returns the currently active CamWnd or NULL if none is active.
	 */
	CamWndPtr getActiveCamWnd();

	/**
	 * Create a new camera window, ready for packing into a parent widget.
	 */
	CamWndPtr createCamWnd();

	// Remove the camwnd with the given ID
	void removeCamWnd(std::size_t id);
	
	/**
	 * Get a PersistentFloatingWindow containing the CamWnd widget, creating
	 * it if necessary.
	 */
	FloatingCamWndPtr createFloatingWindow();
	
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
