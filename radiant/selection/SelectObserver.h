#ifndef SELECTOBSERVER_H_
#define SELECTOBSERVER_H_

#include "ButtonsModifiers.h"
#include "Device.h"
#include "windowobserver.h"
#include "SelectionBox.h"

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
	
	// The keyboard modifier states (Shift, Ctrl, Alt)
	ModifierFlags _state;
	  
	std::size_t _unmovedReplaces;	
	
	const View* _view;
	RectangleCallback _windowUpdate;
	
private:
	/* Returns the current "selection mode" (eToggle, eReplace, etc.) according
	* to the mouse buttons and modifiers
	* Note: standard return value is eManipulator
	*/
	SelectionSystem::EModifier modifier_for_state(ModifierFlags state) {
		// greebo: If (Shift) or (Shift-Control) ist held, switch to eToggle Mode  
		if(state == c_modifier_toggle || state == c_modifier_toggle_face) {
			return SelectionSystem::eToggle;
		}
	    
	    // greebo: If (Shift-Alt) or (Ctrl-Shift-Alt) is held, switch to eReplace Mode 
	    if(state == c_modifier_replace || state == c_modifier_replace_face) {
	      return SelectionSystem::eReplace;
	    }
	    
	    // greebo: Return the standard case: eManipulator mode, if none of the above apply
	    return SelectionSystem::eManipulator;
	}

	/* Return the rectangle coordinates spanned by the mouse pointer and the starting point
	 */
	Rectangle getDeviceArea() const {
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

public:
	// Constructor
	SelectObserver() : 
		_start(0.0f, 0.0f), 
		_current(0.0f, 0.0f), 
		_state(c_modifierNone), 
		_unmovedReplaces(0)
	{}

	// greebo: This gets the rectangle coordinates and passes them to the RectangleCallback function
  	void draw_area() {
    	_windowUpdate(getDeviceArea());
  	}

	/* This is called upon mouseUp and checks what action is to take
	 * The DeviceVector position is passed with constrained values that are between [-1,+1]
	 */
  	void testSelect(DeviceVector position) {
	  	// Obtain the current modifier status (eManipulator, etc.)
	    SelectionSystem::EModifier modifier = modifier_for_state(_state);
	    
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
	        	GlobalSelectionSystem().SelectArea(*_view, &_start[0], &delta[0], modifier, (_state & c_modifier_face) != c_modifierNone);
	      	}
	      	else {
	      		// greebo: This is a click operation with a modifier held
	      		// If Alt-Shift (eReplace) is held, and we already replaced a selection, switch to cycle mode
	      		// so eReplace is only active during the first click with Alt-Shift
	        	if(modifier == SelectionSystem::eReplace && _unmovedReplaces++ > 0) {
	          		modifier = SelectionSystem::eCycle;
	        	}
	        	// Call the selectPoint command in RadiantSelectionSystem that does the actual selecting
	        	GlobalSelectionSystem().SelectPoint(*_view, &position[0], &_epsilon[0], modifier, (_state & c_modifier_face) != c_modifierNone);
	      	}
	    }
		
		// Reset the mouse position to zero, this mouse operation is finished so far
	    _start = _current = DeviceVector(0.0f, 0.0f);
	    draw_area();
	}

	// Returns true if the user is currently selecting something (i.e. if any modifieres are held) 
	bool selecting() const {
		return _state != c_modifier_manipulator;
	}

	// Sets the current state to <state>
	void setState(ModifierFlags state) {
		bool was_selecting = selecting();
		_state = state;
		if (was_selecting ^ selecting()) {
			draw_area();
		}
	}
	
	// Returns the current modifier flags
	ModifierFlags getState() const {
		return _state;
	}

	// Sets the specified modifier active
	void modifierEnable(ModifierFlags type) {
		setState(bitfield_enable(getState(), type));
	}
	
	// Sets the specified modifier inactive
	void modifierDisable(ModifierFlags type) {
		setState(bitfield_disable(getState(), type));
	}

	// onMouseDown: Save the current mouse position as start, the mouse operation is beginning now
  	void mouseDown(DeviceVector position) {
    	_start = _current = device_constrained(position);
  	}

	// onMouseMove: store the current position, and call the area draw update 
	void mouseMoved(DeviceVector position) {
		_current = device_constrained(position);
		draw_area();
	}
	
	// The mouseUp callback: check what has to be done and unconnect self from the calbacks
  	void mouseUp(DeviceVector position) {
  		// Check the result of this (finished) operation, is it a drag or a click?
	    testSelect(device_constrained(position));

		// Unconnect this method from the callbacks
    	g_mouseMovedCallback.clear();
		g_mouseUpCallback.clear();
  	}
  	
  	typedef MemberCaller1<SelectObserver, DeviceVector, &SelectObserver::mouseMoved> MouseMovedCaller;
  	typedef MemberCaller1<SelectObserver, DeviceVector, &SelectObserver::mouseUp> MouseUpCaller;
};

#endif /*SELECTOBSERVER_H_*/
