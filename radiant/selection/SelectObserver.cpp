#include "SelectObserver.h"

#include "ieventmanager.h"

// mouse callback instances
extern Single<MouseEventCallback> g_mouseMovedCallback;
extern Single<MouseEventCallback> g_mouseUpCallback;

// Constructor
SelectObserver::SelectObserver() :
	_start(0.0f, 0.0f),
	_current(0.0f, 0.0f),
	_unmovedReplaces(0) 
{}

SelectionSystem::EModifier SelectObserver::getModifier() {

	IMouseEvents& mouseEvents = GlobalEventManager().MouseEvents();

	// Retrieve the according ObserverEvent for the GdkEventButton
	ui::ObserverEvent observerEvent = mouseEvents.getObserverEvent(_event);

	if (observerEvent == ui::obsSelect || observerEvent == ui::obsToggle
	        || observerEvent == ui::obsToggleFace) {
		return SelectionSystem::eToggle;
	}

	if (observerEvent == ui::obsReplace || observerEvent == ui::obsReplaceFace) {
		return SelectionSystem::eReplace;
	}

	// greebo: Return the standard case: eManipulator mode, if none of the above apply
	return SelectionSystem::eManipulator;
}

/* Return the rectangle coordinates spanned by the mouse pointer and the starting point
 */
Rectangle SelectObserver::getDeviceArea() const {
	// get the mouse position relative to the starting point
	DeviceVector delta(_current - _start);

	// If the user is selecting or dragging, the SelectionBox is returned...
	if (selecting() && fabs(delta.x()) > _epsilon.x() && fabs(delta.y()) > _epsilon.y()) {
		return SelectionBoxForArea(&_start[0], &delta[0]);
	}
	// ...otherwise return the null area
	else {
		Rectangle default_area = { { 0, 0, }, { 0, 0, }, };
		return default_area;
	}
}

// Updates the internal event pointer
void SelectObserver::setEvent(GdkEventButton* event) {
	_event = event;
}

// greebo: This gets the rectangle coordinates and passes them to the RectangleCallback function
void SelectObserver::draw_area() {
	_windowUpdate(getDeviceArea());
}

/* This is called upon mouseUp and checks what action is to take
 * The DeviceVector position is passed with constrained values that are between [-1,+1]
 */
void SelectObserver::testSelect(DeviceVector position) {

	// Get the MouseEvents class from the EventManager
	IMouseEvents& mouseEvents = GlobalEventManager().MouseEvents();

	// Obtain the current modifier status (eManipulator, etc.)
	SelectionSystem::EModifier modifier = getModifier();

	// Determine, if we have a face operation
	// Retrieve the according ObserverEvent for the GdkEventButton
	ui::ObserverEvent observerEvent = mouseEvents.getObserverEvent(_event);
	bool isFaceOperation = (observerEvent == ui::obsToggleFace || observerEvent == ui::obsReplaceFace);

	// If the user pressed some of the modifiers (Shift, Alt, Ctrl) the mode is NOT eManipulator
	// so the if clause is true if there are some modifiers active
	if (modifier != SelectionSystem::eManipulator) {
		// Get the distance of the mouse pointer from the starting point
		DeviceVector delta(position - _start);

		// If the mouse pointer has moved more than <epsilon>, this is considered a drag operation
		if (fabs(delta.x()) > _epsilon.x() && fabs(delta.y()) > _epsilon.y()) {
			// This is a drag operation with a modifier held
			DeviceVector delta(position - _start);	// << superfluous?

			// Call the selectArea command that does the actual selecting
			GlobalSelectionSystem().SelectArea(*_view, &_start[0], &delta[0], modifier, isFaceOperation);
		}
		else {
			// greebo: This is a click operation with a modifier held
			// If Alt-Shift (eReplace) is held, and we already replaced a selection, switch to cycle mode
			// so eReplace is only active during the first click with Alt-Shift
			if (modifier == SelectionSystem::eReplace && _unmovedReplaces++ > 0) {
				modifier = SelectionSystem::eCycle;
			}
			// Call the selectPoint command in RadiantSelectionSystem that does the actual selecting
			GlobalSelectionSystem().SelectPoint(*_view, &position[0], &_epsilon[0], modifier, isFaceOperation);
		}
	}

	// Reset the mouse position to zero, this mouse operation is finished so far
	_start = _current = DeviceVector(0.0f, 0.0f);
	draw_area();
}

// Returns true if the user is currently selecting something (i.e. if any modifieres are held)
bool SelectObserver::selecting() const {
	ui::ObserverEvent observerEvent = GlobalEventManager().MouseEvents().getObserverEvent(_state);
	return observerEvent != ui::obsManipulate;
}

// Called right before onMouseMotion to store the current GDK state (needed for draw_area)
void SelectObserver::setState(const unsigned int& state) {
	_state = state;
}

// onMouseDown: Save the current mouse position as start, the mouse operation is beginning now
void SelectObserver::mouseDown(DeviceVector position) {
	_start = _current = device_constrained(position);
}

// onMouseMove: store the current position, and call the area draw update
void SelectObserver::mouseMoved(DeviceVector position) {
	_current = device_constrained(position);
	draw_area();
}

// The mouseUp callback: check what has to be done and unconnect self from the calbacks
void SelectObserver::mouseUp(DeviceVector position) {
	// Check the result of this (finished) operation, is it a drag or a click?
	testSelect(device_constrained(position));

	// Unconnect this method from the callbacks
	g_mouseMovedCallback.clear();
	g_mouseUpCallback.clear();
}
