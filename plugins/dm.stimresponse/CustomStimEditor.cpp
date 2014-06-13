#include "CustomStimEditor.h"

#include "idialogmanager.h"

#include "string/convert.h"
#include "i18n.h"

#include "gtkutil/menu/IconTextMenuItem.h"
#include <wx/menu.h>
#include <wx/textctrl.h>
#include <wx/stattext.h>
#include <wx/button.h>
#include <wx/sizer.h>

namespace ui
{

namespace
{
	const int TREE_VIEW_WIDTH = 250;
	const int TREE_VIEW_HEIGHT = 200;
}

CustomStimEditor::CustomStimEditor(wxWindow* parent, StimTypes& stimTypes) :
	wxPanel(parent, wxID_ANY),
	_customStimStore(NULL),
	_list(NULL),
	_stimTypes(stimTypes),
	_updatesDisabled(false)
{
	populatePage();

	// Setup the context menu items and connect them to the callbacks
	createContextMenu();

	// The list may be empty, update the sensitivity
	update();
}

void CustomStimEditor::setEntity(const SREntityPtr& entity)
{
	_entity = entity;
}

void CustomStimEditor::createContextMenu()
{
	// Menu widgets (is not packed, hence create a shared_ptr)
	_contextMenu.menu.reset(new wxMenu);

	_contextMenu.add = 
		_contextMenu.menu->Append(new wxutil::StockIconTextMenuItem(_("Add"), wxART_PLUS));
	_contextMenu.remove = 
		_contextMenu.menu->Append(new wxutil::StockIconTextMenuItem(_("Delete"), wxART_MINUS));

	// Connect up the signals
	_contextMenu.menu->Connect(_contextMenu.remove->GetId(), wxEVT_MENU, 
		wxCommandEventHandler(CustomStimEditor::onContextMenuDelete), NULL, this);
	_contextMenu.menu->Connect(_contextMenu.add->GetId(), wxEVT_MENU, 
		wxCommandEventHandler(CustomStimEditor::onContextMenuAdd), NULL, this);
}

void CustomStimEditor::populatePage()
{
	// Add a 6 pixel border around everything
	SetSizer(new wxBoxSizer(wxVERTICAL));

	wxBoxSizer* hbox = new wxBoxSizer(wxHORIZONTAL);
	GetSizer()->Add(hbox, 1, wxEXPAND | wxALL, 6);

	// Setup a treemodel filter to display the custom stims only
	//_customStimStore = new wxutil::TreeModelFilter(_stimTypes.getListStore());
	_customStimStore = _stimTypes.getListStore();

	// wxTODO _customStimStore->set_visible_column(_stimTypes.getColumns().isCustom);

	_list = wxutil::TreeView::Create(this);
	_list->AssociateModel(_customStimStore);
	_list->SetMinClientSize(wxSize(TREE_VIEW_WIDTH, TREE_VIEW_HEIGHT));

	// Connect the signals
	_list->Connect(wxEVT_DATAVIEW_SELECTION_CHANGED,
        wxDataViewEventHandler(CustomStimEditor::onSelectionChange), NULL, this);
	_list->Connect(wxEVT_DATAVIEW_ITEM_CONTEXT_MENU, 
		wxDataViewEventHandler(CustomStimEditor::onContextMenu), NULL, this);

	// Add the columns to the treeview
	// ID number
	_list->AppendTextColumn("ID", _stimTypes.getColumns().id.getColumnIndex(), 
		wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE, wxALIGN_NOT);

	// The Type
	_list->AppendIconTextColumn(_("Type"), _stimTypes.getColumns().caption.getColumnIndex(), 
		wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE, wxALIGN_NOT);

	wxBoxSizer* listVBox = new wxBoxSizer(wxVERTICAL);
	listVBox->Add(_list, 1, wxEXPAND | wxBOTTOM, 6);
	listVBox->Add(createListButtons(), 0, wxEXPAND);

	_propertyWidgets.vbox = new wxPanel(this, wxID_ANY);
	_propertyWidgets.vbox->SetSizer(new wxBoxSizer(wxVERTICAL));

	hbox->Add(listVBox, 0, wxEXPAND | wxRIGHT, 12);
	hbox->Add(_propertyWidgets.vbox, 1, wxEXPAND);

	// The name widgets
	wxBoxSizer* nameHBox = new wxBoxSizer(wxHORIZONTAL);
	_propertyWidgets.nameLabel = new wxStaticText(_propertyWidgets.vbox, wxID_ANY, _("Name:"));
	_propertyWidgets.nameEntry = new wxTextCtrl(_propertyWidgets.vbox, wxID_ANY);

	nameHBox->Add(_propertyWidgets.nameLabel, 0, wxRIGHT | wxALIGN_CENTER_VERTICAL, 6);
	nameHBox->Add(_propertyWidgets.nameEntry, 1, wxEXPAND);

	// Connect the entry field
	_propertyWidgets.nameEntry->Connect(wxEVT_TEXT,
		wxCommandEventHandler(CustomStimEditor::onEntryChanged), NULL, this);

	wxStaticText* infoText = new wxStaticText(_propertyWidgets.vbox, wxID_ANY,
		_("Note: Please beware that deleting custom stims may\n"
		"affect other entities as well. So check before you delete.")
	);

	_propertyWidgets.vbox->GetSizer()->Add(nameHBox, 0, wxEXPAND | wxBOTTOM, 12);
	_propertyWidgets.vbox->GetSizer()->Add(infoText, 0);
}

wxBoxSizer* CustomStimEditor::createListButtons()
{
	wxBoxSizer* hbox = new wxBoxSizer(wxHORIZONTAL);

	_listButtons.add = new wxButton(this, wxID_ANY, _("Add Stim Type"));
	_listButtons.remove = new wxButton(this, wxID_ANY, _("Remove Stim Type"));

	hbox->Add(_listButtons.add, 1, wxRIGHT, 6);
	hbox->Add(_listButtons.remove, 1);

	_listButtons.add->Connect(wxEVT_BUTTON, wxCommandEventHandler(CustomStimEditor::onAddStimType), NULL, this);
	_listButtons.remove->Connect(wxEVT_BUTTON, wxCommandEventHandler(CustomStimEditor::onRemoveStimType), NULL, this);
	
	return hbox;
}

void CustomStimEditor::update()
{
	_updatesDisabled = true;

	int id = getIdFromSelection();

	if (id > 0)
	{
		_propertyWidgets.vbox->Enable(true);

		StimType stimType = _stimTypes.get(id);
		_propertyWidgets.nameEntry->SetValue(stimType.caption);

		_contextMenu.menu->Enable(_contextMenu.remove->GetId(), true);
	}
	else
	{
		_propertyWidgets.vbox->Enable(false);
		_contextMenu.menu->Enable(_contextMenu.remove->GetId(), false);
	}

	_updatesDisabled = false;
}

void CustomStimEditor::selectId(int id)
{
	// Setup the selectionfinder to search for the id
	wxDataViewItem item = _customStimStore->FindInteger(id, _stimTypes.getColumns().id.getColumnIndex());

	if (item)
	{
		// Set the active row of the list to the given stim
		_list->Select(item);
	}
}

void CustomStimEditor::addStimType()
{
	// Add a new stim type with the lowest free custom id
	int id = _stimTypes.getFreeCustomStimId();

	_stimTypes.add(id,
				   string::to_string(id),
				   "CustomStimType",
				   _("Custom Stim"),
				   ICON_CUSTOM_STIM,
				   true);

	selectId(id);
}

int CustomStimEditor::getIdFromSelection()
{
	wxDataViewItem item = _list->GetSelection();

	if (item.IsOk()) 
	{
		wxutil::TreeModel::Row row(item, *_customStimStore);
		return row[_stimTypes.getColumns().id].getInteger();
	}
	
	return -1;
}

void CustomStimEditor::removeStimType()
{
	IDialogPtr dialog = GlobalDialogManager().createMessageBox(_("Delete Custom Stim"),
		_("Beware that other entities might still be using this stim type.\n"
		"Do you really want to delete this custom stim?"), ui::IDialog::MESSAGE_ASK);

	if (dialog->run() == IDialog::RESULT_YES)
	{
		_stimTypes.remove(getIdFromSelection());
	}
}

void CustomStimEditor::onAddStimType(wxCommandEvent& ev)
{
	addStimType();
}

void CustomStimEditor::onRemoveStimType(wxCommandEvent& ev)
{
	// Delete the selected stim type from the list
	removeStimType();
}

void CustomStimEditor::onEntryChanged(wxCommandEvent& ev)
{
	if (_updatesDisabled) return; // Callback loop guard

	// Set the caption of the curently selected stim type
	std::string caption = _propertyWidgets.nameEntry->GetValue().ToStdString();

	// Pass the call to the helper class
	_stimTypes.setStimTypeCaption(getIdFromSelection(), caption);

	if (_entity != NULL)
	{
		_entity->updateListStores();
	}
}

void CustomStimEditor::onSelectionChange(wxDataViewEvent& ev)
{
	update();
}

// Delete context menu items activated
void CustomStimEditor::onContextMenuDelete(wxCommandEvent& ev)
{
	// Delete the selected stim from the list
	removeStimType();
}

// Delete context menu items activated
void CustomStimEditor::onContextMenuAdd(wxCommandEvent& ev)
{
	addStimType();
}

void CustomStimEditor::onContextMenu(wxDataViewEvent& ev)
{
	_list->PopupMenu(_contextMenu.menu.get());
}

} // namespace ui
