#ifndef CAMERASETTINGS_H_
#define CAMERASETTINGS_H_

#include <string>
#include "iregistry.h"

/* greebo: This is the home of all the camera settings. As this class derives
 * from a RegistryKeyObserver, it can be connected to the according registry keys
 * and gets notified if any of the observed keys are changed.*/

namespace {
	const int MAX_CUBIC_SCALE = 23;
	const int MAX_CAMERA_SPEED = 300;
	
	const std::string RKEY_CAMERA_ROOT = "user/ui/camera"; 
	const std::string RKEY_MOVEMENT_SPEED = RKEY_CAMERA_ROOT + "/movementSpeed";
	const std::string RKEY_ROTATION_SPEED = RKEY_CAMERA_ROOT + "/rotationSpeed";
	const std::string RKEY_INVERT_MOUSE_VERTICAL_AXIS = RKEY_CAMERA_ROOT + "/invertMouseVerticalAxis";
	const std::string RKEY_DISCRETE_MOVEMENT = RKEY_CAMERA_ROOT + "/discreteMovement";
	const std::string RKEY_CUBIC_SCALE = RKEY_CAMERA_ROOT + "/cubicScale";
	const std::string RKEY_ENABLE_FARCLIP = RKEY_CAMERA_ROOT + "/enableCubicClipping";
	const std::string RKEY_DRAWMODE = RKEY_CAMERA_ROOT + "/drawMode";
	const std::string RKEY_SOLID_SELECTION_BOXES = "user/ui/xyview/solidSelectionBoxes";
	const std::string RKEY_TOGGLE_FREE_MOVE = RKEY_CAMERA_ROOT + "/toggleFreeMove";
	const std::string RKEY_CAMERA_WINDOW_STATE = RKEY_CAMERA_ROOT + "/window";
}

enum CameraDrawMode {
	drawWire,
	drawSolid,
	drawTexture,
	drawLighting,
};

class CameraSettings : 
	public RegistryKeyObserver 
{
	bool _callbackActive;
	
	int _movementSpeed;
	int _angleSpeed;
	
	bool _invertMouseVerticalAxis;
	bool _discreteMovement;
	
	CameraDrawMode _cameraDrawMode;
	
	int _cubicScale;
	bool _farClipEnabled;
	bool _solidSelectionBoxes;
	// This is TRUE if the mousebutton must be held to stay in freelook mode 
	// instead of enabling it by clicking and clicking again to disable
	bool _toggleFreelook;
	
public:
	CameraSettings();

	// The callback that gets called on registry key changes
	void keyChanged(const std::string& key, const std::string& val);
	
	int movementSpeed() const;
	int angleSpeed() const;
	
	// Returns true if cubic clipping is on
	bool farClipEnabled() const;
	bool invertMouseVerticalAxis() const;
	bool discreteMovement() const;
	bool solidSelectionBoxes() const;
	bool toggleFreelook() const;
	
	// Sets/returns the draw mode (wireframe, solid, textured, lighting)
	CameraDrawMode getMode() const;
	void setMode(const CameraDrawMode& mode);
	void toggleLightingMode();
	
	// Gets/Sets the cubic scale member variable (is automatically constrained [1..MAX_CUBIC_SCALE])
	int cubicScale() const;
	void setCubicScale(const int& scale);

	// Enables/disables the cubic clipping
	void toggleFarClip();
	void setFarClip(bool farClipEnabled);

	// Adds the elements to the "camera" preference page
	void constructPreferencePage();

private:
	void importDrawMode(const int mode);
}; // class CameraSettings

CameraSettings* getCameraSettings();

#endif /*CAMERASETTINGS_H_*/
