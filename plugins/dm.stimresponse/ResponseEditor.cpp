#include "ResponseEditor.h"

#include "wxutil/TreeModel.h"
#include "wxutil/menu/IconTextMenuItem.h"
#include "string/convert.h"

#include "i18n.h"
#include "EffectEditor.h"

#include <wx/combobox.h>
#include <wx/bmpcbox.h>
#include <wx/button.h>
#include <wx/menu.h>
#include <wx/spinctrl.h>
#include <wx/textctrl.h>
#include <wx/checkbox.h>

#include "wxutil/ChoiceHelper.h"

namespace ui
{

ResponseEditor::ResponseEditor(wxWindow* mainPanel, StimTypes& stimTypes) :
	ClassEditor(mainPanel, stimTypes),
	_mainPanel(mainPanel)
{
	setupPage();

	createContextMenu();
	update();
}

void ResponseEditor::setEntity(const SREntityPtr& entity)
{
	// Pass the call to the base class
	ClassEditor::setEntity(entity);

	if (entity)
	{
		wxutil::TreeModel::Ptr responseStore = _entity->getResponseStore();
		_list->AssociateModel(responseStore.get());

		// Trigger column width reevaluation
		responseStore->ItemChanged(responseStore->GetRoot());

		// Clear the treeview
		wxutil::TreeModel* effectsModel = 
			static_cast<wxutil::TreeModel*>(_effectWidgets.view->GetModel());
		effectsModel->Clear();
	}
	else
	{
		// wxWidgets 3.0.0 crashes when associating a NULL model, so use a dummy model
		// to release the old one
		wxutil::TreeModel* dummyStore = new wxutil::TreeModel(SREntity::getColumns(), true);
		_list->AssociateModel(dummyStore);
	}
}

void ResponseEditor::update()
{
	_updatesDisabled = true;

	wxPanel* mainPanel = findNamedObject<wxPanel>(_mainPanel, "SREditorResponsePanel");
	auto removeButton = findNamedObject<wxButton>(_mainPanel, "RemoveResponseButton");

	int id = getIdFromSelection();

	if (id > 0 && _entity != NULL)
	{
		mainPanel->Enable(true);

		StimResponse& sr = _entity->get(id);

		// Get the iter into the liststore pointing at the correct STIM_YYYY type
		std::string typeToFind = sr.get("type");
		wxutil::ChoiceHelper:: SelectComboItemByStoredString (_type, typeToFind);
		
		// Active
		_propertyWidgets.active->SetValue(sr.get("state") == "1");

		// Use Radius
		bool useRandomEffects = (sr.get("random_effects") != "");
		_propertyWidgets.randomEffectsToggle->SetValue(useRandomEffects);
		_propertyWidgets.randomEffectsEntry->SetValue(sr.get("random_effects"));
		_propertyWidgets.randomEffectsEntry->Enable(useRandomEffects);

		// Use Chance
		bool useChance = (sr.get("chance") != "");
		_propertyWidgets.chanceToggle->SetValue(useChance);
		_propertyWidgets.chanceEntry->SetValue(string::convert<double>(sr.get("chance")));
		_propertyWidgets.chanceEntry->Enable(useChance);

		wxutil::TreeModel::Ptr effectsModel = sr.createEffectsStore();

		// It's important to unselect everything before swapping the model
		// otherwise wxDataViewCtrl will keep invalid items in its internal selection list
		_effectWidgets.view->UnselectAll();

		_effectWidgets.view->AssociateModel(effectsModel.get());
		effectsModel->ItemChanged(effectsModel->GetRoot()); // trigger column width re-evaluation

		// Disable the editing of inherited properties completely
		if (sr.inherited())
		{
			mainPanel->Enable(false);
		}

		// Update the delete context menu item
		_contextMenu.remove->Enable(!sr.inherited());
		removeButton->Enable(!sr.inherited());

		// If there is anything selected, the duplicate item is always active
		_contextMenu.duplicate->Enable(true);

		// Update the "enable/disable" menu items
		bool state = sr.get("state") == "1";
		_contextMenu.enable->Enable(!state);
		_contextMenu.disable->Enable(state);

		// The response effect list may be empty, so force an update of the
		// context menu sensitivity, in the case the "selection changed"
		// signal doesn't get called
		updateEffectContextMenu();
	}
	else
	{
		// Nothing selected
		mainPanel->Enable(false);

		// Clear the effect tree view
		wxutil::TreeModel* effectsModel = 
			static_cast<wxutil::TreeModel*>(_effectWidgets.view->GetModel());
		effectsModel->Clear();

		_contextMenu.enable->Enable(false);
		_contextMenu.disable->Enable(false);
		_contextMenu.remove->Enable(false);
		_contextMenu.duplicate->Enable(false);

		removeButton->Enable(false);
	}

	_updatesDisabled = false;
}

void ResponseEditor::setupPage()
{
	wxPanel* listPanel = findNamedObject<wxPanel>(_mainPanel, "SREditorResponseList");
	createListView(listPanel);

#ifdef USE_BMP_COMBO_BOX
	// Response property section
	_type = findNamedObject<wxBitmapComboBox>(_mainPanel, "ResponseEditorTypeCombo");
#else
	{
		// Response property section
		wxControl* typeBox = findNamedObject<wxControl>(_mainPanel, "ResponseEditorTypeCombo");
	
		// Replace the bitmap combo with an ordinary one
		wxComboBox* combo = new wxComboBox(typeBox->GetParent(), wxID_ANY);
		typeBox->GetContainingSizer()->Add(combo, 1, wxEXPAND);
		typeBox->Destroy();

		_type = combo;
		_type->SetName("ResponseEditorTypeCombo");
	}
#endif

	_type->Connect(wxEVT_COMBOBOX, wxCommandEventHandler(ResponseEditor::onStimTypeSelect), NULL, this); 

	// Active
	_propertyWidgets.active = findNamedObject<wxCheckBox>(_mainPanel, "ResponseEditorActive");

	// Random Effects Toggle
	_propertyWidgets.randomEffectsToggle = findNamedObject<wxCheckBox>(_mainPanel, "ResponseEditorRandomFX");
	_propertyWidgets.randomEffectsEntry = findNamedObject<wxTextCtrl>(_mainPanel, "ResponseEditorRandomFXValue");

	// Chance variable
	_propertyWidgets.chanceToggle = findNamedObject<wxCheckBox>(_mainPanel, "ResponseEditorChance");

	wxPanel* chancePanel = findNamedObject<wxPanel>(_mainPanel, "ResponseEditorChanceValuePanel");

	_propertyWidgets.chanceEntry = new wxSpinCtrlDouble(chancePanel, wxID_ANY);
	_propertyWidgets.chanceEntry->SetRange(0.0, 1.0);
	_propertyWidgets.chanceEntry->SetIncrement(0.01);
	_propertyWidgets.chanceEntry->SetValue(0);

	chancePanel->GetSizer()->Add(_propertyWidgets.chanceEntry, 1);

	// Connect the signals
	connectCheckButton(_propertyWidgets.active);
	connectCheckButton(_propertyWidgets.randomEffectsToggle);
	connectCheckButton(_propertyWidgets.chanceToggle);

	connectEntry(_propertyWidgets.randomEffectsEntry, "random_effects");

	connectSpinButton(_propertyWidgets.chanceEntry, "chance");

	makeLabelBold(_mainPanel, "ResponseEditorFXLabel");

	createEffectWidgets();

#ifdef USE_BMP_COMBO_BOX
	_addType = findNamedObject<wxBitmapComboBox>(_mainPanel, "ResponseTypeComboBox");
#else
	{
		// Type selector box
		wxControl* addTypeBox = findNamedObject<wxControl>(_mainPanel, "ResponseTypeComboBox");

		// Replace the bitmap combo with an ordinary one
		wxComboBox* newTypeCombo = new wxComboBox(addTypeBox->GetParent(), wxID_ANY);
		addTypeBox->GetContainingSizer()->Prepend(newTypeCombo, 1, wxEXPAND | wxRIGHT, 6);
		addTypeBox->Destroy();

		_addType = newTypeCombo;
		_addType->SetName("ResponseTypeComboBox");
	}
#endif
	_addType->Bind(wxEVT_COMBOBOX, std::bind(&ResponseEditor::onAddTypeSelect, this, std::placeholders::_1));

	auto addButton = findNamedObject<wxButton>(_mainPanel, "AddResponseButton");
	auto removeButton = findNamedObject<wxButton>(_mainPanel, "RemoveResponseButton");

	addButton->Bind(wxEVT_BUTTON, std::bind(&ResponseEditor::onAddSR, this, std::placeholders::_1));
	removeButton->Bind(wxEVT_BUTTON, std::bind(&ResponseEditor::onRemoveSR, this, std::placeholders::_1));
	
	reloadStimTypes();
}

void ResponseEditor::reloadStimTypes()
{
	if (_stimTypes.getStimMap().empty())
	{
		_stimTypes.reload();
	}

	_stimTypes.populateComboBox(_addType);
	_stimTypes.populateComboBox(_type);
}

void ResponseEditor::createEffectWidgets()
{
	wxPanel* effectsPanel = findNamedObject<wxPanel>(_mainPanel, "ResponseEditorFXPanel");

	wxutil::TreeModel::Ptr dummyModel(
        new wxutil::TreeModel(StimResponse::getColumns(), true)
    );
	_effectWidgets.view = wxutil::TreeView::CreateWithModel(effectsPanel, dummyModel);

	_effectWidgets.view->SetMinClientSize(wxSize(-1, 150));
	effectsPanel->GetSizer()->Add(_effectWidgets.view, 1, wxEXPAND);

	// Connect the signals
	_effectWidgets.view->Connect(wxEVT_DATAVIEW_SELECTION_CHANGED,
        wxDataViewEventHandler(ResponseEditor::onEffectSelectionChange), nullptr, this);

	_effectWidgets.view->Connect(wxEVT_DATAVIEW_ITEM_ACTIVATED, 
		wxDataViewEventHandler(ResponseEditor::onEffectItemActivated), nullptr, this);
	
	_effectWidgets.view->Connect(wxEVT_DATAVIEW_ITEM_CONTEXT_MENU, 
		wxDataViewEventHandler(ResponseEditor::onEffectItemContextMenu), nullptr, this);

	// View Columns
	_effectWidgets.view->AppendTextColumn("#", StimResponse::getColumns().index.getColumnIndex(), 
		wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE, wxALIGN_NOT);
	
	_effectWidgets.view->AppendTextColumn(_("Effect"), StimResponse::getColumns().caption.getColumnIndex(), 
		wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE, wxALIGN_NOT);

	_effectWidgets.view->AppendTextColumn(_("Details (double-click to edit)"), StimResponse::getColumns().arguments.getColumnIndex(), 
		wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE, wxALIGN_NOT);
}

void ResponseEditor::checkBoxToggled(wxCheckBox* toggleButton)
{
	bool active = toggleButton->GetValue();

	if (toggleButton == _propertyWidgets.active)
	{
		setProperty("state", active ? "1" : "0");
	}
	else if (toggleButton == _propertyWidgets.randomEffectsToggle)
	{
		std::string entryText = _propertyWidgets.randomEffectsEntry->GetValue().ToStdString();

		// Enter a default value for the entry text, if it's empty up till now.
		if (active)
		{
			entryText += (entryText.empty()) ? "1" : "";
		}
		else {
			entryText = "";
		}

		setProperty("random_effects", entryText);
	}
	else if (toggleButton == _propertyWidgets.chanceToggle)
	{
		std::string entryText = string::to_string(_propertyWidgets.chanceEntry->GetValue());

		setProperty("chance", active ? entryText : "");
	}
}

void ResponseEditor::addEffect()
{
	if (_entity == NULL) return;

	int id = getIdFromSelection();

	if (id > 0)
	{
		StimResponse& sr = _entity->get(id);
		int effectIndex = getEffectIdFromSelection();

		// Make sure we have a response
		if (sr.get("class") == "R") {
			// Add a new effect and update all the widgets
			sr.addEffect(effectIndex);
			update();
		}
	}
}

void ResponseEditor::removeEffect()
{
	if (_entity == NULL) return;

	int id = getIdFromSelection();

	if (id > 0)
	{
		StimResponse& sr = _entity->get(id);
		int effectIndex = getEffectIdFromSelection();

		// Make sure we have a response and anything selected
		if (sr.get("class") == "R" && effectIndex > 0)
		{
			// Remove the effect and update all the widgets
			sr.deleteEffect(effectIndex);
			update();
		}
	}
}

void ResponseEditor::editEffect()
{
	if (_entity == NULL) return;

	int id = getIdFromSelection();

	if (id > 0)
	{
		StimResponse& sr = _entity->get(id);
		int effectIndex = getEffectIdFromSelection();

		// Make sure we have a response and anything selected
		if (sr.get("class") == "R" && effectIndex > 0)
		{
			// Create a new effect editor (self-destructs)
			EffectEditor* editor = new EffectEditor(_mainPanel, sr, effectIndex, _stimTypes, *this);

			editor->ShowModal();
			editor->Destroy();
		}
	}
}

void ResponseEditor::moveEffect(int direction)
{
	if (_entity == NULL) return;

	int id = getIdFromSelection();

	if (id > 0)
	{
		StimResponse& sr = _entity->get(id);
		int effectIndex = getEffectIdFromSelection();

		if (sr.get("class") == "R" && effectIndex > 0)
		{
			// Move the index (swap the specified indices)
			sr.moveEffect(effectIndex, effectIndex + direction);
			update();
			// Select the moved effect after the update
			selectEffectIndex(effectIndex + direction);
		}
	}
}

void ResponseEditor::updateEffectContextMenu()
{
	// Check if we have anything selected at all
	int curEffectIndex = getEffectIdFromSelection();
	int highestEffectIndex = 0;

	bool anythingSelected = curEffectIndex >= 0;

	int srId = getIdFromSelection();

	if (srId > 0)
	{
		StimResponse& sr = _entity->get(srId);
		highestEffectIndex = sr.highestEffectIndex();
	}

	bool upActive = anythingSelected && curEffectIndex > 1;
	bool downActive = anythingSelected && curEffectIndex < highestEffectIndex;

	// Enable or disable the "Delete" context menu items based on the presence
	// of a selection.
	_effectWidgets.contextMenu->Enable(_effectWidgets.deleteMenuItem->GetId(), anythingSelected);
	_effectWidgets.contextMenu->Enable(_effectWidgets.editMenuItem->GetId(), anythingSelected);

	_effectWidgets.contextMenu->Enable(_effectWidgets.upMenuItem->GetId(), upActive);
	_effectWidgets.contextMenu->Enable(_effectWidgets.downMenuItem->GetId(), downActive);
}

// Create the context menus
void ResponseEditor::createContextMenu()
{
	// Menu widgets
	_contextMenu.menu.reset(new wxMenu);

	// Each menu gets a delete item
	_contextMenu.enable = _contextMenu.menu->Append(
		new wxutil::IconTextMenuItem(_("Activate"), "sr_response.png"));
	_contextMenu.disable = _contextMenu.menu->Append(
		new wxutil::IconTextMenuItem(_("Deactivate"), "sr_response_inactive.png"));
	_contextMenu.duplicate = _contextMenu.menu->Append(
		new wxutil::StockIconTextMenuItem(_("Duplicate"), wxART_COPY));
	_contextMenu.remove = _contextMenu.menu->Append(
		new wxutil::StockIconTextMenuItem(_("Delete"), wxART_DELETE));

	_effectWidgets.contextMenu.reset(new wxMenu);

	_effectWidgets.addMenuItem = _effectWidgets.contextMenu->Append(
		new wxutil::StockIconTextMenuItem(_("Add new Effect"), wxART_PLUS));
	_effectWidgets.editMenuItem = _effectWidgets.contextMenu->Append(
		new wxutil::IconTextMenuItem(_("Edit"), "edit.png"));
	
	_effectWidgets.upMenuItem = _effectWidgets.contextMenu->Append(
		new wxutil::StockIconTextMenuItem(_("Move Up"), wxART_GO_UP));
	_effectWidgets.downMenuItem = _effectWidgets.contextMenu->Append(
		new wxutil::StockIconTextMenuItem(_("Move Down"), wxART_GO_DOWN));

	_effectWidgets.deleteMenuItem = _effectWidgets.contextMenu->Append(
		new wxutil::StockIconTextMenuItem(_("Delete"), wxART_DELETE));

	// Connect up the signals
	_contextMenu.menu->Connect(_contextMenu.remove->GetId(), wxEVT_MENU, 
		wxCommandEventHandler(ResponseEditor::onContextMenuDelete), NULL, this);
	_contextMenu.menu->Connect(_contextMenu.enable->GetId(), wxEVT_MENU, 
		wxCommandEventHandler(ResponseEditor::onContextMenuEnable), NULL, this);
	_contextMenu.menu->Connect(_contextMenu.disable->GetId(), wxEVT_MENU, 
		wxCommandEventHandler(ResponseEditor::onContextMenuDisable), NULL, this);
	_contextMenu.menu->Connect(_contextMenu.duplicate->GetId(), wxEVT_MENU, 
		wxCommandEventHandler(ResponseEditor::onContextMenuDuplicate), NULL, this);

	_effectWidgets.contextMenu->Connect(_effectWidgets.deleteMenuItem->GetId(), wxEVT_MENU, 
		wxCommandEventHandler(ResponseEditor::onEffectMenuDelete), NULL, this);
	_effectWidgets.contextMenu->Connect(_effectWidgets.editMenuItem->GetId(), wxEVT_MENU, 
		wxCommandEventHandler(ResponseEditor::onEffectMenuEdit), NULL, this);
	_effectWidgets.contextMenu->Connect(_effectWidgets.addMenuItem->GetId(), wxEVT_MENU, 
		wxCommandEventHandler(ResponseEditor::onEffectMenuAdd), NULL, this);
	_effectWidgets.contextMenu->Connect(_effectWidgets.upMenuItem->GetId(), wxEVT_MENU, 
		wxCommandEventHandler(ResponseEditor::onEffectMenuEffectUp), NULL, this);
	_effectWidgets.contextMenu->Connect(_effectWidgets.downMenuItem->GetId(), wxEVT_MENU, 
		wxCommandEventHandler(ResponseEditor::onEffectMenuEffectDown), NULL, this);
}

void ResponseEditor::selectEffectIndex(const unsigned int index)
{
	wxutil::TreeModel* model = static_cast<wxutil::TreeModel*>(_effectWidgets.view->GetModel());

	wxDataViewItem item = model->FindInteger(index, StimResponse::getColumns().index);

	if (item.IsOk())
	{
		// Set the active row of the list to the given effect
		_effectWidgets.view->Select(item);
	}
}

int ResponseEditor::getEffectIdFromSelection()
{
	wxDataViewItem item = _effectWidgets.view->GetSelection();

	if (item.IsOk() && _entity != NULL)
	{
		wxutil::TreeModel::Row row(item, *_effectWidgets.view->GetModel());
		return row[StimResponse::getColumns().index].getInteger();
	}
	else
	{
		return -1;
	}
}

void ResponseEditor::openSRListContextMenu()
{
	_list->PopupMenu(_contextMenu.menu.get());
}

void ResponseEditor::selectionChanged()
{
	update();
}

void ResponseEditor::addSR()
{
	if (_entity == NULL) return;

	// Create a new StimResponse object
	int id = _entity->add();

	// Get a reference to the newly allocated object
	StimResponse& sr = _entity->get(id);
	sr.set("class", "R");

	// Get the selected stim type name from the combo box
	std::string name = getStimTypeIdFromSelector(_addType);
	sr.set("type", (!name.empty()) ? name : _stimTypes.getFirstName());

	sr.set("state", "1");

	// Update the list stores AFTER the type has been set
	_entity->updateListStores();

	// Select the newly created response
	selectId(id);
}

void ResponseEditor::onEffectItemContextMenu(wxDataViewEvent& ev)
{
	updateEffectContextMenu();

	_effectWidgets.view->PopupMenu(_effectWidgets.contextMenu.get());
}

// Button click events on TreeViews
void ResponseEditor::onEffectItemActivated(wxDataViewEvent& ev)
{
	// Call the effect editor upon double click
	editEffect();
}

void ResponseEditor::onEffectMenuDelete(wxCommandEvent& ev)
{
	removeEffect();
}

void ResponseEditor::onEffectMenuEdit(wxCommandEvent& ev)
{
	editEffect();
}

void ResponseEditor::onEffectMenuAdd(wxCommandEvent& ev)
{
	addEffect();
}

void ResponseEditor::onEffectMenuEffectUp(wxCommandEvent& ev)
{
	moveEffect(-1);
}

void ResponseEditor::onEffectMenuEffectDown(wxCommandEvent& ev)
{
	moveEffect(+1);
}

// Delete context menu items activated
void ResponseEditor::onContextMenuDelete(wxCommandEvent& ev)
{
	// Delete the selected stim from the list
	removeSR();
}

// Delete context menu items activated
void ResponseEditor::onContextMenuAdd(wxCommandEvent& ev)
{
	addSR();
}

// Callback for effects treeview selection changes
void ResponseEditor::onEffectSelectionChange(wxDataViewEvent& ev)
{
	if (_updatesDisabled)	return; // Callback loop guard

	// Update the sensitivity
	updateEffectContextMenu();
}

} // namespace ui
