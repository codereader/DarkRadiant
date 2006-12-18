#include "RadiantWindowObserver.h"

#include "ui/eventmapper/EventMapper.h"

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
void RadiantWindowObserver::onMouseDown(const WindowVector& position, GdkEventButton* event) {
	
	// Retrieve the according ObserverEvent for the GdkEventButton
	ui::ObserverEvent observerEvent = GlobalEventMapper().getObserverEvent(event);
	
	// Check if the user wants to copy/paste a texture
	if (observerEvent == ui::obsCopyTexture || observerEvent == ui::obsPasteTextureProjected || observerEvent == ui::obsPasteTextureNatural) {
		// Get the mouse position
		DeviceVector devicePosition(device_constrained(window_to_normalised_device(position, _width, _height)));

		// Check the target object 
		View scissored(*_selectObserver._view);
		ConstructSelectionTest(scissored, SelectionBoxForPoint(&devicePosition[0], &_selectObserver._epsilon[0]));
		SelectionVolume volume(scissored);

		// If the apply texture modifier is held (standard: Ctrl-Shift)
		if (observerEvent == ui::obsPasteTextureProjected) {
			// Apply the texture, with the boolean set to true (this means "project")
			Scene_applyClosestTexture(volume, true);
		}
		// If the copy texture modifier is held (standard: Alt-Ctrl)
		else if (observerEvent == ui::obsCopyTexture) {
			Scene_copyClosestTexture(volume);
		}
		else if (observerEvent == ui::obsPasteTextureNatural) {
			// Apply the texture, but do not distort the (patch's) textures, hence "false"
			Scene_applyClosestTexture(volume, false);
		}
	}
		
	// Have any of the "selection" events occurred? 
	// greebo: This could be an "else if (observerEvent != obsNothing)" as well,
	// but perhaps there will be more events in the future that aren't selection events.
	if (observerEvent == ui::obsManipulate || observerEvent == ui::obsSelect ||
		observerEvent == ui::obsToggle || observerEvent == ui::obsToggleFace || 
		observerEvent == ui::obsReplace || observerEvent == ui::obsReplaceFace) 
	{
		_mouseDown = true;
		
		// Determine the current mouse position
		DeviceVector devicePosition(window_to_normalised_device(position, _width, _height));
		
		if (observerEvent ==  ui::obsManipulate && _manipulateObserver.mouseDown(devicePosition)) {
			// This is a manipulation operation, register the callbacks
			// Note: the mouseDown call in the if clause returned already true, 
			// so a manipulator could be successfully selected
			g_mouseMovedCallback.insert(MouseEventCallback(ManipulateObserver::MouseMovedCaller(_manipulateObserver)));
			g_mouseUpCallback.insert(MouseEventCallback(ManipulateObserver::MouseUpCaller(_manipulateObserver)));
		} 
		else {
			// Call the mouseDown method of the selector class, this covers all of the other events
			_selectObserver.mouseDown(devicePosition);
			
			// Register the Selector class mouseDown and mouseUp callBacks  
			g_mouseMovedCallback.insert(MouseEventCallback(SelectObserver::MouseMovedCaller(_selectObserver)));
			g_mouseUpCallback.insert(MouseEventCallback(SelectObserver::MouseUpCaller(_selectObserver)));
			
			// greebo: the according actions (toggle face, replace, etc.) are handled in the mouseUp methods.
		}
	}
}

/* greebo: Handle the mouse movement. This notifies the registered mouseMove callback
 * and resets the cycle selection counter. The argument state is unused at the moment.  
 */
void RadiantWindowObserver::onMouseMotion(const WindowVector& position, const unsigned int& state) {
	// The mouse has been moved, so reset the counter of the cycle selection stack
	_selectObserver._unmovedReplaces = 0;
	
	_selectObserver.setState(state);
	
	/* If the mouse button is currently held, this can be considered a drag, so
	 * notify the according mouse move callback, if there is one registered */
	if( _mouseDown && !g_mouseMovedCallback.empty()) {
  		g_mouseMovedCallback.get()(window_to_normalised_device(position, _width, _height));
	}
}

/* greebo: Handle the mouseUp event. Usually, this means the end of an operation, so
 * this has to check, if there are any callbacks connected, and call them if this is the case
 */
void RadiantWindowObserver::onMouseUp(const WindowVector& position, GdkEventButton* event) {
	
	// Retrieve the according ObserverEvent for the GdkEventButton
	ui::ObserverEvent observerEvent = GlobalEventMapper().getObserverEvent(event);
	
	// Only react, if the "select" or "manipulate" is held, ignore this otherwise
	bool reactToEvent = (observerEvent == ui::obsManipulate || observerEvent == ui::obsSelect ||
						 observerEvent == ui::obsToggle || observerEvent == ui::obsToggleFace || 
						 observerEvent == ui::obsReplace || observerEvent == ui::obsReplaceFace); 
	
	if (reactToEvent && !g_mouseUpCallback.empty()) {
		// No mouse button is active anymore 
  		_mouseDown = false;
  		
  		// Store the current event in the observer classes
  		_selectObserver.setEvent(event);
  		_manipulateObserver.setEvent(event);
  		
  		// Get the callback and call it with the arguments
		g_mouseUpCallback.get()(window_to_normalised_device(position, _width, _height));
	}
}

// Called upon modifier key press
void RadiantWindowObserver::onModifierDown(ModifierFlags type) {
	//_selectObserver.updateState();
}

// Called upon modifier key release
void RadiantWindowObserver::onModifierUp(ModifierFlags type) {
	//_selectObserver.updateState();
}
