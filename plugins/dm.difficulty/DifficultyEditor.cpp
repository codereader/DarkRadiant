#include "DifficultyEditor.h"

#include "iradiant.h"
#include <gtk/gtk.h>
#include "gtkutil/ScrolledFrame.h"
#include "gtkutil/TextColumn.h"
#include "gtkutil/LeftalignedLabel.h"
#include "gtkutil/LeftAlignment.h"
#include "gtkutil/RightAlignment.h"
#include "gtkutil/TreeModel.h"
#include "ClassNameStore.h"

namespace ui {

	namespace {
		const std::string DIFF_ICON("sr_icon_custom.png");
		const int TREE_VIEW_MIN_WIDTH = 400;
	}

DifficultyEditor::DifficultyEditor(const std::string& label, 
								   const difficulty::DifficultySettingsPtr& settings) :
	_settings(settings),
	_settingsStore(gtk_tree_store_new(NUM_SETTINGS_COLS, 
									  G_TYPE_STRING, // description
									  G_TYPE_STRING, // text colour
									  G_TYPE_STRING, // classname
									  G_TYPE_INT,    // setting id
									  G_TYPE_BOOLEAN,// overridden?
									  -1))
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

	updateTreeModel();

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

void DifficultyEditor::updateTreeModel() {
	_settings->updateTreeModel(_settingsStore);
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
		gtk_tree_view_new_with_model(GTK_TREE_MODEL(_settingsStore))
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
	gtk_container_set_border_width(GTK_CONTAINER(frame), 12);
	return frame;
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

	// ===== VALUE ======
	_argumentEntry = gtk_entry_new();
	GtkWidget* argumentLabel = gtkutil::LeftAlignedLabel("Argument:");

	gtk_table_attach(table, argumentLabel, 0, 1, 2, 3, GTK_FILL, (GtkAttachOptions)0, 0, 0);
	gtk_table_attach_defaults(table, _argumentEntry, 1, 2, 2, 3);

	// Save button
	GtkWidget* saveButton = gtk_button_new_from_stock(GTK_STOCK_SAVE);
	g_signal_connect(G_OBJECT(saveButton), "clicked", G_CALLBACK(onSettingSave), this);

	gtk_box_pack_start(GTK_BOX(vbox), gtkutil::RightAlignment(saveButton), FALSE, FALSE, 0);

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
	int id = getSelectedSettingId();

	gboolean widgetSensitive = FALSE;
	
	if (id != -1) {
		// Lookup the setting using className/id combo
		difficulty::SettingPtr setting = _settings->getSettingById(id);

		if (setting != NULL) {
			// Activate editing pane
			widgetSensitive = TRUE;

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

			// We have a treeview selection, lock the classname
			gtk_widget_set_sensitive(_classCombo, FALSE);
		}
	}
	
	// Set editing pane sensitivity
	gtk_widget_set_sensitive(_editorPane, widgetSensitive);
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

	// Pass the data to the DifficultySettings class to handle it
	_settings->save(id, setting);
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

} // namespace ui
