#include "ManipulateObserver.h"

#include "generic/callback.h"

// mouse callback instances
extern Single<MouseEventCallback> g_mouseMovedCallback;
extern Single<MouseEventCallback> g_mouseUpCallback;

// Updates the internal event pointer
void ManipulateObserver::setEvent(GdkEventButton* event) {
	_event = event;
}

// greebo: Handles the mouseDown event and checks whether a manipulator can be made active 
bool ManipulateObserver::mouseDown(DeviceVector position) {
	return GlobalSelectionSystem().SelectManipulator(*_view, &position[0], &_epsilon[0]);
}

/* greebo: Pass the mouse movement to the current selection.
 * This is connected to the according mouse events by the RadiantWindowObserver class 
 */
void ManipulateObserver::mouseMoved(DeviceVector position) {
	GlobalSelectionSystem().MoveSelected(*_view, &position[0]);
}

// The mouse operation is finished, update the selection and unconnect the callbacks
void ManipulateObserver::mouseUp(DeviceVector position) {
	// Notify the selectionsystem about the ended operation 
	GlobalSelectionSystem().endMove();
	
	// Unconnect this method from the callbacks
	g_mouseMovedCallback.clear();
	g_mouseUpCallback.clear();
}
