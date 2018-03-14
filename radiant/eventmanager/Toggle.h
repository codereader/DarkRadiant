#pragma once

#include "ieventmanager.h"
#include <functional>

#include <sigc++/connection.h>
#include "Event.h"

#include <wx/event.h>

class wxToggleButton;
class wxCommandEvent;

namespace ui
{

/* greebo: A Toggle object has a state (toggled = TRUE/FALSE) and a callback that
 * is invoked on toggle.
 *
 * A Toggle can be connected to a wxMenuItem/wxToggleButton/wxToolBarToolBase via the 
 * corresponding method.
 *
 * Use the updateWidget() method to export the current state of the Toggle object to
 * the connected widgets.
 */
class Toggle :
	public Event,
	public wxEvtHandler
{
private:
	// The callback to be performed on toggle()
	ToggleCallback _callback;

protected:
	typedef std::set<wxMenuItem*> MenuItems;
	MenuItems _menuItems;

	typedef std::set<wxToolBarToolBase*> ToolItems;
	ToolItems _toolItems;

	typedef std::set<wxToggleButton*> Buttons;
	Buttons _buttons;

	bool _callbackActive;

	// The toggled state of this object
	bool _toggled;

public:
	Toggle(const ToggleCallback& callback);

	virtual ~Toggle();

	// Returns usually false, because a Toggle is never empty
	virtual bool empty() const;

	// Set the toggled state to true/false, according to <toggled> and update
	// any associated widgets or notify any callbacks.
	virtual bool setToggled(const bool toggled);

	// Update the "active" state of the connected widgets
	virtual void updateWidgets();

	// On key press >> toggle the internal state
	virtual void keyDown();

	// Returns true if the internal state is true
	bool isToggled() const;

	// Returns true for this and all derived classes
	virtual bool isToggle() const;

	virtual void connectMenuItem(wxMenuItem* item);
	virtual void disconnectMenuItem(wxMenuItem* item);

	virtual void connectToolItem(wxToolBarToolBase* item);
	virtual void disconnectToolItem(wxToolBarToolBase* item);

	virtual void connectToggleButton(wxToggleButton* button);
	virtual void disconnectToggleButton(wxToggleButton* button);

	// Invoke the registered callback and update/notify
	virtual void toggle();

    virtual void connectAccelerator(IAccelerator& accel);
    virtual void disconnectAccelerators();

protected:
	virtual void onMenuItemClicked(wxCommandEvent& ev);
	virtual void onToolItemClicked(wxCommandEvent& ev);
	virtual void onToggleButtonClicked(wxCommandEvent& ev);

}; // class Toggle
typedef std::shared_ptr<Toggle> TogglePtr;

}
