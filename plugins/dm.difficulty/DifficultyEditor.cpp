#include "DifficultyEditor.h"

#include "i18n.h"

#include "wxutil/dataview/TreeView.h"
#include "wxutil/ChoiceHelper.h"
#include "wxutil/dialog/MessageBox.h"
#include "wxutil/EntityClassChooser.h"

#include <wx/panel.h>
#include <wx/choice.h>
#include <wx/textctrl.h>
#include <wx/stattext.h>
#include <wx/button.h>
#include <wx/combobox.h>
#include <wx/sizer.h>

#include "ClassNameStore.h"

#include "debugging/ScopedDebugTimer.h"

namespace ui
{

DifficultyEditor::DifficultyEditor(wxWindow* parent,
                                   const difficulty::DifficultySettingsPtr& settings)
: _settings(settings)
{
	// The actual editor pane
	_editor = loadNamedPanel(parent, "DifficultyEditorMainPanel");

	_settings->updateTreeModel();

	populateWindow();
	updateEditorWidgets();
}

wxWindow* DifficultyEditor::getWidget()
{
	return _editor;
}

void DifficultyEditor::populateWindow()
{
    ScopedDebugTimer timer("DifficultyEditor::populateWindow()");

	wxPanel* viewPanel = findNamedObject<wxPanel>(_editor, "DifficultyEditorTreeViewPanel");

	_settingsView = wxutil::TreeView::CreateWithModel(viewPanel, _settings->getTreeStore().get());
	_settingsView->Connect(wxEVT_DATAVIEW_SELECTION_CHANGED, 
		wxDataViewEventHandler(DifficultyEditor::onSettingSelectionChange), NULL, this);
	viewPanel->GetSizer()->Add(_settingsView, 1, wxEXPAND);

	_settingsView->AppendTextColumn(_("Setting"), 
		_settings->getColumns().description.getColumnIndex(), wxDATAVIEW_CELL_INERT, 
		wxCOL_WIDTH_AUTOSIZE, wxALIGN_NOT);

	// Save a few shortcuts
	_spawnArgEntry = findNamedObject<wxTextCtrl>(_editor, "DifficultyEditorSpawnarg");
	_argumentEntry = findNamedObject<wxTextCtrl>(_editor, "DifficultyEditorArgument");
	
    // Set up the class name entry box and button
	_classEntry = findNamedObject<wxTextCtrl>(_editor, "ClassNameTextBox");
	_classEntry->AutoComplete(ClassNameStore::Instance().getStringList());
    wxButton* chooseClassBtn = findNamedObject<wxButton>(_editor, "ChooseClassButton");
    chooseClassBtn->Bind(
        wxEVT_BUTTON, [&] (wxCommandEvent&) { chooseEntityClass(); }
    );

	// AppTypes
	_appTypeCombo = findNamedObject<wxChoice>(_editor, "DifficultyEditorApplicationType");

	_appTypeCombo->Append(_("Assign"), new wxStringClientData(string::to_string(difficulty::Setting::EAssign)));
	_appTypeCombo->Append(_("Add"), new wxStringClientData(string::to_string(difficulty::Setting::EAdd)));
	_appTypeCombo->Append(_("Multiply"), new wxStringClientData(string::to_string(difficulty::Setting::EMultiply)));
	_appTypeCombo->Append(_("Ignore"), new wxStringClientData(string::to_string(difficulty::Setting::EIgnore)));

	_appTypeCombo->Connect(wxEVT_CHOICE, wxCommandEventHandler(DifficultyEditor::onAppTypeChange), NULL, this);

	// Buttons
	_saveSettingButton = findNamedObject<wxButton>(_editor, "DifficultyEditorSaveSettingButton");
	_saveSettingButton->Connect(wxEVT_BUTTON, wxCommandEventHandler(DifficultyEditor::onSettingSave), NULL, this);

	_deleteSettingButton = findNamedObject<wxButton>(_editor, "DifficultyEditorDeleteSettingButton");
	_deleteSettingButton->Connect(wxEVT_BUTTON, wxCommandEventHandler(DifficultyEditor::onSettingDelete), NULL, this);

	_createSettingButton = findNamedObject<wxButton>(_editor, "DifficultyEditorAddSettingButton");
	_createSettingButton->Connect(wxEVT_BUTTON, wxCommandEventHandler(DifficultyEditor::onSettingCreate), NULL, this);

	_refreshButton = findNamedObject<wxButton>(_editor, "DifficultyEditorRefreshButton");
	_refreshButton->Connect(wxEVT_BUTTON, wxCommandEventHandler(DifficultyEditor::onRefresh), NULL, this);

	_noteText = findNamedObject<wxStaticText>(_editor, "DifficultyEditorNoteText");

	makeLabelBold(_editor, "DifficultyEditorSettingLabel");
}

void DifficultyEditor::chooseEntityClass()
{
    // Show dialog and set the entry box text with the chosen entity class
    auto chosenEntity = wxutil::EntityClassChooser::ChooseEntityClass(
        wxutil::EntityClassChooser::Purpose::SelectClassname, _classEntry->GetValue().ToStdString()
    );

    if (!chosenEntity.empty())
    {
        _classEntry->SetValue(chosenEntity);
    }
}

int DifficultyEditor::getSelectedSettingId()
{
	wxDataViewItem item = _settingsView->GetSelection();

	if (item.IsOk())
	{
		wxutil::TreeModel::Row row(item, *_settingsView->GetModel());
		return row[_settings->getColumns().settingId].getInteger(); 
	}
	
	return -1;
}

void DifficultyEditor::updateEditorWidgets()
{
	_updateActive = true;

	int id = getSelectedSettingId();

	bool editWidgetsSensitive = false;

	std::string noteText;

	if (id != -1)
	{
		// Lookup the setting using className/id combo
		difficulty::SettingPtr setting = _settings->getSettingById(id);

		if (setting != NULL)
		{
			// Activate editing pane
			editWidgetsSensitive = true;

			if (_settings->isOverridden(setting))
			{
				editWidgetsSensitive = false;
				noteText += _("This default setting is overridden, cannot edit.");
			}

			_spawnArgEntry->SetValue(setting->spawnArg);
			_argumentEntry->SetValue(setting->argument);

			// Now select the eclass passed in the argument
			// Find the entity using a TreeModel traversor
			_classEntry->SetValue(setting->className);

			wxutil::ChoiceHelper::SelectItemByStoredId(_appTypeCombo, static_cast<int>(setting->appType));

			// Set the sensitivity of the argument entry box
			_argumentEntry->Enable(
				(setting->appType == difficulty::Setting::EIgnore) ? false : true
			);

			// We have a treeview selection, lock the classname
			_classEntry->Enable(false);

			// Disable the deletion of default settings
			_deleteSettingButton->Enable((setting->isDefault) ? false : true);
			_saveSettingButton->Enable(true);
		}
	}
	else
	{
		// Nothing selected, disable deletion
		_deleteSettingButton->Enable(false);
		_saveSettingButton->Enable(false);
	}

	// Set editing pane sensitivity
	findNamedObject<wxPanel>(_editor, "DifficultyEditorSettingsPanel")->Enable(editWidgetsSensitive);

	// Set the note text in any case
	_noteText->SetLabelMarkup(noteText);
	_noteText->Wrap(_noteText->GetSize().GetWidth());

	_updateActive = false;
}

void DifficultyEditor::createSetting()
{
	// Unselect everything
	_settingsView->UnselectAll();

	// Unlock editing widgets
	findNamedObject<wxPanel>(_editor, "DifficultyEditorSettingsPanel")->Enable(true);

	// Unlock class combo
	_classEntry->Enable(true);
	_saveSettingButton->Enable(true);

	_spawnArgEntry->SetValue("");
	_argumentEntry->SetValue("");
}

void DifficultyEditor::saveSetting()
{
	// Get the ID of the currently selected item (might be -1 if no selection)
	int id = getSelectedSettingId();

	// Instantiate a new setting and fill the data in
	difficulty::SettingPtr setting(new difficulty::Setting);

	// Load the widget contents
	setting->className = _classEntry->GetValue();

	if (setting->className.empty())
	{
		wxutil::Messagebox::ShowError(_("Classname cannot be left empty"), wxGetTopLevelParent(_classEntry));
		return;
	}

	setting->spawnArg = _spawnArgEntry->GetValue();
	setting->argument = _argumentEntry->GetValue();

	if (setting->spawnArg.empty() || setting->argument.empty())
	{
		wxutil::Messagebox::ShowError(_("Spawnarg name and value cannot be left empty"), 
			wxGetTopLevelParent(_spawnArgEntry));
		return;
	}

	// Get the apptype from the dropdown list
	setting->appType = difficulty::Setting::EAssign;

	if (_appTypeCombo->GetSelection() != wxNOT_FOUND)
	{
		int selected = wxutil::ChoiceHelper::GetSelectionId(_appTypeCombo);
		setting->appType = static_cast<difficulty::Setting::EApplicationType>(selected);
	}

	// Pass the data to the DifficultySettings class to handle it
	id = _settings->save(id, setting);

	// Update the treemodel
	_settings->updateTreeModel();

	// Highlight the setting, it might have been newly created
	selectSettingById(id);
}

void DifficultyEditor::deleteSetting()
{
	// Get the ID of the currently selected item (might be -1 if no selection)
	int id = getSelectedSettingId();

	// Instantiate a new setting and fill the data in
	difficulty::SettingPtr setting = _settings->getSettingById(id);

	if (setting == NULL || setting->isDefault) {
		// Don't delete NULL or default settings
		return;
	}

	// Remove the setting
	_settings->deleteSetting(id);
}

void DifficultyEditor::selectSettingById(int id)
{
	wxDataViewItem found = _settings->getTreeStore()->FindInteger(id, 
		_settings->getColumns().settingId);

	_settingsView->Select(found);
	_settingsView->EnsureVisible(found);
}

void DifficultyEditor::onSettingSelectionChange(wxDataViewEvent& ev)
{
	// Update editor widgets
	updateEditorWidgets();
}

void DifficultyEditor::onSettingSave(wxCommandEvent& ev)
{
	saveSetting();
}

void DifficultyEditor::onSettingDelete(wxCommandEvent& ev)
{
	deleteSetting();
}

void DifficultyEditor::onSettingCreate(wxCommandEvent& ev)
{
	createSetting();
}

void DifficultyEditor::onRefresh(wxCommandEvent& ev)
{
	_settings->refreshTreeModel();
}

void DifficultyEditor::onAppTypeChange(wxCommandEvent& ev)
{
	if (_updateActive) return;

	// Update the sensitivity of the argument entry widget
	int selected = wxutil::ChoiceHelper::GetSelectionId(_appTypeCombo);

	difficulty::Setting::EApplicationType appType = 
		static_cast<difficulty::Setting::EApplicationType>(selected);

	_argumentEntry->Enable(
		(appType == difficulty::Setting::EIgnore) ? false : true
	);
}

} // namespace ui
