#pragma once

#include <string>
#include <memory>

#include "DifficultySettings.h"
#include "wxutil/XmlResourceBasedWidget.h"

class wxPanel;
class wxTextCtrl;
class wxChoice;
class wxButton;
class wxStaticText;
class wxComboBox;

namespace wxutil { class TreeView; }

namespace ui
{

/**
 * greebo: A Difficulty Editor provides the front-end for editing a
 * set of Difficulty Settings (e.g. "Easy" setting). The actual
 * data is stored in a DifficultySettings instance.
 */
class DifficultyEditor :
	public wxEvtHandler,
	private wxutil::XmlResourceBasedWidget
{
private:
	// The actual settings we're working with
	difficulty::DifficultySettingsPtr _settings;

	// GtkNotebook-related widgets
	wxPanel* _editor;
	std::string _label; // the actual label

	wxutil::TreeView* _settingsView;

	// The classname dropdown entry field
	wxComboBox* _classCombo;
	wxTextCtrl* _spawnArgEntry;
	wxTextCtrl* _argumentEntry;
	wxChoice* _appTypeCombo;

	wxButton* _saveSettingButton;
	wxButton* _deleteSettingButton;
	wxButton* _createSettingButton;
	wxButton* _refreshButton;

	// A label containing notes to the user
	wxStaticText* _noteText;

	// Mutex for avoiding loopbacks
	bool _updateActive;

public:
	/**
	 * greebo: Pass the label string and the difficulty settings object to the
	 *         constructor. The DifficultySettings should be populated first.
	 */
	DifficultyEditor(wxWindow* parent, const std::string& label, const difficulty::DifficultySettingsPtr& settings);

	// Returns the actual editor widget (contains all controls and views)
	wxWindow* getEditor();

	// Returns the label for packing into a GtkNotebook tab.
	std::string getNotebookLabel();

private:
	// Creates the widgets
	void populateWindow();

	// Returns the ID of the selected setting (or -1) if no valid setting is selected
	int getSelectedSettingId();

	// Loads the data from the treeview selection into the editor widgets
	void updateEditorWidgets();

	// Prepares the widgets for addition of a new setting
	void createSetting();

	// Saves the setting data from the widgets to the DifficultySettings object
	void saveSetting();

	// Removes the setting selected in the treeview
	void deleteSetting();

	// Highlights the setting (according to the given <id>) in the treeview
	void selectSettingById(int id);

	// Callback for treeview selection changes
	void onSettingSelectionChange(wxDataViewEvent& ev);

	void onSettingCreate(wxCommandEvent& ev);
	void onSettingSave(wxCommandEvent& ev);
	void onSettingDelete(wxCommandEvent& ev);
	void onRefresh(wxCommandEvent& ev);

	void onAppTypeChange(wxCommandEvent& ev);
};
typedef std::shared_ptr<DifficultyEditor> DifficultyEditorPtr;

} // namespace ui
