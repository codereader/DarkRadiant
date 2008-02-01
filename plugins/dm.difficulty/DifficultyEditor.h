#ifndef DIFFICULTY_EDITOR_H_
#define DIFFICULTY_EDITOR_H_

#include <string>
#include <boost/shared_ptr.hpp>

#include "DifficultySettings.h"

typedef struct _GtkWidget GtkWidget;
typedef struct _GtkTreeStore GtkTreeStore;
typedef struct _GtkTreeView GtkTreeView;
typedef struct _GtkTreeSelection GtkTreeSelection;

namespace ui {

/**
 * greebo: A Difficulty Editor provides the front-end for editing a 
 *         set of Difficulty Settings (e.g. "Easy" setting). The actual
 *         data is stored in a DifficultySettings instance.
 */
class DifficultyEditor
{
	// The actual settings we're working with
	difficulty::DifficultySettingsPtr _settings;

	// GtkNotebook-related widgets
	GtkWidget* _editor;
	GtkWidget* _labelHBox;
	GtkWidget* _label; // the actual label

	// The classname dropdown entry field
	GtkWidget* _editorPane;
	GtkWidget* _classCombo;
	GtkWidget* _spawnArgEntry;
	GtkWidget* _argumentEntry;
	GtkWidget* _appTypeCombo;

	// A label containing notes to the user
	GtkWidget* _noteText;

	GtkTreeStore* _settingsStore;
	GtkTreeView* _settingsView;
	GtkTreeSelection* _selection;

public:
	/**
	 * greebo: Pass the label string and the difficulty settings object to the
	 *         constructor. The DifficultySettings should be populated first.
	 */
	DifficultyEditor(const std::string& label, const difficulty::DifficultySettingsPtr& settings);

	// Returns the actual editor widget (contains all controls and views)
	GtkWidget* getEditor();

	// Returns the label for packing into a GtkNotebook tab.
	GtkWidget* getNotebookLabel();

	// Set the title label of this editor pane
	void setLabel(const std::string& label);

private:
	// Reloads the treedata from the difficulty settings
	void updateTreeModel();

	// Creates the widgets
	void populateWindow();
	GtkWidget* createTreeView();
	GtkWidget* createEditingWidgets();

	// Returns the ID of the selected setting (or -1) if no valid setting is selected
	int getSelectedSettingId();

	// Loads the data from the treeview selection into the editor widgets
	void updateEditorWidgets();
	// Saves the setting data from the widgets to the DifficultySettings object
	void saveSetting();

	// GTK Callback for treeview selection changes
	static void onSettingSelectionChange(GtkTreeSelection* treeView, DifficultyEditor* self);
	static void onSettingSave(GtkWidget* button, DifficultyEditor* self);
};
typedef boost::shared_ptr<DifficultyEditor> DifficultyEditorPtr;

} // namespace ui

#endif /* DIFFICULTY_EDITOR_H_ */