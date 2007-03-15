#include "RadiantWindowObserver.h"

#include "gdk/gdkkeysyms.h"
#include "ieventmanager.h"
#include "iscenegraph.h"
#include "Device.h"
#include "camera/GlobalCamera.h"
#include "selection/algorithm/Shader.h"
#include "selection/shaderclipboard/ShaderClipboard.h"
#include <iostream>

// mouse callback instances
Single<MouseEventCallback> g_mouseMovedCallback;
Single<MouseEventCallback> g_mouseUpCallback;

SelectionSystemWindowObserver* NewWindowObserver() {
  return new RadiantWindowObserver;
}

void RadiantWindowObserver::setObservedWidget(GtkWidget* observed) {
	_observedWidget = observed;
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
	ui::ObserverEvent observerEvent = GlobalEventManager().MouseEvents().getObserverEvent(event);
	
	// Check if the user wants to copy/paste a texture
	if (observerEvent == ui::obsCopyTexture || observerEvent == ui::obsPasteTextureProjected ||
		observerEvent == ui::obsPasteTextureNatural || observerEvent == ui::obsPasteTextureCoordinates ||
		observerEvent == ui::obsPasteTextureToBrush || observerEvent == ui::obsJumpToObject) 
	{
		// Get the mouse position
		DeviceVector devicePosition(device_constrained(window_to_normalised_device(position, _width, _height)));

		// Check the target object 
		View scissored(*_selectObserver._view);
		ConstructSelectionTest(scissored, SelectionBoxForPoint(&devicePosition[0], &_selectObserver._epsilon[0]));
		SelectionVolume volume(scissored);
		
		// Do we have a camera view (fill() == true)?
		if (_selectObserver._view->fill()) {
			if (observerEvent == ui::obsJumpToObject) {
				GlobalCamera().getCamWnd()->jumpToObject(volume);
			}
			// If the apply texture modifier is held
			else if (observerEvent == ui::obsPasteTextureProjected) {
				// Paste the shader projected (TRUE), but not to an entire brush (FALSE)
				selection::algorithm::pasteShader(volume, true, false);
			}
			// If the copy texture modifier is held
			else if (observerEvent == ui::obsCopyTexture) {
				// Set the source texturable from the given test
				GlobalShaderClipboard().setSource(volume);
			}
			else if (observerEvent == ui::obsPasteTextureNatural) {
				// Paste the shader naturally (FALSE), but not to an entire brush (FALSE)
				selection::algorithm::pasteShader(volume, false, false);
			}
			else if (observerEvent == ui::obsPasteTextureCoordinates) {
				// Clone the texture coordinates from the patch in the clipboard
				selection::algorithm::pasteTextureCoords(volume);
			}
			else if (observerEvent == ui::obsPasteTextureToBrush) {
				// Paste the shader projected (TRUE), and to the entire brush (TRUE)
				selection::algorithm::pasteShader(volume, true, true);
			}
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
			
			if (_observedWidget != NULL) {
				// Connect the keypress event to catch the "cancel" call
				_keyPressHandler = g_signal_connect(G_OBJECT(_observedWidget), 
								 					"key_press_event", 
								 					G_CALLBACK(onKeyPress), 
								 					this);
			}
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
	ui::ObserverEvent observerEvent = GlobalEventManager().MouseEvents().getObserverEvent(event);
	
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
	
	if (_observedWidget != NULL && _keyPressHandler != 0) {
		g_signal_handler_disconnect(G_OBJECT(_observedWidget), _keyPressHandler);
		_keyPressHandler = 0;
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

void RadiantWindowObserver::onCancel() {
	
	// Disconnect the mouseMoved and mouseUp callbacks
	g_mouseMovedCallback.clear();
	g_mouseUpCallback.clear();
	
	// Disconnect the key handler
	if (_observedWidget != NULL && _keyPressHandler != 0) {
		g_signal_handler_disconnect(G_OBJECT(_observedWidget), _keyPressHandler);
		_keyPressHandler = 0;
	}
	
	// Update the views
	GlobalSelectionSystem().cancelMove();
}

// The GTK keypress callback
gboolean RadiantWindowObserver::onKeyPress(GtkWindow* window, 
	GdkEventKey* event, RadiantWindowObserver* self) 
{
	// Check for ESC and call the onCancel method, if found
	if (event->keyval == GDK_Escape) {
		self->onCancel();
		
		// Don't pass the key event to the event chain 
		return true;
	}
	
	return false;
}
