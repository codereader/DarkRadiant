#include "SelectionSetToolmenu.h"

#include "i18n.h"
#include "iselection.h"
#include "ieventmanager.h"
#include "iuimanager.h"
#include "imap.h"
#include "iradiant.h"
#include "itextstream.h"
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
	_dropdownToolId(wxID_NONE)
{
	_dropdown = new wxComboBox(toolbar, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0, NULL, wxTE_PROCESS_ENTER);

	// Add tooltip
	_dropdown->SetHelpText(_(ENTRY_TOOLTIP));

	// Connect the signals
	_dropdown->Bind(wxEVT_TEXT_ENTER, &SelectionSetToolmenu::onEntryActivated, this);
	_dropdown->Bind(wxEVT_COMBOBOX, &SelectionSetToolmenu::onSelectionChanged, this);

	auto dropdownTool = toolbar->AddControl(_dropdown);
	_dropdownToolId = dropdownTool->GetId();

	_mapEventHandler = GlobalMapModule().signal_mapEvent().connect(
		sigc::mem_fun(*this, &SelectionSetToolmenu::onMapEvent)
	);

	GlobalRadiant().signal_radiantShutdown().connect(
		sigc::mem_fun(*this, &SelectionSetToolmenu::onRadiantShutdown)
	);

	connectToMapRoot();
	update();
}

void SelectionSetToolmenu::onRadiantShutdown()
{
	if (_dropdownToolId != wxID_NONE)
	{
		auto toolbar = static_cast<wxToolBar*>(_dropdown->GetParent());
		toolbar->DeleteTool(_dropdownToolId);
	}

	_dropdown = nullptr;

	_mapEventHandler.disconnect();
}

void SelectionSetToolmenu::onMapEvent(IMap::MapEvent ev)
{
	if (ev == IMap::MapLoaded)
	{
		// Rebuild the dialog once a map is loaded
		connectToMapRoot();
		update();
	}
	else if (ev == IMap::MapUnloading)
	{
		disconnectFromMapRoot();
		update();
	}
}

void SelectionSetToolmenu::connectToMapRoot()
{
	// Always disconnect first
	disconnectFromMapRoot();

	if (GlobalMapModule().getRoot())
	{
		auto& setManager = GlobalMapModule().getRoot()->getSelectionSetManager();

		_setsChangedSignal = setManager.signal_selectionSetsChanged().connect(
			sigc::mem_fun(this, &SelectionSetToolmenu::update));
	}
}

void SelectionSetToolmenu::disconnectFromMapRoot()
{
	_setsChangedSignal.disconnect();
}

void SelectionSetToolmenu::update()
{
	_dropdown->Clear();

	auto root = GlobalMapModule().getRoot();

	if (!root)
	{
		return;
	}

	root->getSelectionSetManager().foreachSelectionSet([&] (const ISelectionSetPtr& set)
	{
		_dropdown->Append(set->getName());
	});
}

void SelectionSetToolmenu::onEntryActivated(wxCommandEvent& ev)
{
	auto root = GlobalMapModule().getRoot();

	if (!root)
	{
		rError() << "No map loaded, cannot create selection sets" << std::endl;
		return;
	}

	// Create new selection set if possible
	std::string name = _dropdown->GetValue().ToStdString();

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

	auto set = root->getSelectionSetManager().createSelectionSet(name);

	assert(set);

	set->assignFromCurrentScene();

	// Clear the entry again
	_dropdown->SetValue("");

	// Unset our focus
	wxGetTopLevelParent(_dropdown)->SetFocus();
}

void SelectionSetToolmenu::onSelectionChanged(wxCommandEvent& ev)
{
	auto root = GlobalMapModule().getRoot();

	if (!root)
	{
		rError() << "No map loaded, cannot select or deselect sets" << std::endl;
		return;
	}

	std::string name = _dropdown->GetStringSelection().ToStdString();

	if (name.empty()) return;

	auto set = root->getSelectionSetManager().findSelectionSet(name);

	if (!set) return;

	// The user can choose to DESELECT the set nodes when holding down shift
	if (wxGetKeyState(WXK_SHIFT))
	{
		set->deselect();
	}
	else
	{
		set->select();
	}

	_dropdown->SetValue("");

	// Unset our focus
	wxGetTopLevelParent(_dropdown)->SetFocus();
}

} // namespace
