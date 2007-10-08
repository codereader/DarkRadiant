#ifndef KEYEVENT_H_
#define KEYEVENT_H_

#include "ieventmanager.h"
#include "generic/callback.h"

#include "gtk/gtkmenuitem.h"
#include "gdk/gdk.h"

#include "Event.h"

/* greebo: A KeyEvent is an object that contains a two callbacks, 
 * one for keyUp and one for keyDown events  
 */

typedef struct _GtkMenuItem GtkMenuItem;

class KeyEvent :
	public Event
{
	// The callbacks to be performed on keyDown/keyUp
	Callback _keyUpCallback;
	Callback _keyDownCallback;
	
public:
	KeyEvent(const Callback& keyUpCallback, const Callback& keyDownCallback) :
		_keyUpCallback(keyUpCallback),
		_keyDownCallback(keyDownCallback)
	{}

	virtual ~KeyEvent() {}
	
	virtual bool empty() const {
		return false;
	}
	
	void keyDown() {
		if (_enabled) {
			// Execute the command on key down event
			_keyDownCallback();
		}
	}
	
	void keyUp() {
		if (_enabled) {
			// Execute the command on key up event
			_keyUpCallback();
		}
	}

}; // class KeyEvent

#endif /*KEYEVENT_H_*/
