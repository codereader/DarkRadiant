#include "CameraSettings.h"

#include "i18n.h"
#include "ieventmanager.h"
#include "ipreferencesystem.h"

#include "registry/registry.h"
#include "GlobalCamera.h"

#include <boost/lexical_cast.hpp>

CameraSettings::CameraSettings() :
	_callbackActive(false),
	_movementSpeed(registry::getValue<int>(RKEY_MOVEMENT_SPEED)),
	_angleSpeed(registry::getValue<int>(RKEY_ROTATION_SPEED)),
	_invertMouseVerticalAxis(registry::getValue<bool>(RKEY_INVERT_MOUSE_VERTICAL_AXIS)),
	_discreteMovement(registry::getValue<bool>(RKEY_DISCRETE_MOVEMENT)),
	_cameraDrawMode(RENDER_MODE_TEXTURED),
	_cubicScale(registry::getValue<int>(RKEY_CUBIC_SCALE)),
	_farClipEnabled(registry::getValue<bool>(RKEY_ENABLE_FARCLIP)),
	_solidSelectionBoxes(registry::getValue<bool>(RKEY_SOLID_SELECTION_BOXES)),
	_toggleFreelook(registry::getValue<bool>(RKEY_TOGGLE_FREE_MOVE))
{
	// Constrain the cubic scale to a fixed value
	if (_cubicScale > MAX_CUBIC_SCALE) {
		_cubicScale = MAX_CUBIC_SCALE;
	}

	// Initialise the draw mode from the integer value stored in the registry
	importDrawMode(registry::getValue<int>(RKEY_DRAWMODE));

	// Connect self to the according registry keys
	observeKey(RKEY_MOVEMENT_SPEED);
	observeKey(RKEY_ROTATION_SPEED);
	observeKey(RKEY_INVERT_MOUSE_VERTICAL_AXIS);
	observeKey(RKEY_DISCRETE_MOVEMENT);
	observeKey(RKEY_ENABLE_FARCLIP);
	observeKey(RKEY_DRAWMODE);
	observeKey(RKEY_SOLID_SELECTION_BOXES);
	observeKey(RKEY_TOGGLE_FREE_MOVE);

	// greebo: Add the preference settings
	constructPreferencePage();
}

void CameraSettings::observeKey(const std::string& key)
{
    GlobalRegistry().signalForKey(key).connect(
        sigc::mem_fun(this, &CameraSettings::keyChanged)
    );
}

void CameraSettings::constructPreferencePage() 
{
	PreferencesPagePtr page = GlobalPreferenceSystem().getPage(_("Settings/Camera"));

	// Add the sliders for the movement and angle speed and connect them to the observer
    page->appendSlider(_("Movement Speed (game units)"), RKEY_MOVEMENT_SPEED, TRUE, 100, 1, MAX_CAMERA_SPEED, 1, 1, 1);
    page->appendSlider(_("Rotation Speed"), RKEY_ROTATION_SPEED, TRUE, 3, 1, 180, 1, 10, 10);

	// Add the checkboxes and connect them with the registry key and the according observer
	page->appendCheckBox("", _("Freelook mode can be toggled"), RKEY_TOGGLE_FREE_MOVE);
	page->appendCheckBox("", _("Discrete movement (non-freelook mode)"), RKEY_DISCRETE_MOVEMENT);
	page->appendCheckBox("", _("Enable far-clip plane (hides distant objects)"), RKEY_ENABLE_FARCLIP);

	// Add the "inverse mouse vertical axis in free-look mode" preference
	page->appendCheckBox("", _("Invert mouse vertical axis (freelook mode)"), RKEY_INVERT_MOUSE_VERTICAL_AXIS);

	// States whether the selection boxes are stippled or not
	page->appendCheckBox("", _("Solid selection boxes"), RKEY_SOLID_SELECTION_BOXES);

    // Whether to show the toolbar (to please the screenspace addicts)
    page->appendCheckBox(
        "", _("Show camera toolbar"), RKEY_SHOW_CAMERA_TOOLBAR
    );
}

bool CameraSettings::showCameraToolbar() const
{
    // TODO: There must be a less verbose way of introducing a new RKEY with a
    // default behaviour if unset.
    if (!GlobalRegistry().keyExists(RKEY_SHOW_CAMERA_TOOLBAR))
    {
        registry::setValue(RKEY_SHOW_CAMERA_TOOLBAR, true);
    }
    return registry::getValue<bool>(RKEY_SHOW_CAMERA_TOOLBAR);
}

void CameraSettings::importDrawMode(const int mode) 
{
	switch (mode) {
		case 0:
			_cameraDrawMode = RENDER_MODE_WIREFRAME;
			break;
		case 1:
			_cameraDrawMode = RENDER_MODE_SOLID;
			break;
		case 2:
			_cameraDrawMode = RENDER_MODE_TEXTURED;
			break;
		case 3:
			_cameraDrawMode = RENDER_MODE_LIGHTING;
			break;
		default:
			_cameraDrawMode = RENDER_MODE_TEXTURED;
	}

	GlobalRenderSystem().setShaderProgram(
        _cameraDrawMode == RENDER_MODE_LIGHTING 
        ? RenderSystem::SHADER_PROGRAM_INTERACTION
        : RenderSystem::SHADER_PROGRAM_NONE
    );

    _sigRenderModeChanged.emit();
}

void CameraSettings::keyChanged()
{
	// Check for iterative loops
	if (_callbackActive) {
		return;
	}
	else {
		_callbackActive = true;

		// Load the values from the registry
		_toggleFreelook = registry::getValue<bool>(RKEY_TOGGLE_FREE_MOVE);
		_movementSpeed = registry::getValue<int>(RKEY_MOVEMENT_SPEED);
		_angleSpeed = registry::getValue<int>(RKEY_ROTATION_SPEED);
		_invertMouseVerticalAxis = registry::getValue<bool>(RKEY_INVERT_MOUSE_VERTICAL_AXIS);
		_farClipEnabled = registry::getValue<bool>(RKEY_ENABLE_FARCLIP);
		_solidSelectionBoxes = registry::getValue<bool>(RKEY_SOLID_SELECTION_BOXES);

		GlobalEventManager().setToggled("ToggleCubicClip", _farClipEnabled);

		// Determine the draw mode represented by the integer registry value
		importDrawMode(registry::getValue<int>(RKEY_DRAWMODE));

		// Check if a global camwindow is set
		CamWndPtr cam = GlobalCamera().getActiveCamWnd();

		if (cam != NULL) {
			// Disable free move if it was enabled during key change (e.g. LightingMode Toggle)
			if (cam->freeMoveEnabled()) {
				cam->disableFreeMove();
			}

			// Disconnect the handlers for the old state and re-connect after reading the registry value
			cam->removeHandlersMove();

			// Check the value and take the according actions
			_discreteMovement = registry::getValue<bool>(RKEY_DISCRETE_MOVEMENT);

			// Reconnect the new handlers
			cam->addHandlersMove();

			cam->getCamera().updateProjection();

			// Call the update method in case the render mode has changed
			GlobalCamera().update();
		}
	}
	_callbackActive = false;
}

CameraDrawMode CameraSettings::getRenderMode() const 
{
	return _cameraDrawMode;
}

void CameraSettings::setRenderMode(const CameraDrawMode& mode) 
{
    // Write the value into the registry, this should trigger the keyChanged()
    // callback that in turn calls the update functions
	registry::setValue(RKEY_DRAWMODE, static_cast<int>(mode));
}

void CameraSettings::toggleLightingMode() 
{
	// switch between textured and lighting mode
	setRenderMode(
        (_cameraDrawMode == RENDER_MODE_LIGHTING) 
        ? RENDER_MODE_TEXTURED 
        : RENDER_MODE_LIGHTING
    );
}

bool CameraSettings::toggleFreelook() const {
	return _toggleFreelook;
}

bool CameraSettings::farClipEnabled() const
{
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

void CameraSettings::setCubicScale(const int& scale)
{
	// Update the internal value
	_cubicScale = scale;

	// Constrain the value to [1..MAX_CUBIC_SCALE]
	if (_cubicScale>MAX_CUBIC_SCALE) {
		_cubicScale = MAX_CUBIC_SCALE;
	}

	if (_cubicScale < 1) {
		_cubicScale = 1;
	}

    // Store to registry
    registry::setValue(RKEY_CUBIC_SCALE, _cubicScale);
}

void CameraSettings::setFarClip(bool farClipEnabled)
{
    registry::setValue(RKEY_ENABLE_FARCLIP, farClipEnabled);
}

void CameraSettings::toggleFarClip(bool)
{
	setFarClip(!_farClipEnabled);
}

// ---------------------------------------------------------------------------------

CameraSettings* getCameraSettings() {
	static CameraSettings _cameraSettings;
	return &_cameraSettings;
}
