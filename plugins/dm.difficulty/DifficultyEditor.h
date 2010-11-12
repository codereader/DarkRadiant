#ifndef DIFFICULTY_EDITOR_H_
#define DIFFICULTY_EDITOR_H_

#include <string>
#include <boost/shared_ptr.hpp>

#include "DifficultySettings.h"

namespace Gtk
{
	class Widget;
	class TreeView;
	class HBox;
	class VBox;
	class Label;
	class Entry;
	class Button;
	class ComboBoxEntry;
	class ComboBox;
}

namespace ui
{

/**
 * greebo: A Difficulty Editor provides the front-end for editing a
 * set of Difficulty Settings (e.g. "Easy" setting). The actual
 * data is stored in a DifficultySettings instance.
 */
class DifficultyEditor
{
	// The actual settings we're working with
	difficulty::DifficultySettingsPtr _settings;

	// GtkNotebook-related widgets
	Gtk::VBox* _editor;
	Gtk::HBox* _labelHBox;
	Gtk::Label* _label; // the actual label

	// The classname dropdown entry field
	Gtk::VBox* _editorPane;
	Gtk::ComboBoxEntry* _classCombo;
	Gtk::Entry* _spawnArgEntry;
	Gtk::Entry* _argumentEntry;
	Gtk::ComboBox* _appTypeCombo;

	Gtk::Button* _saveSettingButton;
	Gtk::Button* _deleteSettingButton;
	Gtk::Button* _createSettingButton;
	Gtk::Button* _refreshButton;

	// A label containing notes to the user
	Gtk::Label* _noteText;

	Gtk::TreeView* _settingsView;

	// Mutex for avoiding loopbacks
	bool _updateActive;

public:
	/**
	 * greebo: Pass the label string and the difficulty settings object to the
	 *         constructor. The DifficultySettings should be populated first.
	 */
	DifficultyEditor(const std::string& label, const difficulty::DifficultySettingsPtr& settings);

	// Returns the actual editor widget (contains all controls and views)
	Gtk::Widget& getEditor();

	// Returns the label for packing into a GtkNotebook tab.
	Gtk::Widget& getNotebookLabel();

	// Set the title label of this editor pane
	void setLabel(const std::string& label);

private:
	// Creates the widgets
	void populateWindow();

	Gtk::Widget& createTreeView();
	Gtk::Widget& createEditingWidgets();

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

	// gtkmm Callback for treeview selection changes
	void onSettingSelectionChange();

	void onSettingCreate();
	void onSettingSave();
	void onSettingDelete();
	void onRefresh();

	void onAppTypeChange();
};
typedef boost::shared_ptr<DifficultyEditor> DifficultyEditorPtr;

} // namespace ui

#endif /* DIFFICULTY_EDITOR_H_ */
