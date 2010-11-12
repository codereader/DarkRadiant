#ifndef KEYEVENT_H_
#define KEYEVENT_H_

#include "ieventmanager.h"
#include "Event.h"

/**
 * greebo: A KeyEvent is an object that contains a state change callback,
 * for tracking the keyUp and keyDown events.
 */
class KeyEvent :
	public Event
{
private:
	// The callbacks to be performed on keyDown/keyUp
	ui::KeyStateChangeCallback _keyStateChangeCallback;

public:
	KeyEvent(const ui::KeyStateChangeCallback& keyStateChangeCallback) :
		_keyStateChangeCallback(keyStateChangeCallback)
	{}

	virtual bool empty() const
	{
		return false;
	}

	void keyDown()
	{
		if (_enabled) {
			// Execute the command on key down event
			_keyStateChangeCallback(ui::KeyPressed);
		}
	}

	void keyUp()
	{
		if (_enabled) {
			// Execute the command on key up event
			_keyStateChangeCallback(ui::KeyReleased);
		}
	}

}; // class KeyEvent

#endif /*KEYEVENT_H_*/
