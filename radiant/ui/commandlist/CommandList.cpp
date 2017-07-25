#include "CommandList.h"

#include "i18n.h"
#include "imainframe.h"
#include "iuimanager.h"

#include <wx/sizer.h>
#include <wx/button.h>

#include "wxutil/dialog/MessageBox.h"

#include "CommandListPopulator.h"
#include "ShortcutChooser.h"

namespace ui
{

namespace
{
	const char* const CMDLISTDLG_WINDOW_TITLE = N_("Shortcut List");
}

CommandList::CommandList() :
	wxutil::DialogBase(_(CMDLISTDLG_WINDOW_TITLE), GlobalMainFrame().getWxTopLevelWindow()),
	_listStore(NULL),
	_treeView(NULL)
{
	// Create all the widgets
	populateWindow();

	FitToScreen(0.4f, 0.7f);
}

void CommandList::reloadList()
{
	_listStore->Clear();

	// Instantiate the visitor class with the target list store
	CommandListPopulator populator(_listStore, _columns);

	// Cycle through all the events and create the according list items
	GlobalEventManager().foreachEvent(populator);
    
    _treeView->TriggerColumnSizeEvent();
}

void CommandList::populateWindow()
{
	wxBoxSizer* hbox = new wxBoxSizer(wxHORIZONTAL);
	SetSizer(hbox);

	// Create a new liststore item and define its columns
	_listStore = new wxutil::TreeModel(_columns, true);

	_treeView = wxutil::TreeView::CreateWithModel(this, _listStore);
	
	_treeView->AppendTextColumn(_("Command"), _columns.command.getColumnIndex(),
		wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE, wxALIGN_NOT, wxDATAVIEW_COL_SORTABLE);
	_treeView->AppendTextColumn(_("Key"), _columns.key.getColumnIndex(),
		wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE, wxALIGN_NOT, wxDATAVIEW_COL_SORTABLE);

	// Connect the mouseclick event to catch the double clicks
	_treeView->Connect(wxEVT_DATAVIEW_SELECTION_CHANGED, 
		wxDataViewEventHandler(CommandList::onSelectionChanged), NULL, this);

	_treeView->Connect(wxEVT_DATAVIEW_ITEM_ACTIVATED, 
		wxDataViewEventHandler(CommandList::onItemDoubleClicked), NULL, this);

	_treeView->AddSearchColumn(_columns.command);

	// Load the list items into the treeview
	reloadList();

	wxBoxSizer* buttonVBox = new wxBoxSizer(wxVERTICAL);

	// Create the close button
	wxButton* closeButton = new wxButton(this, wxID_CLOSE);
	closeButton->Connect(wxEVT_BUTTON, wxCommandEventHandler(CommandList::onClose), NULL, this);
	closeButton->SetMinClientSize(wxSize(80, -1));

	// Create the assign shortcut button
	_assignButton = new wxButton(this, wxID_EDIT);
	_assignButton->Connect(wxEVT_BUTTON, wxCommandEventHandler(CommandList::onAssign), NULL, this);
	_assignButton->Enable(false);

	// Create the clear shortcut button
	_clearButton = new wxButton(this, wxID_CLEAR);
	_clearButton->Connect(wxEVT_BUTTON, wxCommandEventHandler(CommandList::onClear), NULL, this);
	_clearButton->Enable(false);

	// Create the clear shortcut button
	wxButton* resetButton = new wxButton(this, wxID_ANY, _("Reset to Default"));
	resetButton->Connect(wxEVT_BUTTON, wxCommandEventHandler(CommandList::onResetToDefault), NULL, this);

	buttonVBox->Add(resetButton, 0, wxEXPAND | wxBOTTOM, 12);
	buttonVBox->Add(_clearButton, 0, wxEXPAND | wxBOTTOM, 6);
	buttonVBox->Add(_assignButton, 0, wxEXPAND | wxBOTTOM, 6);
	buttonVBox->Add(closeButton, 0, wxEXPAND);

	hbox->Add(_treeView, 1, wxEXPAND | wxALL, 12);
	hbox->Add(buttonVBox, 0, wxALIGN_BOTTOM | wxRIGHT | wxTOP | wxBOTTOM, 12);
}

std::string CommandList::getSelectedCommand()
{
	wxDataViewItem item = _treeView->GetSelection();

	if (item.IsOk())
	{
		wxutil::TreeModel::Row row(item, *_listStore);

		const std::string commandName = row[_columns.command];

		IEventPtr ev = GlobalEventManager().findEvent(commandName);

		// Double check, if the command exists
		if (ev != NULL)
		{
			return commandName;
		}
	}

	return "";
}

void CommandList::assignShortcut()
{
	std::string command = getSelectedCommand();

	if (command.empty()) return;

	// Instantiate the helper dialog
	ShortcutChooser* chooser = new ShortcutChooser(_("Enter new Shortcut"), this, command);

	if (chooser->ShowModal() == wxID_OK)
	{
		// The chooser returned OK, update the list
		reloadList();
	}

	chooser->Destroy();
}

void CommandList::onAssign(wxCommandEvent& ev)
{
	assignShortcut();
	updateButtonState();
}

void CommandList::onItemDoubleClicked(wxDataViewEvent& ev)
{
	assignShortcut();
	updateButtonState();
}

void CommandList::updateButtonState()
{
	wxDataViewItem item = _treeView->GetSelection();

	_clearButton->Enable(item.IsOk());
	_assignButton->Enable(item.IsOk());
}

void CommandList::onSelectionChanged(wxDataViewEvent& ev)
{
	updateButtonState();
}

void CommandList::onClear(wxCommandEvent& ev)
{
	const std::string commandName = getSelectedCommand();

	if (!commandName.empty())
	{
		// Disconnect the event and update the list
		GlobalEventManager().disconnectAccelerator(commandName);
		reloadList();
		updateButtonState();
	}
}

void CommandList::onResetToDefault(wxCommandEvent& ev)
{
	IDialog::Result result = wxutil::Messagebox::Show(_("Reset to default?"),
		_("Really clear all bindings and reload\nthem from the default settings?"), IDialog::MESSAGE_ASK);

	if (result == IDialog::RESULT_YES)
	{
		// Reset and reload
		GlobalEventManager().resetAcceleratorBindings();

		reloadList();
		updateButtonState();
	}
}

void CommandList::onClose(wxCommandEvent& ev)
{
	EndModal(wxCLOSE);
}

void CommandList::ShowDialog(const cmd::ArgumentList& args)
{
	CommandList* dialog = new CommandList;
	
	dialog->ShowModal();
	dialog->Destroy();
}

} // namespace ui
