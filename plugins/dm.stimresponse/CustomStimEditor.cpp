#include "CustomStimEditor.h"

#include <gtk/gtk.h>
#include "gtkutil/ScrolledFrame.h"
#include "gtkutil/TreeModel.h"

#include "SREntity.h"

namespace ui {

	namespace {
		const unsigned int TREE_VIEW_WIDTH = 250;
		const unsigned int TREE_VIEW_HEIGHT = 200;
	}

/** greebo: Constructor creates all the widgets
 */
CustomStimEditor::CustomStimEditor(StimTypes& stimTypes) :
	_stimTypes(stimTypes)
{
	populatePage();
}

CustomStimEditor::operator GtkWidget*() {
	gtk_widget_show_all(_pageHBox);
	return _pageHBox;
}

/** greebo: As the name states, this creates the context menu widgets.
 */
void CustomStimEditor::createContextMenu() {
	
}

/** greebo: Creates all the widgets
 */
void CustomStimEditor::populatePage() {
	_pageHBox = gtk_hbox_new(FALSE, 6);
	gtk_container_set_border_width(GTK_CONTAINER(_pageHBox), 6);
	
	_list = gtk_tree_view_new();
	gtk_widget_set_size_request(_list, TREE_VIEW_WIDTH, TREE_VIEW_HEIGHT);
	
	_selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(_list));

	GtkListStore* listStore = _stimTypes;
	
	// Setup a treemodel filter to display the custom stims only
	GtkTreeModel* filter = gtk_tree_model_filter_new(GTK_TREE_MODEL(listStore), NULL);
	gtk_tree_model_filter_set_visible_column(GTK_TREE_MODEL_FILTER(filter), ST_CUSTOM_COL);
	
	gtk_tree_view_set_model(GTK_TREE_VIEW(_list), GTK_TREE_MODEL(filter));
	g_object_unref(filter); // treeview owns reference now

	// Connect the signals to the callbacks
	/*g_signal_connect(G_OBJECT(_selection), "changed", 
					 G_CALLBACK(onSRSelectionChange), this);
	g_signal_connect(G_OBJECT(_list), "key-press-event", 
					 G_CALLBACK(onTreeViewKeyPress), this);
	g_signal_connect(G_OBJECT(_list), "button-release-event", 
					 G_CALLBACK(onTreeViewButtonRelease), this);*/
					 
	// Add the columns to the treeview
	// ID number
	GtkTreeViewColumn* numCol = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(numCol, "ID");
	GtkCellRenderer* numRenderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(numCol, numRenderer, FALSE);
	gtk_tree_view_column_set_attributes(numCol, numRenderer, 
										"text", ST_ID_COL,
										NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(_list), numCol);
	
	// The Type
	GtkTreeViewColumn* typeCol = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(typeCol, "Type");
	
	GtkCellRenderer* typeIconRenderer = gtk_cell_renderer_pixbuf_new();
	gtk_tree_view_column_pack_start(typeCol, typeIconRenderer, FALSE);
	
	GtkCellRenderer* typeTextRenderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(typeCol, typeTextRenderer, FALSE);
	
	gtk_tree_view_column_set_attributes(typeCol, typeTextRenderer, 
										"text", ST_CAPTION_COL,
										NULL);
	gtk_tree_view_column_set_attributes(typeCol, typeIconRenderer, 
										"pixbuf", ST_ICON_COL,
										NULL);

	gtk_tree_view_append_column(GTK_TREE_VIEW(_list), typeCol);
	
	GtkWidget* listVBox = gtk_vbox_new(FALSE, 3);
	gtk_box_pack_start(GTK_BOX(listVBox), gtkutil::ScrolledFrame(_list), TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(listVBox), createListButtons(), FALSE, FALSE, 0);
	
	gtk_box_pack_start(GTK_BOX(_pageHBox), listVBox, FALSE, FALSE, 0);
}

GtkWidget* CustomStimEditor::createListButtons() {
	GtkWidget* hbox = gtk_hbox_new(TRUE, 6);
	
	_listButtons.add = gtk_button_new_with_label("Add Stim Type");
	gtk_button_set_image(
		GTK_BUTTON(_listButtons.add), 
		gtk_image_new_from_stock(GTK_STOCK_ADD, GTK_ICON_SIZE_BUTTON)
	);
	
	_listButtons.remove = gtk_button_new_with_label("Remove Stim Type");
	gtk_button_set_image(
		GTK_BUTTON(_listButtons.remove), 
		gtk_image_new_from_stock(GTK_STOCK_DELETE, GTK_ICON_SIZE_BUTTON)
	);
	
	gtk_box_pack_start(GTK_BOX(hbox), _listButtons.add, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), _listButtons.remove, TRUE, TRUE, 0);
	
	g_signal_connect(G_OBJECT(_listButtons.add), "clicked", G_CALLBACK(onAddStimType), this);
	g_signal_connect(G_OBJECT(_listButtons.remove), "clicked", G_CALLBACK(onRemoveStimType), this);
	
	return hbox; 
}

void CustomStimEditor::selectId(int id) {
	// Setup the selectionfinder to search for the id
	gtkutil::TreeModel::SelectionFinder finder(id, ST_ID_COL);

	gtk_tree_model_foreach(
		gtk_tree_view_get_model(GTK_TREE_VIEW(_list)),
		gtkutil::TreeModel::SelectionFinder::forEach,
		&finder
	);
	
	if (finder.getPath() != NULL) {
		GtkTreeIter iter = finder.getIter();
		// Set the active row of the list to the given effect
		gtk_tree_selection_select_iter(_selection, &iter);
	}
}

void CustomStimEditor::addStimType() {
	// Add a new stim type with the lowest free custom id
	int id = _stimTypes.getFreeCustomStimId();
	
	_stimTypes.add(id,
				   "Custom",
				   "CustomStimType",
				   "Custom Stim",
				   ICON_CUSTOM_STIM,
				   true);
	
	selectId(id);
}

void CustomStimEditor::removeStimType() {
	
}

void CustomStimEditor::onAddStimType(GtkWidget* button, CustomStimEditor* self) {
	self->addStimType();
}

void CustomStimEditor::onRemoveStimType(GtkWidget* button, CustomStimEditor* self) {
	// Delete the selected stim type from the list
	self->removeStimType();
}
	
} // namespace ui
