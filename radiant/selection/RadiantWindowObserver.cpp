#include "RadiantWindowObserver.h"

#include "gdk/gdkkeysyms.h"
#include "ieventmanager.h"
#include "iscenegraph.h"
#include "Device.h"
#include "camera/GlobalCamera.h"
#include "selection/algorithm/Shader.h"
#include "selection/shaderclipboard/ShaderClipboard.h"
#include "registry/registry.h"
#include <iostream>
#include <boost/bind.hpp>

SelectionSystemWindowObserver* NewWindowObserver() {
  return new RadiantWindowObserver;
}

namespace
{
	const char* const RKEY_SELECT_EPSILON = "user/ui/selectionEpsilon";
}

RadiantWindowObserver::RadiantWindowObserver() :
	_selectEpsilon(registry::getValue<float>(RKEY_SELECT_EPSILON)),
	_mouseDown(false),
	_listenForCancelEvents(false)
{}

void RadiantWindowObserver::addObservedWidget(Gtk::Widget* observed)
{
	// Connect the keypress event to catch the "cancel" events (ESC)
	_keyHandlers[observed] = observed->signal_key_press_event().connect(
		sigc::mem_fun(*this, &RadiantWindowObserver::onKeyPress), false);
}

void RadiantWindowObserver::removeObservedWidget(Gtk::Widget* observed)
{
	KeyHandlerMap::iterator found = _keyHandlers.find(observed);

	if (found == _keyHandlers.end())
	{
		rWarning() <<
			"RadiantWindowObserver: Cannot remove observed widget, not found."
			<< std::endl;
		return;
	}

	// Disconnect the key handler
	found->second.disconnect();

	// And remove the element from our map
	_keyHandlers.erase(found);
}

void RadiantWindowObserver::addObservedWidget(const Glib::RefPtr<Gtk::Widget>& observed)
{
	_refKeyHandlers[observed] = observed->signal_key_press_event().connect(
		sigc::mem_fun(*this, &RadiantWindowObserver::onKeyPress), false);
}

void RadiantWindowObserver::removeObservedWidget(const Glib::RefPtr<Gtk::Widget>& observed)
{
	RefPtrKeyHandlerMap::iterator found = _refKeyHandlers.find(observed);

	if (found == _refKeyHandlers.end())
	{
		rWarning() <<
			"RadiantWindowObserver: Cannot remove observed refptr widget, not found."
			<< std::endl;
		return;
	}

	// Disconnect the key handler
	found->second.disconnect();

	// And remove the element from our map
	_refKeyHandlers.erase(found);
}

void RadiantWindowObserver::setView(const render::View& view)
{
	_selectObserver._view = &view;
	_manipulateObserver._view = &view;
}

void RadiantWindowObserver::setRectangleDrawCallback(const Rectangle::Callback& callback)
{
	_selectObserver._windowUpdate = callback;
}

// greebo: This is called if the window size changes (camera, orthoview)
void RadiantWindowObserver::onSizeChanged(int width, int height)
{
	// Store the new width and height
	_width = width;
	_height = height;

	// Rescale the epsilon accordingly...
	DeviceVector epsilon(_selectEpsilon / _width, _selectEpsilon / _height);
	// ...and pass it to the helper classes
	_selectObserver._epsilon = epsilon;
	_manipulateObserver._epsilon = epsilon;
}

// Handles the mouseDown event, basically determines which action should be performed (select or manipulate)
void RadiantWindowObserver::onMouseDown(const WindowVector& position, GdkEventButton* ev)
{
	// Retrieve the according ObserverEvent for the GdkEventButton
	ui::ObserverEvent observerEvent = GlobalEventManager().MouseEvents().getObserverEvent(ev);

	// Check if the user wants to copy/paste a texture
	if (observerEvent == ui::obsCopyTexture || observerEvent == ui::obsPasteTextureProjected ||
		observerEvent == ui::obsPasteTextureNatural || observerEvent == ui::obsPasteTextureCoordinates ||
		observerEvent == ui::obsPasteTextureToBrush || observerEvent == ui::obsJumpToObject)
	{
		// Get the mouse position
		DeviceVector devicePosition(device_constrained(window_to_normalised_device(position, _width, _height)));

		// Check the target object
		render::View scissored(*_selectObserver._view);
		ConstructSelectionTest(scissored, Rectangle::ConstructFromPoint(devicePosition, _selectObserver._epsilon));
		SelectionVolume volume(scissored);

		// Do we have a camera view (fill() == true)?
		if (_selectObserver._view->fill())
		{
			if (observerEvent == ui::obsJumpToObject) {
				CamWndPtr cam = GlobalCamera().getActiveCamWnd();
				if (cam != NULL) {
					cam->jumpToObject(volume);
				}
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
		observerEvent == ui::obsToggleGroupPart ||
		observerEvent == ui::obsReplace || observerEvent == ui::obsReplaceFace)
	{
		_mouseDown = true;

		// Determine the current mouse position
		DeviceVector devicePosition(window_to_normalised_device(position, _width, _height));

		if (observerEvent ==  ui::obsManipulate && _manipulateObserver.mouseDown(devicePosition)) {
			// This is a manipulation operation, register the callbacks
			// Note: the mouseDown call in the if clause returned already true,
			// so a manipulator could be successfully selected
			_mouseMotionCallback = boost::bind(&ManipulateObserver::mouseMoved, &_manipulateObserver, _1);
			_mouseUpCallback = boost::bind(&ManipulateObserver::mouseUp, &_manipulateObserver, _1);

			_listenForCancelEvents = true;
		}
		else {
			// Call the mouseDown method of the selector class, this covers all of the other events
			_selectObserver.mouseDown(devicePosition);

			_mouseMotionCallback = boost::bind(&SelectObserver::mouseMoved, &_selectObserver, _1);
			_mouseUpCallback = boost::bind(&SelectObserver::mouseUp, &_selectObserver, _1);

			// greebo: the according actions (toggle face, replace, etc.) are handled in the mouseUp methods.
		}
	}
}

/* greebo: Handle the mouse movement. This notifies the registered mouseMove callback
 * and resets the cycle selection counter. The argument state is unused at the moment.
 */
void RadiantWindowObserver::onMouseMotion(const WindowVector& position, unsigned int state)
{
	// The mouse has been moved, so reset the counter of the cycle selection stack
	_selectObserver._unmovedReplaces = 0;

	_selectObserver.setState(state);

	/* If the mouse button is currently held, this can be considered a drag, so
	 * notify the according mouse move callback */
	if( _mouseDown && _mouseMotionCallback)
	{
		_mouseMotionCallback(window_to_normalised_device(position, _width, _height));
	}
}

/* greebo: Handle the mouseUp event. Usually, this means the end of an operation, so
 * this has to check, if there are any callbacks connected, and call them if this is the case
 */
void RadiantWindowObserver::onMouseUp(const WindowVector& position, GdkEventButton* ev)
{
	// Retrieve the according ObserverEvent for the GdkEventButton
	ui::ObserverEvent observerEvent = GlobalEventManager().MouseEvents().getObserverEvent(ev);

	// Only react, if the "select" or "manipulate" is held, ignore this otherwise
	bool reactToEvent = (observerEvent == ui::obsManipulate || observerEvent == ui::obsSelect ||
						 observerEvent == ui::obsToggle || observerEvent == ui::obsToggleFace ||
						 observerEvent == ui::obsToggleGroupPart ||
						 observerEvent == ui::obsReplace || observerEvent == ui::obsReplaceFace);

	if (reactToEvent)
	{
		// No mouse button is active anymore
  		_mouseDown = false;

  		// Store the current event in the observer classes
  		_selectObserver.setEvent(ev);
  		_manipulateObserver.setEvent(ev);

  		// Get the callback and call it with the arguments
		if (_mouseUpCallback)
		{
			_mouseUpCallback(window_to_normalised_device(position, _width, _height));
		}
	}

	// Stop listening for cancel events
	_listenForCancelEvents = false;

	// Disconnect the mouseMoved and mouseUp callbacks, mouse has been released
	_mouseMotionCallback.clear();
	_mouseUpCallback.clear();
}

void RadiantWindowObserver::cancelOperation()
{
	// Disconnect the mouseMoved and mouseUp callbacks
	_mouseMotionCallback.clear();
	_mouseUpCallback.clear();

	// Stop listening for cancel events
	_listenForCancelEvents = false;

	// Update the views
	GlobalSelectionSystem().cancelMove();
}

// The GTK keypress callback
bool RadiantWindowObserver::onKeyPress(GdkEventKey* ev)
{
	if (!_listenForCancelEvents)
	{
		// Not listening, let the event pass through
		return false;
	}

	// Check for ESC and call the cancelOperation method, if found
	if (ev->keyval == GDK_Escape)
	{
		cancelOperation();

		// Don't pass the key event to the event chain
		return true;
	}

	return false;
}
