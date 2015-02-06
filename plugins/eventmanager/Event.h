#pragma once

#include <typeinfo>
#include <iostream>

/* The base class for an Event.
 *
 * Provides methods to enable/disable the event and to connect GtkWidgets
 */

class Event :
	public IEvent
{
protected:
	// If false, the command is ignored and not executed.
	bool _enabled;

public:

	// Constructor
	Event() :
		_enabled(true)
	{}

	virtual ~Event() {}

	// Enables/disables this command according to the passed bool <enabled>
	virtual void setEnabled(const bool enabled) {
		_enabled = enabled;
	}

	virtual bool isToggle() const {
		return false;
	}

	virtual bool setToggled(const bool toggled) {
		return false;
	}

	virtual bool empty() const {
		// This is the base class for an event, it's empty by default
		return true;
	}

	virtual void keyUp() {}
	virtual void keyDown() {}

	virtual void updateWidgets() {}

	// Empty standard implementation
	virtual void connectTopLevelWindow(wxTopLevelWindow* widget) {}
	virtual void disconnectTopLevelWindow(wxTopLevelWindow* widget) {}

	virtual void connectMenuItem(wxMenuItem* item) {}
	virtual void disconnectMenuItem(wxMenuItem* item) {}

	virtual void connectToolItem(wxToolBarToolBase* item) {}
	virtual void disconnectToolItem(wxToolBarToolBase* item) {}

	virtual void connectButton(wxButton* button) {}
	virtual void disconnectButton(wxButton* button) {}

	virtual void connectToggleButton(wxToggleButton* button) {}
	virtual void disconnectToggleButton(wxToggleButton* button) {}

    virtual void connectAccelerator(IAccelerator& accel) {}
    virtual void disconnectAccelerators() {}

}; // class Event
