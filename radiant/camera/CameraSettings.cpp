#include "CameraSettings.h"

#include "GlobalCamera.h"
#include "camwindow.h"

#include <boost/lexical_cast.hpp>

CameraSettings::CameraSettings() : 
	_callbackActive(false),
	_movementSpeed(GlobalRegistry().getInt(RKEY_MOVEMENT_SPEED)),
	_angleSpeed(GlobalRegistry().getInt(RKEY_ROTATION_SPEED)),
	_invertMouseVerticalAxis(GlobalRegistry().get(RKEY_INVERT_MOUSE_VERTICAL_AXIS) == "1"),
	_discreteMovement(GlobalRegistry().get(RKEY_DISCRETE_MOVEMENT) == "1"),
	_cubicScale(GlobalRegistry().getInt(RKEY_CUBIC_SCALE)),
	_farClipEnabled(GlobalRegistry().get(RKEY_ENABLE_FARCLIP) == "1"),
	_farClipCaller(*this),
	_farClipCallBack(_farClipCaller),
	_farClipItem(_farClipCallBack)
{
	// Constrain the cubic scale to a fixed value
	if (_cubicScale > MAX_CUBIC_SCALE) {
		_cubicScale = MAX_CUBIC_SCALE;
	}
}

void CameraSettings::farClipExport(const BoolImportCallback& importCallback) {
	importCallback((GlobalRegistry().get(RKEY_ENABLE_FARCLIP) == "1"));
}

void CameraSettings::keyChanged() {
	// Check for iterative loops
	if (_callbackActive) {
		return;
	}
	else {
		_callbackActive = true;
		
		// Load the values from the registry
		_movementSpeed = GlobalRegistry().getInt(RKEY_MOVEMENT_SPEED);
		_angleSpeed = GlobalRegistry().getInt(RKEY_ROTATION_SPEED);
		_invertMouseVerticalAxis = (GlobalRegistry().get(RKEY_INVERT_MOUSE_VERTICAL_AXIS) == "1");
		_farClipEnabled = (GlobalRegistry().get(RKEY_ENABLE_FARCLIP) == "1");
		
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
			_discreteMovement = (GlobalRegistry().get(RKEY_DISCRETE_MOVEMENT) == "1");
			
			// If it is activated now, take the appropriate action
			if (_discreteMovement) {
				GlobalCamera().getCamWnd()->moveDiscreteEnable();
			}
			else {
				GlobalCamera().getCamWnd()->moveEnable();
			}
		}
	}
	_callbackActive = false; 
}

bool CameraSettings::farClipEnabled() const {
	return _farClipEnabled;
}

int CameraSettings::cubicScale() const {
	return _cubicScale;
}

int CameraSettings::movementSpeed() const {
	return _movementSpeed;
}

int CameraSettings::angleSpeed() const {
	return _angleSpeed;
}

bool CameraSettings::invertMouseVerticalAxis() const {
	return _invertMouseVerticalAxis;
}

bool CameraSettings::discreteMovement() const {
	return _discreteMovement;
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
	
	GlobalRegistry().set(RKEY_CUBIC_SCALE, scaleStr);
	
	// Constrain the value to [1..MAX_CUBIC_SCALE]
	if (_cubicScale>MAX_CUBIC_SCALE) {
		_cubicScale = MAX_CUBIC_SCALE;
	}
	
	if (_cubicScale < 1) {
		_cubicScale = 1;
	}
}


ToggleItem& CameraSettings::farClipItem() {
	return _farClipItem;
}

void CameraSettings::setFarClip(bool farClipEnabled) {
	// greebo: Set the bool (just in case the keyObserver isn't attached yet)
	_farClipEnabled = farClipEnabled;
	
	// write the value into the registry
	GlobalRegistry().set(RKEY_ENABLE_FARCLIP, farClipEnabled ? "1" : "0");
	
	_farClipItem.update();
	GlobalCamera().getCamWnd()->getCamera().updateProjection();
	GlobalCamera().getCamWnd()->update();
}

void CameraSettings::toggleFarClip() {
	setFarClip(!_farClipEnabled);
}

CameraSettings* getCameraSettings() {
	static CameraSettings _cameraSettings;
	return &_cameraSettings;
}
