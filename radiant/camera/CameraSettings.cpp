#include "CameraSettings.h"

#include "GlobalCamera.h"
#include "renderstate.h"

#include <boost/lexical_cast.hpp>

CameraSettings::CameraSettings() : 
	_callbackActive(false),
	_movementSpeed(GlobalRegistry().getInt(RKEY_MOVEMENT_SPEED)),
	_angleSpeed(GlobalRegistry().getInt(RKEY_ROTATION_SPEED)),
	_invertMouseVerticalAxis(GlobalRegistry().get(RKEY_INVERT_MOUSE_VERTICAL_AXIS) == "1"),
	_discreteMovement(GlobalRegistry().get(RKEY_DISCRETE_MOVEMENT) == "1"),
	_cameraDrawMode(drawTexture),
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
	
	// Initialise the draw mode from the integer value stored in the registry
	importDrawMode(GlobalRegistry().getInt(RKEY_DRAWMODE));
	
	// Connect self to the according registry keys
	GlobalRegistry().addKeyObserver(this, RKEY_MOVEMENT_SPEED);
	GlobalRegistry().addKeyObserver(this, RKEY_ROTATION_SPEED);
	GlobalRegistry().addKeyObserver(this, RKEY_INVERT_MOUSE_VERTICAL_AXIS);
	GlobalRegistry().addKeyObserver(this, RKEY_DISCRETE_MOVEMENT);
	GlobalRegistry().addKeyObserver(this, RKEY_ENABLE_FARCLIP);
	GlobalRegistry().addKeyObserver(this, RKEY_DRAWMODE);
	GlobalRegistry().addKeyObserver(this, RKEY_SOLID_SELECTION_BOXES);
}

void CameraSettings::farClipExport(const BoolImportCallback& importCallback) {
	importCallback((GlobalRegistry().get(RKEY_ENABLE_FARCLIP) == "1"));
}

void CameraSettings::importDrawMode(const int mode) {
	switch (mode) {
		case 0: 
			_cameraDrawMode = drawWire;
			break;
		case 1: 
			_cameraDrawMode = drawSolid;
			break;
		case 2: 
			_cameraDrawMode = drawTexture;
			break;
		case 3: 
			_cameraDrawMode = drawLighting;
			break;
		default: 
			_cameraDrawMode = drawTexture;
	}

	// Notify the shadercache that the lighting mode is enabled/disabled
	ShaderCache_setBumpEnabled(_cameraDrawMode == drawLighting);
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
		_solidSelectionBoxes = (GlobalRegistry().get(RKEY_SOLID_SELECTION_BOXES) == "1");
		
		// Determine the draw mode represented by the integer registry value
		importDrawMode(GlobalRegistry().getInt(RKEY_DRAWMODE));
		
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
			
			// Call the update method for the farclip plane
			_farClipItem.update();
			GlobalCamera().getCamWnd()->getCamera().updateProjection();
			
			// Call the update method in case the render mode has changed
			GlobalCamera().getCamWnd()->update();
		}
	}
	_callbackActive = false; 
}

CameraDrawMode CameraSettings::getMode() const {
	return _cameraDrawMode;
}

void CameraSettings::setMode(const CameraDrawMode& mode) {
	// Write the value into the registry, this should trigger the keyChanged() callback that in turn calls the update functions
	GlobalRegistry().setInt(RKEY_DRAWMODE, static_cast<int>(mode));
}

void CameraSettings::toggleLightingMode() {
	// switch between textured and lighting mode
	setMode((_cameraDrawMode == drawLighting) ? drawTexture : drawLighting);
}

bool CameraSettings::farClipEnabled() const {
	return _farClipEnabled;
}

bool CameraSettings::solidSelectionBoxes() const {
	return _solidSelectionBoxes;
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
	// Write the value into the registry, the keyChanged() method is automatically triggered hereby
	GlobalRegistry().set(RKEY_ENABLE_FARCLIP, farClipEnabled ? "1" : "0");
}

void CameraSettings::toggleFarClip() {
	setFarClip(!_farClipEnabled);
}

// ---------------------------------------------------------------------------------

CameraSettings* getCameraSettings() {
	static CameraSettings _cameraSettings;
	return &_cameraSettings;
}
