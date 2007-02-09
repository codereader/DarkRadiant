#ifndef SELECTOBSERVER_H_
#define SELECTOBSERVER_H_

#include "gdk/gdkevents.h"

#include "iselection.h"
#include "generic/callback.h"
#include "windowobserver.h"
#include "Device.h"
#include "SelectionBox.h"
#include "view.h"

/* greebo: This is the class that handles the selection-related mouse operations, like Alt-Shift-Click,
 * Selection toggles and drag selections. All the other modifier combinations that might occur are ignored,
 * just the selection-relevant combinations are handled. The callbacks are invoked from
 * RadiantWindowObserver, which is registered in the GlobalWindowObservers list
 */
class SelectObserver {

public:
	// The internally stored mouse positions
	DeviceVector _start;		// This is set at mouseDown
	DeviceVector _current;		// This is set during mouseMove
	DeviceVector _epsilon;		// A small threshold value to "swallow" minimal mouse moves
	
	std::size_t _unmovedReplaces;
	
	const View* _view;
	RectangleCallback _windowUpdate;
	
	GdkEventButton* _event;
	unsigned int _state;
	
private:
	/* Returns the current "selection mode" (eToggle, eReplace, etc.) according
	* to the mouse buttons and modifiers
	* Note: standard return value is eManipulator
	*/
	SelectionSystem::EModifier getModifier();

	/* Return the rectangle coordinates spanned by the mouse pointer and the starting point
	 */
	Rectangle getDeviceArea() const;

public:
	// Constructor
	SelectObserver();

	// Updates the internal event pointer
	void setEvent(GdkEventButton* event);

	// greebo: This gets the rectangle coordinates and passes them to the RectangleCallback function
  	void draw_area();

	/* This is called upon mouseUp and checks what action is to take
	 * The DeviceVector position is passed with constrained values that are between [-1,+1]
	 */
  	void testSelect(DeviceVector position);

	// Returns true if the user is currently selecting something (i.e. if any modifieres are held) 
	bool selecting() const;

	// Called right before onMouseMotion to store the current GDK state (needed for draw_area)
	void setState(const unsigned int& state);

	// onMouseDown: Save the current mouse position as start, the mouse operation is beginning now
  	void mouseDown(DeviceVector position);

	// onMouseMove: store the current position, and call the area draw update 
	void mouseMoved(DeviceVector position);
	
	// The mouseUp callback: check what has to be done and unconnect self from the calbacks
  	void mouseUp(DeviceVector position);
  	
  	typedef MemberCaller1<SelectObserver, DeviceVector, &SelectObserver::mouseMoved> MouseMovedCaller;
  	typedef MemberCaller1<SelectObserver, DeviceVector, &SelectObserver::mouseUp> MouseUpCaller;
};

#endif /*SELECTOBSERVER_H_*/
