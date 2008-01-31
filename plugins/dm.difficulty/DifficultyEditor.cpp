#include "DifficultyEditor.h"

#include "iradiant.h"
#include <gtk/gtk.h>
#include "gtkutil/ScrolledFrame.h"
#include "gtkutil/TextColumn.h"
#include "gtkutil/LeftalignedLabel.h"
#include "gtkutil/LeftAlignment.h"
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
                                        NULL);

	return gtkutil::ScrolledFrame(GTK_WIDGET(_settingsView));
}

GtkWidget* DifficultyEditor::createEditingWidgets() {
	GtkWidget* vbox = gtk_vbox_new(FALSE, 6);
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

	return vbox;
}

} // namespace ui
