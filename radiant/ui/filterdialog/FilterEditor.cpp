#include "FilterEditor.h"

#include <gtk/gtk.h>
#include "gtkutil/RightAlignment.h"
#include "gtkutil/LeftAlignment.h"
#include "gtkutil/LeftAlignedLabel.h"
#include "gtkutil/TextColumn.h"
#include "gtkutil/TreeModel.h"
#include "gtkutil/ScrolledFrame.h"

namespace ui {

	namespace {
		const int DEFAULT_SIZE_X = 500;
	    const int DEFAULT_SIZE_Y = 300;
		const std::string WINDOW_TITLE = "Edit Filter";

		enum {
			COL_INDEX,
			COL_TYPE,
			COL_TYPE_STR,
			COL_REGEX,
			COL_ACTION,
			NUM_COLS
		};
	}

FilterEditor::FilterEditor(Filter& filter, GtkWindow* parent) :
	BlockingTransientWindow(WINDOW_TITLE, parent),
	_originalFilter(filter),
	_filter(_originalFilter), // copy-construct
	_criteriaStore(gtk_list_store_new(NUM_COLS, G_TYPE_INT,	// index
												G_TYPE_INT, // type
												G_TYPE_STRING, // type string
												G_TYPE_STRING, // regex match
												G_TYPE_STRING))  // show/hide
{
	gtk_window_set_default_size(GTK_WINDOW(getWindow()), DEFAULT_SIZE_X, DEFAULT_SIZE_Y);
	gtk_container_set_border_width(GTK_CONTAINER(getWindow()), 12);
	gtk_window_set_type_hint(GTK_WINDOW(getWindow()), GDK_WINDOW_TYPE_HINT_DIALOG);

	// Create the widgets
	populateWindow();

	// Update the widget contents
	update();

	// Show and block
	show();
}

void FilterEditor::populateWindow() {
	// Create the dialog vbox
	GtkWidget* vbox = gtk_vbox_new(FALSE, 6);

	// Create the "Filters" label	
	gtk_box_pack_start(GTK_BOX(vbox), gtkutil::LeftAlignedLabel("<b>Name</b>"), FALSE, FALSE, 0);

	// Pack the treeview into the main window's vbox
	gtk_box_pack_start(GTK_BOX(vbox), createCriteriaPanel(), TRUE, TRUE, 0);

	// Buttons
	gtk_box_pack_start(GTK_BOX(vbox), createButtonPanel(), FALSE, FALSE, 0);

	gtk_container_add(GTK_CONTAINER(getWindow()), GTK_WIDGET(vbox));
}

void FilterEditor::update() {
	// Populate the criteria store
	gtk_list_store_clear(_criteriaStore);

	// Traverse the criteria of the Filter to be edited
	for (std::size_t i = 0; i < _filter.rules.size(); ++i) {
		GtkTreeIter iter;
		const FilterRule& rule = _filter.rules[i];

		// Allocate a new list store element and store its pointer into <iter>
		gtk_list_store_append(_criteriaStore, &iter);

		int typeIndex = getTypeIndexForString(rule.type);
		
		gtk_list_store_set(_criteriaStore, &iter, 
			COL_INDEX, static_cast<int>(i), 
			COL_TYPE, typeIndex,
			COL_TYPE_STR, rule.type.c_str(),
			COL_REGEX, rule.match.c_str(),
			COL_ACTION, rule.show ? "show" : "hide",
			-1
		);
	}
}

GtkWidget* FilterEditor::createCriteriaPanel() {
	// Create an hbox for the treeview and the action buttons
	GtkWidget* hbox = gtk_hbox_new(FALSE, 6);

	// Create a new treeview
	_criteriaView = GTK_TREE_VIEW(gtk_tree_view_new_with_model(GTK_TREE_MODEL(_criteriaStore)));
		
	gtkutil::TextColumn indexCol("Index", COL_INDEX);
	gtkutil::TextColumn regexCol("Match", COL_REGEX);
	gtkutil::TextColumn actionCol("Action", COL_ACTION);

	// Create the cell renderer for the type choice
	GtkCellRenderer* typeComboRenderer = gtk_cell_renderer_combo_new();
	g_object_set(G_OBJECT(typeComboRenderer), "has-entry", FALSE, NULL);
	g_object_set(G_OBJECT(typeComboRenderer), "text-column", 1, NULL);
	g_object_set(G_OBJECT(typeComboRenderer), "editable", TRUE, NULL);

	// Create the typestore
	GtkListStore* typeStore = createTypeStore();
	g_object_set(G_OBJECT(typeComboRenderer), "model", GTK_TREE_MODEL(typeStore), NULL);

	// Construct the column itself
	GtkTreeViewColumn* typeCol = gtk_tree_view_column_new_with_attributes(
		"Type", 
		typeComboRenderer, 
		"markup", COL_TYPE_STR,
		NULL
	);
	g_signal_connect(G_OBJECT(typeComboRenderer), "edited", G_CALLBACK(onTypeEdited), this);

	GtkCellRendererText* rend = regexCol.getCellRenderer();
	g_object_set(G_OBJECT(rend), "editable", TRUE, NULL);
	g_signal_connect(G_OBJECT(rend), "edited", G_CALLBACK(onRegexEdited), this);

	gtk_tree_view_append_column(GTK_TREE_VIEW(_criteriaView), indexCol);
	gtk_tree_view_append_column(GTK_TREE_VIEW(_criteriaView), typeCol);
	gtk_tree_view_append_column(GTK_TREE_VIEW(_criteriaView), regexCol);
	gtk_tree_view_append_column(GTK_TREE_VIEW(_criteriaView), actionCol);

	GtkTreeSelection* sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(_criteriaView));
	g_signal_connect(G_OBJECT(sel), "changed", G_CALLBACK(onCriterionSelectionChanged), this);

	// Action buttons
	/*_widgets[WIDGET_ADD_FILTER_BUTTON] = gtk_button_new_from_stock(GTK_STOCK_ADD);
	_widgets[WIDGET_EDIT_FILTER_BUTTON] = gtk_button_new_from_stock(GTK_STOCK_EDIT);
	_widgets[WIDGET_DELETE_FILTER_BUTTON] = gtk_button_new_from_stock(GTK_STOCK_DELETE);

	g_signal_connect(G_OBJECT(_widgets[WIDGET_ADD_FILTER_BUTTON]), "clicked", G_CALLBACK(onAddFilter), this);
	g_signal_connect(G_OBJECT(_widgets[WIDGET_EDIT_FILTER_BUTTON]), "clicked", G_CALLBACK(onEditFilter), this);
	g_signal_connect(G_OBJECT(_widgets[WIDGET_DELETE_FILTER_BUTTON]), "clicked", G_CALLBACK(onDeleteFilter), this);

	GtkWidget* actionVBox = gtk_vbox_new(FALSE, 6);

	gtk_box_pack_start(GTK_BOX(actionVBox), _widgets[WIDGET_ADD_FILTER_BUTTON], FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(actionVBox), _widgets[WIDGET_EDIT_FILTER_BUTTON], FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(actionVBox), _widgets[WIDGET_DELETE_FILTER_BUTTON], FALSE, FALSE, 0);*/

	gtk_box_pack_start(GTK_BOX(hbox), gtkutil::ScrolledFrame(GTK_WIDGET(_criteriaView)), TRUE, TRUE, 0);
	//gtk_box_pack_start(GTK_BOX(hbox), actionVBox, FALSE, FALSE, 0);

	return gtkutil::LeftAlignment(hbox, 18, 1);
}

GtkListStore* FilterEditor::createTypeStore() {
	// Create the typestore
	GtkTreeIter iter;
	int index = 0;

	GtkListStore* typeStore = gtk_list_store_new(2, G_TYPE_INT, G_TYPE_STRING);
	
	gtk_list_store_append(typeStore, &iter);
	gtk_list_store_set(typeStore, &iter, 0, index++, 1, "entityclass", -1);

	gtk_list_store_append(typeStore, &iter);
	gtk_list_store_set(typeStore, &iter, 0, index++, 1, "texture", -1);

	gtk_list_store_append(typeStore, &iter);
	gtk_list_store_set(typeStore, &iter, 0, index++, 1, "object", -1);

	return typeStore;
}

GtkWidget* FilterEditor::createButtonPanel() {
	GtkWidget* buttonHBox = gtk_hbox_new(TRUE, 12);
	
	// Save button
	GtkWidget* okButton = gtk_button_new_from_stock(GTK_STOCK_OK);
	g_signal_connect(G_OBJECT(okButton), "clicked", G_CALLBACK(onSave), this);
	gtk_box_pack_end(GTK_BOX(buttonHBox), okButton, TRUE, TRUE, 0);
	
	// Cancel Button
	GtkWidget* cancelButton = gtk_button_new_from_stock(GTK_STOCK_CANCEL);
	g_signal_connect(G_OBJECT(cancelButton), "clicked", G_CALLBACK(onCancel), this);
	gtk_box_pack_end(GTK_BOX(buttonHBox), cancelButton, TRUE, TRUE, 0);
	
	return gtkutil::RightAlignment(buttonHBox);	
}

int FilterEditor::getTypeIndexForString(const std::string& type) {
	// Switch on the string
	if (type == "entityclass") {
		return 0;
	}
	else if (type == "texture") {
		return 1;
	}
	else if (type == "object") {
		return 2;
	}
	
	return -1;
}

void FilterEditor::save() {
	// Copy the working set over the actual Filter
	_originalFilter = _filter;
}

void FilterEditor::onSave(GtkWidget* widget, FilterEditor* self) {
	self->save();
	self->destroy();
}

void FilterEditor::onCancel(GtkWidget* widget, FilterEditor* self) {
	self->destroy();
}

void FilterEditor::onRegexEdited(GtkCellRendererText* renderer, gchar* path, gchar* new_text, FilterEditor* self) 
{
	GtkTreeIter iter;
	if (gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(self->_criteriaStore), &iter, path)) {
		// The iter points to the edited cell now, get the criterion number
		int index = gtkutil::TreeModel::getInt(GTK_TREE_MODEL(self->_criteriaStore), &iter, COL_INDEX);
		
		// Update the criterion
		assert(index >= 0 && index < self->_filter.rules.size());

		self->_filter.rules[index].match = new_text;

		// Update the liststore item
		gtk_list_store_set(self->_criteriaStore, &iter, COL_REGEX, new_text, -1);
	}
}

void FilterEditor::onTypeEdited(GtkCellRendererText* renderer, gchar* path, gchar* new_text, FilterEditor* self) 
{
	GtkTreeIter iter;
	if (gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(self->_criteriaStore), &iter, path)) {
		// Look up the type index for "new_text"
		int typeIndex = self->getTypeIndexForString(new_text);

		// The iter points to the edited cell, get the criterion number
		int index = gtkutil::TreeModel::getInt(GTK_TREE_MODEL(self->_criteriaStore), &iter, COL_INDEX);
		
		// Update the criterion
		assert(index >= 0 && index < self->_filter.rules.size());

		self->_filter.rules[index].type = new_text;

		// Update the liststore item
		gtk_list_store_set(self->_criteriaStore, &iter, 
			COL_TYPE, typeIndex,
			COL_TYPE_STR, new_text, 
			-1
		);
	}
}

void FilterEditor::onCriterionSelectionChanged(GtkTreeSelection* sel, FilterEditor* self) {
	// TODO
}

} // namespace ui
