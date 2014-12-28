#include "SelectionSetToolmenu.h"

#include "i18n.h"
#include "iselection.h"
#include "ieventmanager.h"
#include "iuimanager.h"
#include "idialogmanager.h"

#include <wx/combobox.h>
#include <wx/toolbar.h>
#include <wx/sizer.h>

namespace selection
{

namespace
{
	const char* const ENTRY_TOOLTIP = N_("Enter a name and hit ENTER to save a set.\n\n"
		"Select an item from the dropdown list to restore the selection.\n\n"
		"Hold SHIFT when opening the dropdown list and selecting the item to de-select the set.");
}

SelectionSetToolmenu::SelectionSetToolmenu(wxToolBar* toolbar) :
	wxComboBox(toolbar, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, wxTE_PROCESS_ENTER)
{
	// Add tooltip
	SetHelpText(_(ENTRY_TOOLTIP));

	// Connect the signals
	Connect(wxEVT_TEXT_ENTER, wxCommandEventHandler(SelectionSetToolmenu::onEntryActivated), NULL, this);
	Connect(wxEVT_COMBOBOX, wxCommandEventHandler(SelectionSetToolmenu::onSelectionChanged), NULL, this);

	// Populate the list
	update();

	GlobalSelectionSetManager().signal_selectionSetsChanged().connect(
        sigc::mem_fun(this, &SelectionSetToolmenu::update)
    );
}

void SelectionSetToolmenu::update()
{
	Clear();

	GlobalSelectionSetManager().foreachSelectionSet([&] (const ISelectionSetPtr& set)
	{
		Append(set->getName());
	});
}

void SelectionSetToolmenu::onEntryActivated(wxCommandEvent& ev)
{
	// Create new selection set if possible
	std::string name = GetValue().ToStdString();

	if (name.empty()) return;

	// don't create empty sets
	if (GlobalSelectionSystem().countSelected() == 0)
	{
		ui::IDialogPtr dialog = GlobalDialogManager().createMessageBox(
			_("Cannot create selection set"),
			_("Cannot create a selection set, there is nothing selected in the current scene."),
			ui::IDialog::MESSAGE_CONFIRM);

		dialog->run();
		return;
	}

	ISelectionSetPtr set = GlobalSelectionSetManager().createSelectionSet(name);

	assert(set != NULL);

	set->assignFromCurrentScene();

	// Clear the entry again
	SetValue("");

	// Unset our focus
	wxGetTopLevelParent(this)->SetFocus();
}

void SelectionSetToolmenu::onSelectionChanged(wxCommandEvent& ev)
{
	std::string name = GetStringSelection().ToStdString();

	if (name.empty()) return;

	ISelectionSetPtr set = GlobalSelectionSetManager().findSelectionSet(name);

	if (set == NULL) return;

	// The user can choose to DESELECT the set nodes when holding down shift
	if (wxGetKeyState(WXK_SHIFT))
	{
		set->deselect();
	}
	else
	{
		set->select();
	}

	SetValue("");

	// Unset our focus
	wxGetTopLevelParent(this)->SetFocus();
}

} // namespace
