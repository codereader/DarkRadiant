#ifndef MANIPULATEOBSERVER_H_
#define MANIPULATEOBSERVER_H_

#include "Device.h"
#include "RadiantSelectionSystem.h"

/* greebo: This is the class handling the manipulate-related mouse operations, it basically just
 * passes all the mouse clicks back to the SelectionSystem. The callbacks are invoked from
 * RadiantWindowObserver, which is registered in the GlobalWindowObservers list
 */
class ManipulateObserver {
public:
  DeviceVector _epsilon;
  const View* _view;

	// greebo: Handles the mouseDown event and checks whether a manipulator can be made active 
	bool mouseDown(DeviceVector position) {
		return GlobalSelectionSystem().SelectManipulator(*_view, &position[0], &_epsilon[0]);
	}

	/* greebo: Pass the mouse movement to the current selection.
	 * This is connected to the according mouse events by the RadiantWindowObserver class 
	 */
  	void mouseMoved(DeviceVector position) {
    	GlobalSelectionSystem().MoveSelected(*_view, &position[0]);
  	}
  	
  	// The mouse operation is finished, update the selection and unconnect the callbacks
  	void mouseUp(DeviceVector position) {
  		// Notify the selectionsystem about the ended operation 
    	GlobalSelectionSystem().endMove();
    	
    	// Unconnect this method from the callbacks
    	g_mouseMovedCallback.clear();
    	g_mouseUpCallback.clear();
  	}
  
  	typedef MemberCaller1<ManipulateObserver, DeviceVector, &ManipulateObserver::mouseMoved> MouseMovedCaller;
  	typedef MemberCaller1<ManipulateObserver, DeviceVector, &ManipulateObserver::mouseUp> MouseUpCaller;
};

#endif /*MANIPULATEOBSERVER_H_*/
