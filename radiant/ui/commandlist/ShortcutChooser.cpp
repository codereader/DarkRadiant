#include "ShortcutChooser.h"

#include "imainframe.h"
#include "i18n.h"
#include "idialogmanager.h"

#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/button.h>

#include <fmt/format.h>

namespace ui
{

ShortcutChooser::ShortcutChooser(const std::string& title,
								 wxWindow* parent,
								 const std::string& command) :
	wxutil::DialogBase(title, parent),
	_statusText(nullptr),
	_existingEventText(nullptr),
	_entry(nullptr),
	_commandName(command)
{
	wxBoxSizer* vbox = new wxBoxSizer(wxVERTICAL);
	SetSizer(vbox);

	wxStaticText* label = new wxStaticText(this, wxID_ANY, _commandName);
	label->SetFont(label->GetFont().Bold());

	_entry = new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition, 
		wxDefaultSize, wxTE_PROCESS_TAB | wxTE_PROCESS_ENTER);

	_entry->Bind(wxEVT_KEY_DOWN, &ShortcutChooser::onShortcutKeyPress, this);

	// The widget to display the status text
	_statusText = new wxStaticText(this, wxID_ANY, _("Note: This shortcut is already assigned to:"));
	_statusText->Hide();

	_existingEventText = new wxStaticText(this, wxID_ANY, "");
	_existingEventText->SetFont(_existingEventText->GetFont().Bold());
	_existingEventText->Hide();
	
	wxBoxSizer* buttonHBox = new wxBoxSizer(wxHORIZONTAL);

	// Create the close button
	wxButton* okButton = new wxButton(this, wxID_ANY, _("OK"));
	okButton->Bind(wxEVT_BUTTON, &ShortcutChooser::onOK, this);
	
	wxButton* cancelButton = new wxButton(this, wxID_ANY, _("Cancel"));
	cancelButton->Bind(wxEVT_BUTTON, &ShortcutChooser::onCancel, this);

	buttonHBox->Add(okButton, 0, wxRIGHT, 6);
	buttonHBox->Add(cancelButton, 0);

	vbox->Add(label, 0, wxALIGN_CENTER | wxALL, 12);
	vbox->Add(_entry, 0, wxEXPAND | wxBOTTOM | wxLEFT | wxRIGHT, 12);
	vbox->Add(_statusText, 0, wxEXPAND | wxLEFT | wxRIGHT, 12);
	vbox->Add(_existingEventText, 0, wxEXPAND | wxLEFT | wxRIGHT, 12);
	vbox->Add(buttonHBox, 0, wxALIGN_RIGHT | wxALL, 12);

	Fit();
	CenterOnParent();
}

void ShortcutChooser::onOK(wxCommandEvent& ev)
{
	EndModal(wxID_OK);
}

void ShortcutChooser::onCancel(wxCommandEvent& ev)
{
	EndModal(wxID_CANCEL);
}

void ShortcutChooser::onShortcutKeyPress(wxKeyEvent& ev)
{
	// Store the shortcut string representation into the Entry field
	_entry->SetValue(GlobalEventManager().getEventStr(ev));

	// Store this key/modifier combination for later use (UPPERCASE!)
	_savedKeyEvent = ev;

	auto currentlyBoundCommand = GlobalEventManager().findEventForAccelerator(ev);

	// Display the note if the found command differs from the one we're editing
	bool accelAlreadybound = !currentlyBoundCommand.empty() && currentlyBoundCommand != _commandName;

	if (accelAlreadybound)
	{
		_existingEventText->SetLabel(currentlyBoundCommand);
	}
	
	_statusText->Show(accelAlreadybound);
	_existingEventText->Show(accelAlreadybound);

	Fit();
	CenterOnParent();
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
	if (_savedKeyEvent.GetEventType() != wxEVT_NULL)
	{
		// Construct an eventkey structure to be passed to the EventManager query
		// Try to lookup an existing command with the same shortcut
		auto currentlyBoundCommand = GlobalEventManager().findEventForAccelerator(_savedKeyEvent);

		// Only react on non-empty and non-"self" events
		if (!currentlyBoundCommand.empty() && currentlyBoundCommand != _commandName)
		{
			// There is already a command connected to this shortcut, ask the user
			std::string message =
				fmt::format(_("The specified shortcut is already assigned to {0}"
				"\nOverwrite the current setting and assign this shortcut to {1} instead?"),
				currentlyBoundCommand, _commandName);

			// Fire up the dialog to ask the user what action to take
			IDialogPtr popup = GlobalDialogManager().createMessageBox(
				_("Overwrite existing shortcut?"), message, ui::IDialog::MESSAGE_ASK);

			// Only react on "YES"
			if (popup->run() == ui::IDialog::RESULT_YES)
			{
				// Disconnect both the found command and the new command
				GlobalEventManager().disconnectAccelerator(currentlyBoundCommand);
				GlobalEventManager().disconnectAccelerator(_commandName);

				// Create a new accelerator and connect it to the selected command
				GlobalEventManager().connectAccelerator(_savedKeyEvent, _commandName);

				shortcutsChanged = true;
			}
		}
		else
		{
			// No command is using the accelerator up to now, so assign it

			// Disconnect the current command to avoid duplicate accelerators
			GlobalEventManager().disconnectAccelerator(_commandName);

			// Create a new accelerator and connect it to the selected command
			GlobalEventManager().connectAccelerator(_savedKeyEvent, _commandName);

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
