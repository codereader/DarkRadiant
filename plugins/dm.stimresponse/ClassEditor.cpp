#include "ClassEditor.h"

#include "string/convert.h"
#include <iostream>
#include "i18n.h"

#include <wx/bmpcbox.h>
#include <wx/combobox.h>
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

ClassEditor::ClassEditor(wxWindow* mainPanel, StimTypes& stimTypes) :
	_list(nullptr),
	_stimTypes(stimTypes),
	_updatesDisabled(false),
	_type(nullptr),
	_addType(nullptr)
{}

void ClassEditor::createListView(wxWindow* parent)
{
	wxutil::TreeModel::Ptr dummyModel(
		new wxutil::TreeModel(SREntity::getColumns(), true)
	);

	_list = wxutil::TreeView::CreateWithModel(parent, dummyModel);

	_list->SetMinClientSize(wxSize(TREE_VIEW_WIDTH, TREE_VIEW_HEIGHT));
	parent->GetSizer()->Add(_list, 1, wxEXPAND);

	// Connect the signals to the callbacks
	_list->Connect(wxEVT_DATAVIEW_SELECTION_CHANGED, wxDataViewEventHandler(ClassEditor::onSRSelectionChange), NULL, this);
	_list->Connect(wxEVT_KEY_DOWN, wxKeyEventHandler(ClassEditor::onTreeViewKeyPress), NULL, this);
	_list->Connect(wxEVT_DATAVIEW_ITEM_CONTEXT_MENU, wxDataViewEventHandler(ClassEditor::onContextMenu), NULL, this);

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
}

void ClassEditor::setEntity(const SREntityPtr& entity)
{
	_entity = entity;
}

int ClassEditor::getIndexFromSelection()
{
	wxDataViewItem item = _list->GetSelection();

	if (item.IsOk() && _entity != nullptr)
	{
		wxutil::TreeModel::Row row(item, *_list->GetModel());
		return row[SREntity::getColumns().index].getInteger();
	}
	
	return -1;
}

void ClassEditor::setProperty(const std::string& key, const std::string& value)
{
	int index = getIndexFromSelection();

	if (index > 0)
	{
		// Don't edit inherited stims/responses
		_entity->setProperty(index, key, value);
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

void ClassEditor::reloadStimTypes()
{
	if (_type != NULL)
	{
		_stimTypes.populateComboBox(_type);
	}

	if (_addType != NULL)
	{
		_stimTypes.populateComboBox(_addType);
	}
}

void ClassEditor::removeSR()
{
	// Get the selected stim index
	int index = getIndexFromSelection();

	if (index > 0)
	{
		_entity->remove(index);
	}
}

void ClassEditor::selectIndex(int index)
{
	// Setup the selectionfinder to search for the id
	wxutil::TreeModel* model = dynamic_cast<wxutil::TreeModel*>(_list->GetModel());
	assert(model != NULL);

	wxDataViewItem item = model->FindInteger(index, SREntity::getColumns().index);

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
	int index = getIndexFromSelection();

	if (index > 0)
	{
		int newIndex = _entity->duplicate(index);
		// Select the newly created stim
		selectIndex(newIndex);
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

std::string ClassEditor::getStimTypeIdFromSelector(wxComboBox* comboBox)
{
	if (comboBox->GetSelection() == -1) return "";

	wxStringClientData* stringData = static_cast<wxStringClientData*>(
		comboBox->GetClientObject(comboBox->GetSelection()));

	if (stringData == NULL) return "";

	return stringData->GetData().ToStdString();
}

void ClassEditor::onContextMenu(wxDataViewEvent& ev)
{
	// Call the subclass implementation
	openSRListContextMenu();
}

void ClassEditor::onStimTypeSelect(wxCommandEvent& ev)
{
	if (_updatesDisabled || _type == nullptr) return; // Callback loop guard

	std::string name = getStimTypeIdFromSelector(_type);

	if (!name.empty())
	{
		// Write it to the entity
		setProperty("type", name);
	}
}

void ClassEditor::onAddTypeSelect(wxCommandEvent& ev)
{
	if (_updatesDisabled || _addType == nullptr) return; // Callback loop guard

	wxComboBox* combo = dynamic_cast<wxComboBox*>(ev.GetEventObject());
	assert(combo != nullptr);

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
