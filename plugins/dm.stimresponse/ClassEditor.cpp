#include "ClassEditor.h"

#include "string/convert.h"
#include <iostream>
#include "i18n.h"

#include <wx/bmpcbox.h>
#include <wx/sizer.h>
#include <wx/spinctrl.h>
#include <wx/button.h>
#include <wx/textctrl.h>
#include <wx/checkbox.h>

namespace ui
{

namespace
{
	const int TREE_VIEW_WIDTH = 320;
	const int TREE_VIEW_HEIGHT = 160;
}

ClassEditor::ClassEditor(wxWindow* parent, StimTypes& stimTypes) :
	wxPanel(parent, wxID_ANY),
	_list(NULL),
	_stimTypes(stimTypes),
	_updatesDisabled(false),
	_type(NULL),
	_addType(NULL),
	_overallHBox(NULL)
{
	SetSizer(new wxBoxSizer(wxVERTICAL));

	_overallHBox = new wxBoxSizer(wxHORIZONTAL);
	GetSizer()->Add(_overallHBox, 1, wxEXPAND | wxALL, 6);

	wxBoxSizer* vbox = new wxBoxSizer(wxVERTICAL);
	_overallHBox->Add(vbox, 0, wxEXPAND | wxRIGHT, 12);

	_list = wxutil::TreeView::Create(parent);
	_list->SetMinClientSize(wxSize(TREE_VIEW_WIDTH, TREE_VIEW_HEIGHT));
	vbox->Add(_list, 1, wxEXPAND | wxBOTTOM, 6);

	// Connect the signals to the callbacks
	_list->Connect(wxEVT_DATAVIEW_SELECTION_CHANGED,
        wxDataViewEventHandler(ClassEditor::onSRSelectionChange), NULL, this);
	_list->Connect(wxEVT_KEY_DOWN, wxKeyEventHandler(ClassEditor::onTreeViewKeyPress), NULL, this);
	_list->Connect(wxEVT_DATAVIEW_ITEM_CONTEXT_MENU, 
		wxDataViewEventHandler(ClassEditor::onContextMenu), NULL, this);

	// Add the columns to the treeview
	// ID number
	_list->AppendTextColumn("#", SREntity::getColumns().index.getColumnIndex(), 
		wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE, wxALIGN_NOT);
	
	// The S/R icon
	_list->AppendBitmapColumn(_("S/R"), SREntity::getColumns().srClass.getColumnIndex(), 
		wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE, wxALIGN_NOT);

	// The Type
	_list->AppendIconTextColumn(_("Type"), SREntity::getColumns().caption.getColumnIndex(), 
		wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE, wxALIGN_NOT);

	// Buttons below the treeview
	wxBoxSizer* hbox = new wxBoxSizer(wxHORIZONTAL);
	vbox->Add(hbox, 0, wxEXPAND);

	// Create the type selector and pack it
	_addType = createStimTypeSelector(parent);
	hbox->Add(_addType, 1, wxRIGHT, 6);

	_listButtons.add = new wxButton(parent, wxID_ANY, _("Add"));
	_listButtons.remove = new wxButton(parent, wxID_ANY, _("Remove"));

	hbox->Add(_listButtons.add, 0, wxRIGHT, 6);
	hbox->Add(_listButtons.remove, 0);

	_addType->Connect(wxEVT_COMBOBOX, wxCommandEventHandler(ClassEditor::onAddTypeSelect), NULL, this);
	_listButtons.add->Connect(wxEVT_BUTTON, wxCommandEventHandler(ClassEditor::onAddSR), NULL, this);
	_listButtons.remove->Connect(wxEVT_BUTTON, wxCommandEventHandler(ClassEditor::onRemoveSR), NULL, this);
}

void ClassEditor::packEditingPane(wxWindow* pane)
{
	_overallHBox->Add(pane, 1, wxEXPAND);
}

void ClassEditor::setEntity(const SREntityPtr& entity)
{
	_entity = entity;
}

int ClassEditor::getIdFromSelection()
{
	wxDataViewItem item = _list->GetSelection();

	if (item.IsOk() && _entity != NULL)
	{
		wxutil::TreeModel::Row row(item, *_list->GetModel());
		return row[SREntity::getColumns().id].getInteger();
	}
	else
	{
		return -1;
	}
}

void ClassEditor::setProperty(const std::string& key, const std::string& value)
{
	int id = getIdFromSelection();

	if (id > 0)
	{
		// Don't edit inherited stims/responses
		_entity->setProperty(id, key, value);
	}

	// Call the method of the child class to update the widgets
	update();
}

void ClassEditor::entryChanged(wxTextCtrl* entry)
{
	// Try to find the key this entry widget is associated to
	EntryMap::iterator found = _entryWidgets.find(entry);

	if (found != _entryWidgets.end())
	{
		std::string entryText = entry->GetValue().ToStdString();

		if (!entryText.empty())
		{
			setProperty(found->second, entryText);
		}
	}
}

void ClassEditor::spinButtonChanged(wxSpinCtrl* ctrl)
{
	// Try to find the key this spinbutton widget is associated to
	SpinCtrlMap::iterator found = _spinWidgets.find(ctrl);

	if (found != _spinWidgets.end())
	{
		std::string valueText = string::to_string(ctrl->GetValue());

		if (!valueText.empty())
		{
			setProperty(found->second, valueText);
		}
	}
}

void ClassEditor::spinButtonChanged(wxSpinCtrlDouble* ctrl)
{
	// Try to find the key this spinbutton widget is associated to
	SpinCtrlMap::iterator found = _spinWidgets.find(ctrl);

	if (found != _spinWidgets.end())
	{
		std::string valueText = string::to_string(ctrl->GetValue());

		if (!valueText.empty())
		{
			setProperty(found->second, valueText);
		}
	}
}

wxBitmapComboBox* ClassEditor::createStimTypeSelector(wxWindow* parent)
{
	wxBitmapComboBox* combo = new wxBitmapComboBox(parent, 
		wxID_ANY, "", wxDefaultPosition, wxDefaultSize, 0, NULL, wxCB_READONLY);

	_stimTypes.populateBitmapComboBox(combo);

	return combo;
}

void ClassEditor::removeSR()
{
	// Get the selected stim ID
	int id = getIdFromSelection();

	if (id > 0)
	{
		_entity->remove(id);
	}
}

void ClassEditor::selectId(int id)
{
	// Setup the selectionfinder to search for the id
	wxutil::TreeModel* model = dynamic_cast<wxutil::TreeModel*>(_list->GetModel());
	assert(model != NULL);

	wxDataViewItem item = model->FindInteger(id, SREntity::getColumns().id.getColumnIndex());

	if (item.IsOk())
	{
		// Set the active row of the list to the given effect
		_list->Select(item);
		// Manually trigger the selection change signal
		selectionChanged();
	}
}

void ClassEditor::duplicateStimResponse()
{
	int id = getIdFromSelection();

	if (id > 0)
	{
		int newId = _entity->duplicate(id);
		// Select the newly created stim
		selectId(newId);
	}

	// Call the method of the child class to update the widgets
	update();
}

void ClassEditor::onSRSelectionChange(wxDataViewEvent& ev)
{
	selectionChanged();
}

void ClassEditor::onTreeViewKeyPress(wxKeyEvent& ev)
{
	if (ev.GetKeyCode() == WXK_DELETE)
	{
		removeSR();
		return;
	}

	// Propagate further
	ev.Skip();
}

void ClassEditor::onSpinCtrlChanged(wxSpinEvent& ev)
{
	if (_updatesDisabled) return; // Callback loop guard

	spinButtonChanged(dynamic_cast<wxSpinCtrl*>(ev.GetEventObject()));
}

void ClassEditor::onSpinCtrlDoubleChanged(wxSpinDoubleEvent& ev)
{
	if (_updatesDisabled) return; // Callback loop guard

	spinButtonChanged(dynamic_cast<wxSpinCtrlDouble*>(ev.GetEventObject()));
}

void ClassEditor::connectSpinButton(wxSpinCtrl* spinCtrl, const std::string& key)
{
	// Associate the spin button with a specific entity key, if not empty
	if (!key.empty())
	{
		_spinWidgets[spinCtrl] = key;
	}

	// Connect the callback and bind the spinbutton pointer as first argument
	spinCtrl->Connect(wxEVT_SPINCTRL, 
		wxSpinEventHandler(ClassEditor::onSpinCtrlChanged), NULL, this);
}

void ClassEditor::connectSpinButton(wxSpinCtrlDouble* spinCtrl, const std::string& key)
{
	// Associate the spin button with a specific entity key, if not empty
	if (!key.empty())
	{
		_spinWidgets[spinCtrl] = key;
	}

	// Connect the callback and bind the spinbutton pointer as first argument
	spinCtrl->Connect(wxEVT_SPINCTRLDOUBLE, 
		wxSpinDoubleEventHandler(ClassEditor::onSpinCtrlDoubleChanged), NULL, this);
}

void ClassEditor::onEntryChanged(wxCommandEvent& ev)
{
	if (_updatesDisabled) return; // Callback loop guard

	entryChanged(dynamic_cast<wxTextCtrl*>(ev.GetEventObject()));
}

void ClassEditor::connectEntry(wxTextCtrl* entry, const std::string& key)
{
	// Associate the entry with a specific entity key
	_entryWidgets[entry] = key;

	// Connect the callback
	entry->Connect(wxEVT_TEXT,
		wxCommandEventHandler(ClassEditor::onEntryChanged), NULL, this);
}

void ClassEditor::onCheckboxToggle(wxCommandEvent& ev)
{
	if (_updatesDisabled) return; // Callback loop guard

	checkBoxToggled(dynamic_cast<wxCheckBox*>(ev.GetEventObject()));
}

void ClassEditor::connectCheckButton(wxCheckBox* checkButton)
{
	// Bind the checkbutton pointer to the callback, it is needed in onCheckboxToggle
	checkButton->Connect(wxEVT_CHECKBOX,
		wxCommandEventHandler(ClassEditor::onCheckboxToggle), NULL, this);
}

std::string ClassEditor::getStimTypeIdFromSelector(wxBitmapComboBox* comboBox)
{
	if (comboBox->GetSelection() == -1) return "";

	wxStringClientData* stringData = static_cast<wxStringClientData*>(
		comboBox->GetClientData(comboBox->GetSelection()));

	if (stringData == NULL) return "";

	return stringData->GetData().ToStdString();
}

void ClassEditor::onContextMenu(wxDataViewEvent& ev)
{
	wxutil::TreeView* view = dynamic_cast<wxutil::TreeView*>(ev.GetEventObject());

	assert(view != NULL);

	// Call the subclass implementation
	openContextMenu(view);
}

void ClassEditor::onStimTypeSelect(wxCommandEvent& ev)
{
	if (_updatesDisabled || _type == NULL) return; // Callback loop guard

	std::string name = getStimTypeIdFromSelector(_type);

	if (!name.empty())
	{
		// Write it to the entity
		setProperty("type", name);
	}
}

void ClassEditor::onAddTypeSelect(wxCommandEvent& ev)
{
	if (_updatesDisabled || _addType == NULL) return; // Callback loop guard

	wxBitmapComboBox* combo = dynamic_cast<wxBitmapComboBox*>(ev.GetEventObject());
	assert(combo != NULL);

	std::string name = getStimTypeIdFromSelector(combo);

	if (!name.empty())
	{
		addSR();
	}
}

// "Disable" context menu item
void ClassEditor::onContextMenuDisable(wxCommandEvent& ev)
{
	setProperty("state", "0");
}

// "Enable" context menu item
void ClassEditor::onContextMenuEnable(wxCommandEvent& ev)
{
	setProperty("state", "1");
}

void ClassEditor::onContextMenuDuplicate(wxCommandEvent& ev)
{
	duplicateStimResponse();
}

void ClassEditor::onAddSR(wxCommandEvent& ev)
{
	// Add a S/R
	addSR();
}

void ClassEditor::onRemoveSR(wxCommandEvent& ev)
{
	// Delete the selected S/R from the list
	removeSR();
}

} // namespace ui
