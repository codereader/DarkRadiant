#include "CameraSettings.h"

#include "GlobalCamera.h"
#include "camwindow.h"

#include <boost/lexical_cast.hpp>

CameraSettings::CameraSettings() : 
	_callbackActive(false),
	_movementSpeed(GlobalRegistry().getInt("user/ui/camera/movementSpeed")),
	_invertMouseVerticalAxis(GlobalRegistry().get("user/ui/camera/invertMouseVerticalAxis") == "1"),
	_cubicScale(GlobalRegistry().getInt("user/ui/camera/cubicScale"))
{
	initDiscreteMovement();
	
	// Constrain the cubic scale to a fixed value
	if (_cubicScale > MAX_CUBIC_SCALE) {
		_cubicScale = MAX_CUBIC_SCALE;
	}
}

void CameraSettings::keyChanged() {
	// Check for iterative loops
	if (_callbackActive) {
		return;
	}
	else {
		_callbackActive = true;
		
		// Call the setFarClip method to update the current far clip state
		Camera_SetFarClip( GlobalCamera().getCamWnd()->getCamera().farClipEnabled() );
		
		// Load the values from the registry
		_movementSpeed = GlobalRegistry().getInt("user/ui/camera/movementSpeed");
		_invertMouseVerticalAxis = (GlobalRegistry().get("user/ui/camera/invertMouseVerticalAxis") == "1");
		
		// Check if a global camwindow is set
		if (GlobalCamera().getCamWnd() != 0) {
			// Disable discrete movement if it was active before
			if (_discreteMovement == true) {
				GlobalCamera().getCamWnd()->moveDiscreteDisable();
			}
			else {
				GlobalCamera().getCamWnd()->moveDisable();
			}
		
			// Check the value and take the according actions
			initDiscreteMovement();
		}
		
	}
	_callbackActive = false; 
}

void CameraSettings::initDiscreteMovement() {
	_discreteMovement = (GlobalRegistry().get("user/ui/camera/discreteMovement") == "1");
	
	// If it is activated now, take the appropriate action
	if (_discreteMovement) {
		GlobalCamera().getCamWnd()->moveDiscreteEnable();
	}
	else {
		GlobalCamera().getCamWnd()->moveEnable();
	}
}

int& CameraSettings::getCubicScale() {
	return _cubicScale;
}

void CameraSettings::setCubicScale(const int& scale) {
	// Update the internal value
	_cubicScale = scale;
	
	std::string scaleStr;
	try {
		scaleStr = boost::lexical_cast<std::string>(_cubicScale);
	}
	catch (boost::bad_lexical_cast e) {
		scaleStr = "0";
	}
	
	GlobalRegistry().set("user/ui/camera/cubicScale", scaleStr);
	
	// Constrain the value to [1..MAX_CUBIC_SCALE]
	if (_cubicScale>MAX_CUBIC_SCALE) {
		_cubicScale = MAX_CUBIC_SCALE;
	}
	
	if (_cubicScale < 1) {
		_cubicScale = 1;
	}
}

CameraSettings* getCameraSettings() {
	static CameraSettings _cameraSettings;
	return &_cameraSettings;
}
