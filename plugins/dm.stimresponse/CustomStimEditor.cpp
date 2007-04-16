#include "CustomStimEditor.h"

#include <gtk/gtk.h>
#include "gtkutil/ScrolledFrame.h"

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
	gtk_tree_view_set_model(GTK_TREE_VIEW(_list), GTK_TREE_MODEL(listStore));
	g_object_unref(listStore); // treeview owns reference now

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
	
	gtk_box_pack_start(GTK_BOX(_pageHBox), gtkutil::ScrolledFrame(_list), TRUE, TRUE, 0);
}
	
} // namespace ui
