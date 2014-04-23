#include "ShortcutChooser.h"

#include "imainframe.h"
#include "i18n.h"

#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>

#include <boost/format.hpp>

namespace ui
{

ShortcutChooser::ShortcutChooser(const std::string& title,
								 wxWindow* parent,
								 const std::string& command) :
	wxutil::DialogBase(title, parent),
	_statusWidget(NULL),
	_entry(NULL),
	_keyval(0),
	_state(0),
	_commandName(command),
	_event(GlobalEventManager().findEvent(_commandName))
{
	wxBoxSizer* vbox = new wxBoxSizer(wxVERTICAL);

	wxStaticText* label = new wxStaticText(this, wxID_ANY, _commandName);
	label->SetFont(label->GetFont().Bold());

	_entry = new wxTextCtrl(this, wxID_ANY);
	_entry->Connect(wxEVT_KEY_DOWN, wxKeyEventHandler(ShortcutChooser::onShortcutKeyPress), NULL, this);

	// The widget to display the status text
	_statusWidget = new wxStaticText(this, wxID_ANY, "");
	
	vbox->Add(label, 0, wxALIGN_CENTER | wxALL, 12);
	vbox->Add(_entry, 0, wxEXPAND | wxBOTTOM, 12);
	vbox->Add(_statusWidget, 0, wxEXPAND | wxBOTTOM, 12);
	vbox->Add(CreateStdDialogButtonSizer(wxOK | wxCANCEL), 0, wxALIGN_RIGHT | wxBOTTOM, 12);
}

void ShortcutChooser::onShortcutKeyPress(wxKeyEvent& ev)
{
	std::string statusText("");
#if 0
	// Store the shortcut string representation into the Entry field
	_entry->SetValue(GlobalEventManager().getGDKEventStr(ev));


	// Store this key/modifier combination for later use (UPPERCASE!)
	_keyval = gdk_keyval_to_upper(ev->keyval);
	_state = ev->state;

	IEventPtr foundEvent = GlobalEventManager().findEvent(ev);

	// Only display the note if any event was found and it's not the "self" event
	if (!foundEvent->empty() && foundEvent != _event)
	{
		statusText = (boost::format(_("Note: This is already assigned to: <b>%s</b>")) %
					  GlobalEventManager().getEventName(foundEvent)).str();
	}

	_statusWidget->set_markup(statusText);
#endif
}

int ShortcutChooser::ShowModal()
{
	int result = DialogBase::ShowModal();

	if (result == wxID_OK)
	{
		assignShortcut();
	}

	return result;
}

bool ShortcutChooser::assignShortcut()
{
	bool shortcutsChanged = false;

	// Check, if the user has pressed a meaningful key
	if (_keyval != 0)
	{
		// Construct an eventkey structure to be passed to the EventManager query
		GdkEventKey eventKey;

		eventKey.keyval = _keyval;
		eventKey.state = _state;

		// Try to lookup an existing command with the same shortcut
		IEventPtr foundEvent = GlobalEventManager().findEvent(&eventKey);

		// Only react on non-empty and non-"self" events
		if (!foundEvent->empty() && foundEvent != _event)
		{
			// There is already a command connected to this shortcut, ask the user
			const std::string foundEventName = GlobalEventManager().getEventName(foundEvent);

			// Construct the message
			std::string message =
				(boost::format(_("The specified shortcut is already assigned to <b>%s</b>"
				"\nOverwrite the current setting and assign this shortcut to <b>%s</b> instead?")) %
				foundEventName % _commandName).str();

			// Fire up the dialog to ask the user what action to take
			IDialogPtr popup = GlobalDialogManager().createMessageBox(
				_("Overwrite existing shortcut?"), message, ui::IDialog::MESSAGE_ASK);

			// Only react on "YES"
			if (popup->run() == ui::IDialog::RESULT_YES)
			{
				// Disconnect both the found command and the new command
				GlobalEventManager().disconnectAccelerator(foundEventName);
				GlobalEventManager().disconnectAccelerator(_commandName);

				// Create a new accelerator and connect it to the selected command
				IAccelerator& accel = GlobalEventManager().addAccelerator(&eventKey);
				GlobalEventManager().connectAccelerator(accel, _commandName);

				shortcutsChanged = true;
			}
		}
		else
		{
			// No command is using the accelerator up to now, so assign it

			// Disconnect the current command to avoid duplicate accelerators
			GlobalEventManager().disconnectAccelerator(_commandName);

			// Create a new accelerator and connect it to the selected command
			IAccelerator& accel = GlobalEventManager().addAccelerator(&eventKey);
			GlobalEventManager().connectAccelerator(accel, _commandName);

			shortcutsChanged = true;
		}
	}
	else
	{
		// No key is specified, disconnect the command
		GlobalEventManager().disconnectAccelerator(_commandName);

		shortcutsChanged = true;
	}

	return shortcutsChanged;
}

} // namespace ui
