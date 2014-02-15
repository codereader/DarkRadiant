#pragma once

#include "ieventmanager.h"

#include <wx/event.h>
#include <sigc++/connection.h>
#include "Event.h"

/* greebo: A Statement is an object that executes a command string when invoked.
 *
 * Trigger the Statement via the execute() method (usually done by the associated accelerator).
 *
 * Connect the statement to a GtkToolButton / GtkButton / GtkMenuItem via the connectWidget method.
 */
class Statement :
	public Event,
	public wxEvtHandler
{
private:
	// The statement to execute
	std::string _statement;

	// Whether this Statement reacts on keyup or keydown
	bool _reactOnKeyUp;

	typedef std::map<Gtk::Widget*, sigc::connection> WidgetList;
	WidgetList _connectedWidgets;

	typedef std::set<wxMenuItem*> MenuItems;
	MenuItems _menuItems;

public:
	Statement(const std::string& statement, bool reactOnKeyUp = false);

	virtual ~Statement();

	// Invoke the registered callback
	virtual void execute();

	// Override the derived keyDown/keyUp method
	virtual void keyUp();
	virtual void keyDown();

	// Connect the given menuitem/toolbutton to this Statement
	virtual void connectWidget(Gtk::Widget* widget);
	virtual void connectMenuItem(wxMenuItem* item);
	virtual void disconnectMenuItem(wxMenuItem* item);

	virtual bool empty() const;

private:
	// The gtkmm callback methods that can be connected to a ToolButton or a MenuItem
	void onButtonPress();
	void onToolButtonPress();
	void onMenuItemClicked();
	void onWxMenuItemClicked(wxCommandEvent& ev);

}; // class Statement
