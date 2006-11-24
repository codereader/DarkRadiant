#include "RadiantWindowObserver.h"

// mouse callback instances
Single<MouseEventCallback> g_mouseMovedCallback;
Single<MouseEventCallback> g_mouseUpCallback;

SelectionSystemWindowObserver* NewWindowObserver() {
  return new RadiantWindowObserver;
}

void RadiantWindowObserver::setView(const View& view) {
	_selectObserver._view = &view;
	_manipulateObserver._view = &view;
}

void RadiantWindowObserver::setRectangleDrawCallback(const RectangleCallback& callback) {
	_selectObserver._windowUpdate = callback;
}

// greebo: This is called if the window size changes (camera, orthoview)
void RadiantWindowObserver::onSizeChanged(int width, int height) {
	// Store the new width and height
	_width = width;
	_height = height;
	
	// Rescale the epsilon accordingly...
	DeviceVector epsilon(SELECT_EPSILON / static_cast<float>(_width), SELECT_EPSILON / static_cast<float>(_height));
	// ...and pass it to the helper classes
	_selectObserver._epsilon = epsilon;
	_manipulateObserver._epsilon = epsilon;
}
  
// Handles the mouseDown event, basically determines which action should be performed (select or manipulate)
void RadiantWindowObserver::onMouseDown(const WindowVector& position, ButtonIdentifier button, ModifierFlags modifiers) {

	// Is the Select button (default: left button) pressed
	if(button == c_button_select) {
		_mouseDown = true;

		DeviceVector devicePosition(window_to_normalised_device(position, _width, _height));
  
		if (modifiers == c_modifier_manipulator && _manipulateObserver.mouseDown(devicePosition)) {
			// This is a manipulation operation, register the callbacks
			// Note: the mouseDown call in the if clause returned already true, 
			// so a manipulator could be successfully selected
			g_mouseMovedCallback.insert(MouseEventCallback(ManipulateObserver::MouseMovedCaller(_manipulateObserver)));
			g_mouseUpCallback.insert(MouseEventCallback(ManipulateObserver::MouseUpCaller(_manipulateObserver)));
		}
		else {
			// Call the mouseDown method of the selector class
			_selectObserver.mouseDown(devicePosition);

			// Register the Selector class mouseDown and mouseUp callBacks  
			g_mouseMovedCallback.insert(MouseEventCallback(SelectObserver::MouseMovedCaller(_selectObserver)));
			g_mouseUpCallback.insert(MouseEventCallback(SelectObserver::MouseUpCaller(_selectObserver)));
		}
	}
	// if the texture (middle mouse) button is pressed
	else if (button == c_button_texture) {
		// get the mouse position
		DeviceVector devicePosition(device_constrained(window_to_normalised_device(position, _width, _height)));

		// Check the target object 
		View scissored(*_selectObserver._view);
		ConstructSelectionTest(scissored, SelectionBoxForPoint(&devicePosition[0], &_selectObserver._epsilon[0]));
		SelectionVolume volume(scissored);

		// If the apply texture modifier is held (standard: Ctrl-Shift)
		if(modifiers == c_modifier_apply_texture) {
			Scene_applyClosestTexture(volume);
		}
		// If the copy texture modifier is held (standard: Alt-Ctrl)
		else if(modifiers == c_modifier_copy_texture) {
			Scene_copyClosestTexture(volume);
		}
	}
}

/* greebo: Handle the mouse movement. This notifies the registered mouseMove callback
 * and resets the cycle selection counter 
 */
void RadiantWindowObserver::onMouseMotion(const WindowVector& position, ModifierFlags modifiers) {
	// The mouse has been moved, so reset the counter of the cycle selection stack
	_selectObserver._unmovedReplaces = 0;
	
	/* If the mouse button is currently held, this can be considered a drag, so
	 * notify the according mouse move callback, if there is one registered */
	if( _mouseDown && !g_mouseMovedCallback.empty()) {
  		g_mouseMovedCallback.get()(window_to_normalised_device(position, _width, _height));
	}
}

/* greebo: Handle the mouseUp event. Usually, this means the end of an operation, so
 * this has to check, if there are any callbacks connected, and call them if this is the case
 */
void RadiantWindowObserver::onMouseUp(const WindowVector& position, ButtonIdentifier button, ModifierFlags modifiers) {
	// Only react, if the "select" (usually left) mouse button is held, ignore this otherwise
	if (button == c_button_select && !g_mouseUpCallback.empty()) {
		// No mouse button is active anymore 
  		_mouseDown = false;
  		// Get the callback and call it with the arguments
		g_mouseUpCallback.get()(window_to_normalised_device(position, _width, _height));
	}
}

// Called upon modifier key press, updates the interal bit mask accordingly
void RadiantWindowObserver::onModifierDown(ModifierFlags type) {
	_selectObserver.modifierEnable(type);
}

// Called upon modifier key release, updates the interal bit mask accordingly
void RadiantWindowObserver::onModifierUp(ModifierFlags type) {
	_selectObserver.modifierDisable(type);
}
