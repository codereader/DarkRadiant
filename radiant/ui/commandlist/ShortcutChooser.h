#pragma once

#include <string>
#include "ieventmanager.h"
#include "wxutil/dialog/DialogBase.h"
#include <wx/event.h>

class wxStaticText;
class wxTextCtrl;

/**
 * greebo: The Shortcutchooser takes care of displaying the dialog and
 * re-assigning the events after the shortcut has been entered by the user.
 */
namespace ui
{

class ShortcutChooser :
	public wxutil::DialogBase
{
private:
	// The label to hold the status text of the shortcut chooser
	wxStaticText* _statusText;
	wxStaticText* _existingEventText;
	wxTextCtrl* _entry;

	// Working variable to store the new key/modifier from the user input
	wxKeyEvent _savedKeyEvent;

	// The event name the shortcut will be assigned to
	std::string _commandName;
	IEventPtr _event;

public:
	// Constructor, instantiate this class by specifying the parent window
	ShortcutChooser(const std::string& title,
					wxWindow* parent,
					const std::string& command);

	virtual int ShowModal();

private:
	// Assigns or unassigns shortcut, based on the user's input (called after OK)
	// returns true if the shortcut actually got changed.
	bool assignShortcut();

	// The callback for catching the keypress events in the shortcut entry field
	void onShortcutKeyPress(wxKeyEvent& ev);
	void onOK(wxCommandEvent& ev);
	void onCancel(wxCommandEvent& ev);

}; // class ShortcutChooser

} // namespace ui
