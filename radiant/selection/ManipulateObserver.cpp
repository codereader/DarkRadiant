#include "ManipulateObserver.h"

// greebo: Handles the mouseDown event and checks whether a manipulator can be made active
bool ManipulateObserver::mouseDown(DeviceVector position)
{
	return GlobalSelectionSystem().SelectManipulator(*_view, position, _epsilon);
}

/* greebo: Pass the mouse movement to the current selection.
 * This is connected to the according mouse events by the RadiantWindowObserver class
 */
void ManipulateObserver::mouseMoved(DeviceVector position)
{
	GlobalSelectionSystem().MoveSelected(*_view, position);
}

// The mouse operation is finished, update the selection and unconnect the callbacks
void ManipulateObserver::mouseUp(DeviceVector position)
{
	// Notify the selectionsystem about the ended operation
	GlobalSelectionSystem().endMove();
}
