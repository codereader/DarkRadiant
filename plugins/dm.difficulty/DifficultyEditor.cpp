#include "DifficultyEditor.h"

#include "iradiant.h"
#include <gtk/gtk.h>
#include "gtkutil/ScrolledFrame.h"
#include "gtkutil/TextColumn.h"
#include "gtkutil/LeftAlignedLabel.h"
#include "gtkutil/LeftAlignment.h"
#include "gtkutil/RightAlignment.h"
#include "gtkutil/TreeModel.h"
#include "ClassNameStore.h"

namespace ui {

	namespace {
		const std::string DIFF_ICON("sr_icon_custom.png");
		const int TREE_VIEW_MIN_WIDTH = 320;
	}

DifficultyEditor::DifficultyEditor(const std::string& label, 
								   const difficulty::DifficultySettingsPtr& settings) :
	_settings(settings),
	_updateActive(false)
{
	// The tab label items (icon + label)
	_labelHBox = gtk_hbox_new(FALSE, 3);
	_label = gtk_label_new(label.c_str());

	gtk_box_pack_start(
    	GTK_BOX(_labelHBox), 
    	gtk_image_new_from_pixbuf(GlobalRadiant().getLocalPixbufWithMask(DIFF_ICON)), 
    	FALSE, FALSE, 3
    );
	gtk_box_pack_start(GTK_BOX(_labelHBox), _label, FALSE, FALSE, 3);

	// The actual editor pane
	_editor = gtk_vbox_new(FALSE, 12);

	_settings->updateTreeModel();

	populateWindow();
	updateEditorWidgets();
}

GtkWidget* DifficultyEditor::getEditor() {
	return _editor;
}

// Returns the label for packing into a GtkNotebook tab.
GtkWidget* DifficultyEditor::getNotebookLabel() {
	return _labelHBox;
}

void DifficultyEditor::setLabel(const std::string& label) {
	gtk_label_set_markup(GTK_LABEL(_label), label.c_str());
}

void DifficultyEditor::populateWindow() {
	// Pack the treeview and the editor pane into a GtkPaned
	GtkWidget* paned = gtk_hpaned_new();
	gtk_paned_add1(GTK_PANED(paned), createTreeView());
	gtk_paned_add2(GTK_PANED(paned), createEditingWidgets());

	// Pack the pane into the topmost editor container
	gtk_box_pack_start(GTK_BOX(_editor), paned, TRUE, TRUE, 0);
}

GtkWidget* DifficultyEditor::createTreeView() {
	// First, create the treeview
	_settingsView = GTK_TREE_VIEW(
		gtk_tree_view_new_with_model(GTK_TREE_MODEL(_settings->getTreeStore()))
	);
	gtk_widget_set_size_request(GTK_WIDGET(_settingsView), TREE_VIEW_MIN_WIDTH, -1);

	// Connect the tree view selection
	_selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(_settingsView));
	g_signal_connect(G_OBJECT(_selection), "changed", 
					 G_CALLBACK(onSettingSelectionChange), this);

	// Add columns to this view
	GtkCellRenderer* textRenderer = gtk_cell_renderer_text_new();

	GtkTreeViewColumn* settingCol = gtk_tree_view_column_new();
	gtk_tree_view_column_pack_start(settingCol, textRenderer, FALSE);

    gtk_tree_view_column_set_title(settingCol, "Setting");
	gtk_tree_view_column_set_sizing(settingCol, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
    gtk_tree_view_column_set_spacing(settingCol, 3);

	gtk_tree_view_append_column(_settingsView, settingCol);

	gtk_tree_view_column_set_attributes(settingCol, textRenderer,
                                        "text", COL_DESCRIPTION,
                                        "foreground", COL_TEXTCOLOUR,
										"strikethrough", COL_IS_OVERRIDDEN,
                                        NULL);

	GtkWidget* frame = gtkutil::ScrolledFrame(GTK_WIDGET(_settingsView));
	
	// Create the action buttons
	GtkWidget* buttonHBox = gtk_hbox_new(FALSE, 6);
	
	// Create button
	_createSettingButton = gtk_button_new_from_stock(GTK_STOCK_ADD);
	g_signal_connect(G_OBJECT(_createSettingButton), "clicked", G_CALLBACK(onSettingCreate), this);

	// Delete button
	_deleteSettingButton = gtk_button_new_from_stock(GTK_STOCK_DELETE);
	g_signal_connect(G_OBJECT(_deleteSettingButton), "clicked", G_CALLBACK(onSettingDelete), this);

	gtk_box_pack_start(GTK_BOX(buttonHBox), _createSettingButton, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(buttonHBox), _deleteSettingButton, TRUE, TRUE, 0);

	GtkWidget* vbox = gtk_vbox_new(FALSE, 6);
	gtk_box_pack_start(GTK_BOX(vbox), frame, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), buttonHBox, FALSE, FALSE, 0);

	gtk_container_set_border_width(GTK_CONTAINER(vbox), 12);

	return vbox;
}

GtkWidget* DifficultyEditor::createEditingWidgets() {
	GtkWidget* vbox = gtk_vbox_new(FALSE, 6);
	_editorPane = vbox;
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 12);

	// The "Settings" label
	GtkWidget* settingsLabel = gtkutil::LeftAlignedLabel("<b>Setting</b>");
	gtk_box_pack_start(GTK_BOX(vbox), settingsLabel, FALSE, FALSE, 0);

	// The table aligning the editing widgets
	GtkTable* table = GTK_TABLE(gtk_table_new(3, 2, false));
    gtk_table_set_col_spacings(table, 12);
    gtk_table_set_row_spacings(table, 6);

	gtk_box_pack_start(GTK_BOX(vbox), 
		gtkutil::LeftAlignment(GTK_WIDGET(table), 18, 1.0), 
		FALSE, FALSE, 0);

	// ===== CLASSNAME ======

	GtkWidget* classNameLabel = gtkutil::LeftAlignedLabel("Classname:");

	// Add classname widget
	_classCombo = gtk_combo_box_entry_new_with_model(
		ClassNameStore::getModel(),
		ClassNameStore::CLASSNAME_COL
	); 

	// Add completion functionality to the combobox entry
	GtkEntryCompletion* completion = gtk_entry_completion_new();
	gtk_entry_completion_set_model(completion, ClassNameStore::getModel());
	gtk_entry_completion_set_text_column(completion, ClassNameStore::CLASSNAME_COL);
	gtk_entry_set_completion(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(_classCombo))), 
							 completion);

	// Sort the list alphabetically
	gtk_tree_sortable_set_sort_column_id(
		GTK_TREE_SORTABLE(ClassNameStore::getModel()), 
		ClassNameStore::CLASSNAME_COL, GTK_SORT_ASCENDING
	);

	gtk_table_attach(table, classNameLabel, 0, 1, 0, 1, GTK_FILL, (GtkAttachOptions)0, 0, 0);
	gtk_table_attach_defaults(table, _classCombo, 1, 2, 0, 1);

	// ===== SPAWNARG ======
	_spawnArgEntry = gtk_entry_new();
	GtkWidget* spawnArgLabel = gtkutil::LeftAlignedLabel("Spawnarg:");

	gtk_table_attach(table, spawnArgLabel, 0, 1, 1, 2, GTK_FILL, (GtkAttachOptions)0, 0, 0);
	gtk_table_attach_defaults(table, _spawnArgEntry, 1, 2, 1, 2);

	// ===== ARGUMENT ======
	_argumentEntry = gtk_entry_new();
	GtkWidget* argumentLabel = gtkutil::LeftAlignedLabel("Argument:");

	// The appType chooser
	GtkTreeModel* model = GTK_TREE_MODEL(difficulty::Setting::getAppTypeStore());
	_appTypeCombo = gtk_combo_box_new_with_model(model);
	g_object_unref(model);
	g_signal_connect(G_OBJECT(_appTypeCombo), "changed", G_CALLBACK(onAppTypeChange), this);

	// Add the cellrenderer for the apptype text
	GtkCellRenderer* appTypeRenderer = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(_appTypeCombo), appTypeRenderer, FALSE);
	gtk_cell_layout_add_attribute(GTK_CELL_LAYOUT(_appTypeCombo), appTypeRenderer, "text", 0);

	// Pack the argument entry and the appType dropdown field together
	GtkWidget* argHBox = gtk_hbox_new(FALSE, 6);
	gtk_box_pack_start(GTK_BOX(argHBox), _argumentEntry, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(argHBox), _appTypeCombo, FALSE, FALSE, 0);

	gtk_table_attach(table, argumentLabel, 0, 1, 2, 3, GTK_FILL, (GtkAttachOptions)0, 0, 0);
	gtk_table_attach_defaults(table, argHBox, 1, 2, 2, 3);

	// Save button
	GtkWidget* buttonHbox = gtk_hbox_new(FALSE, 6);

	_saveSettingButton = gtk_button_new_from_stock(GTK_STOCK_SAVE);
	g_signal_connect(G_OBJECT(_saveSettingButton), "clicked", G_CALLBACK(onSettingSave), this);
	gtk_box_pack_start(GTK_BOX(buttonHbox), _saveSettingButton, FALSE, FALSE, 0);

	gtk_box_pack_start(GTK_BOX(vbox), gtkutil::RightAlignment(buttonHbox), FALSE, FALSE, 0);

	// The "note" text
	_noteText = gtk_label_new("");
	gtk_label_set_line_wrap(GTK_LABEL(_noteText), TRUE);
	gtk_box_pack_start(GTK_BOX(vbox), gtkutil::LeftAlignment(_noteText), FALSE, FALSE, 6);

	return vbox;
}

int DifficultyEditor::getSelectedSettingId() {
	GtkTreeIter iter;
	GtkTreeModel* model;
	gboolean anythingSelected = gtk_tree_selection_get_selected(_selection, &model, &iter);

	if (anythingSelected) {
		return gtkutil::TreeModel::getInt(model, &iter, COL_SETTING_ID);
	}
	else {
		return -1;
	}
}

void DifficultyEditor::updateEditorWidgets() {
	_updateActive = true;

	int id = getSelectedSettingId();

	gboolean editWidgetsSensitive = FALSE;

	std::string noteText;

	if (id != -1) {
		// Lookup the setting using className/id combo
		difficulty::SettingPtr setting = _settings->getSettingById(id);

		if (setting != NULL) {
			// Activate editing pane
			editWidgetsSensitive = TRUE;

			if (_settings->isOverridden(setting)) {
				editWidgetsSensitive = FALSE;
				noteText += "This default setting is overridden, cannot edit.";
			}

			gtk_entry_set_text(GTK_ENTRY(_spawnArgEntry), setting->spawnArg.c_str());
			gtk_entry_set_text(GTK_ENTRY(_argumentEntry), setting->argument.c_str());

			// Now select the eclass passed in the argument
			// Find the entity using a TreeModel traversor
			gtkutil::TreeModel::SelectionFinder finder(
				setting->className, ClassNameStore::CLASSNAME_COL
			);

			gtk_tree_model_foreach(
				ClassNameStore::getModel(), 
				gtkutil::TreeModel::SelectionFinder::forEach, 
				&finder
			);
			
			// Select the found treeiter, if the name was found in the liststore
			if (finder.getPath() != NULL) {
				GtkTreeIter iter = finder.getIter();
				gtk_combo_box_set_active_iter(GTK_COMBO_BOX(_classCombo), &iter);
			}

			// Select the appType in the dropdown combo box (search the second column)
			gtkutil::TreeModel::SelectionFinder appTypeFinder(setting->appType, 1); 
			gtk_tree_model_foreach(
				gtk_combo_box_get_model(GTK_COMBO_BOX(_appTypeCombo)),
				gtkutil::TreeModel::SelectionFinder::forEach,
				&appTypeFinder
			);

			// Select the found treeiter, if the name was found in the liststore
			if (appTypeFinder.getPath() != NULL) {
				GtkTreeIter iter = appTypeFinder.getIter();
				gtk_combo_box_set_active_iter(GTK_COMBO_BOX(_appTypeCombo), &iter);
			}

			// Set the sensitivity of the argument entry box		
			gtk_widget_set_sensitive(
				_argumentEntry, 
				(setting->appType == difficulty::Setting::EIgnore) ? FALSE : TRUE
			);

			// We have a treeview selection, lock the classname
			gtk_widget_set_sensitive(_classCombo, FALSE);

			// Disable the deletion of default settings
			gtk_widget_set_sensitive(_deleteSettingButton, (setting->isDefault) ? FALSE : TRUE);
			gtk_widget_set_sensitive(_saveSettingButton, TRUE);
		}
	}
	else {
		// Nothing selected, disable deletion 
		gtk_widget_set_sensitive(_deleteSettingButton, FALSE);
		gtk_widget_set_sensitive(_saveSettingButton, FALSE);
	}

	// Set editing pane sensitivity
	gtk_widget_set_sensitive(_editorPane, editWidgetsSensitive);

	// Set the note text in any case
	gtk_label_set_markup(GTK_LABEL(_noteText), noteText.c_str());

	_updateActive = false;
}

void DifficultyEditor::createSetting() {
	// Unselect everything
	gtk_tree_selection_unselect_all(_selection);

	// Unlock editing widgets 
	gtk_widget_set_sensitive(_editorPane, TRUE);
	// Unlock class combo
	gtk_widget_set_sensitive(_classCombo, TRUE);
	gtk_widget_set_sensitive(_saveSettingButton, TRUE);

	gtk_entry_set_text(GTK_ENTRY(_spawnArgEntry), "");
	gtk_entry_set_text(GTK_ENTRY(_argumentEntry), "");
}

void DifficultyEditor::saveSetting() {
	// Get the ID of the currently selected item (might be -1 if no selection)
	int id = getSelectedSettingId();

	// Instantiate a new setting and fill the data in
	difficulty::SettingPtr setting(new difficulty::Setting);

	// Load the widget contents
	setting->className = gtk_combo_box_get_active_text(GTK_COMBO_BOX(_classCombo));
	setting->spawnArg = gtk_entry_get_text(GTK_ENTRY(_spawnArgEntry));
	setting->argument = gtk_entry_get_text(GTK_ENTRY(_argumentEntry));

	// Get the apptype from the dropdown list
	setting->appType = difficulty::Setting::EAssign;

	GtkTreeIter iter;
	if (gtk_combo_box_get_active_iter(GTK_COMBO_BOX(_appTypeCombo), &iter)) {
		setting->appType = static_cast<difficulty::Setting::EApplicationType>(
			gtkutil::TreeModel::getInt(gtk_combo_box_get_model(GTK_COMBO_BOX(_appTypeCombo)), &iter, 1)
		);
	}

	// Pass the data to the DifficultySettings class to handle it
	id = _settings->save(id, setting);

	// Update the treemodel
	_settings->updateTreeModel();

	// Highlight the setting, it might have been newly created
	selectSettingById(id);
}

void DifficultyEditor::deleteSetting() {
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

void DifficultyEditor::selectSettingById(int id) {
	// Use the local SelectionFinder class to walk the TreeModel
	gtkutil::TreeModel::SelectionFinder finder(id, COL_SETTING_ID);
	GtkTreeModel* model = gtk_tree_view_get_model(GTK_TREE_VIEW(_settingsView));
	gtk_tree_model_foreach(model, gtkutil::TreeModel::SelectionFinder::forEach, &finder);
	
	// Get the found TreePath (may be NULL)
	GtkTreePath* path = finder.getPath();
	if (path != NULL) {
		// Expand the treeview to display the target row
		gtk_tree_view_expand_to_path(GTK_TREE_VIEW(_settingsView), path);
		// Highlight the target row
		gtk_tree_view_set_cursor(GTK_TREE_VIEW(_settingsView), path, NULL, false);
		// Make the selected row visible 
		gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(_settingsView), path, NULL, true, 0.3f, 0.0f);
		// Select the item
		gtk_tree_selection_select_path(_selection, path);
	}
}

void DifficultyEditor::onSettingSelectionChange(
	GtkTreeSelection* treeView, DifficultyEditor* self)
{
	// Update editor widgets
	self->updateEditorWidgets();
}

void DifficultyEditor::onSettingSave(GtkWidget* button, DifficultyEditor* self) {
	self->saveSetting();
}

void DifficultyEditor::onSettingDelete(GtkWidget* button, DifficultyEditor* self) {
	self->deleteSetting();
}

void DifficultyEditor::onSettingCreate(GtkWidget* button, DifficultyEditor* self) {
	self->createSetting();
}

void DifficultyEditor::onAppTypeChange(GtkComboBox* appTypeCombo, DifficultyEditor* self) {
	if (self->_updateActive) return;

	// Update the sensitivity of the argument entry widget
	GtkTreeIter iter;
	if (gtk_combo_box_get_active_iter(appTypeCombo, &iter)) {

		typedef difficulty::Setting::EApplicationType AppType; // shortcut

		AppType appType = static_cast<AppType>(gtkutil::TreeModel::getInt(
			gtk_combo_box_get_model(GTK_COMBO_BOX(self->_appTypeCombo)), &iter, 1
		));

		gtk_widget_set_sensitive(
			self->_argumentEntry, 
			(appType == difficulty::Setting::EIgnore) ? FALSE : TRUE
		);
	}
}

} // namespace ui
