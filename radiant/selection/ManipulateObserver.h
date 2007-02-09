#ifndef MANIPULATEOBSERVER_H_
#define MANIPULATEOBSERVER_H_

#include "gdk/gdkevents.h"
#include "generic/callbackfwd.h"
#include "windowobserver.h"
#include "Device.h"
#include "RadiantSelectionSystem.h"

/* greebo: This is the class handling the manipulate-related mouse operations, it basically just
 * passes all the mouse clicks back to the SelectionSystem. The callbacks are invoked from
 * RadiantWindowObserver, which is registered in the GlobalWindowObservers list
 */
class ManipulateObserver {
	GdkEventButton* _event;
public:
	DeviceVector _epsilon;
	const View* _view;

	// Updates the internal event pointer
	void setEvent(GdkEventButton* event);

	// greebo: Handles the mouseDown event and checks whether a manipulator can be made active 
	bool mouseDown(DeviceVector position);

	/* greebo: Pass the mouse movement to the current selection.
	 * This is connected to the according mouse events by the RadiantWindowObserver class 
	 */
  	void mouseMoved(DeviceVector position);
  	
  	// The mouse operation is finished, update the selection and unconnect the callbacks
  	void mouseUp(DeviceVector position);
  
  	typedef MemberCaller1<ManipulateObserver, DeviceVector, &ManipulateObserver::mouseMoved> MouseMovedCaller;
  	typedef MemberCaller1<ManipulateObserver, DeviceVector, &ManipulateObserver::mouseUp> MouseUpCaller;
};

#endif /*MANIPULATEOBSERVER_H_*/
