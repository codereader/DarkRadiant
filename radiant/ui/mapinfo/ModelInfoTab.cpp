#include "ModelInfoTab.h"

#include <gtk/gtk.h>

#include "string/string.h"
#include "gtkutil/ScrolledFrame.h"
#include "gtkutil/TextColumn.h"

namespace ui {

	namespace {
		const std::string TAB_NAME("Models");
		const std::string TAB_ICON("model16green.png");
	   	
	   	enum {
	   		MODEL_COL,
	   		MODELCOUNT_COL,
			POLYCOUNT_COL,
			SKINCOUNT_COL,
	   		NUM_COLS
	   	};
	}

ModelInfoTab::ModelInfoTab() :
	_widget(gtk_vbox_new(FALSE, 6))
{
	// Create all the widgets
	populateTab();
}

GtkWidget* ModelInfoTab::getWidget() {
	return _widget;
}

std::string ModelInfoTab::getLabel() {
	return TAB_NAME;
}

std::string ModelInfoTab::getIconName() {
	return TAB_ICON;
}

void ModelInfoTab::populateTab() {
	// Set the outer space of the vbox
	gtk_container_set_border_width(GTK_CONTAINER(_widget), 12);
	
	// Create the list store that contains the model => info map 
	_listStore = gtk_list_store_new(4, G_TYPE_STRING, G_TYPE_INT, G_TYPE_INT, G_TYPE_INT);

	// Create the treeview and pack two columns into it
	_treeView = gtk_tree_view_new_with_model(GTK_TREE_MODEL(_listStore));
	gtk_tree_view_set_headers_clickable(GTK_TREE_VIEW(_treeView), TRUE);

	GtkTreeViewColumn* modelCol = gtkutil::TextColumn("Model", MODEL_COL);
	gtk_tree_view_column_set_sort_column_id(modelCol, MODEL_COL);
	
	GtkTreeViewColumn* polyCountCol = gtkutil::TextColumn("Polys", POLYCOUNT_COL);
	gtk_tree_view_column_set_sort_column_id(polyCountCol, POLYCOUNT_COL);

	GtkTreeViewColumn* modelCountCol = gtkutil::TextColumn("Count", MODELCOUNT_COL);
	gtk_tree_view_column_set_sort_column_id(modelCountCol, MODELCOUNT_COL);

	GtkTreeViewColumn* skinCountCol = gtkutil::TextColumn("Skins", SKINCOUNT_COL);
	gtk_tree_view_column_set_sort_column_id(skinCountCol, SKINCOUNT_COL);

	gtk_tree_view_append_column(GTK_TREE_VIEW(_treeView), modelCol);
	gtk_tree_view_append_column(GTK_TREE_VIEW(_treeView), polyCountCol);
	gtk_tree_view_append_column(GTK_TREE_VIEW(_treeView), modelCountCol);
	gtk_tree_view_append_column(GTK_TREE_VIEW(_treeView), skinCountCol);
	
    gtk_box_pack_start(GTK_BOX(_widget), gtkutil::ScrolledFrame(_treeView), TRUE, TRUE, 0);
    
    // Populate the liststore with the entity count information
    for (map::ModelBreakdown::Map::const_iterator i = _modelBreakdown.begin(); 
		 i != _modelBreakdown.end(); 
		 ++i)
	{
		GtkTreeIter iter;
		gtk_list_store_append(_listStore, &iter);
		gtk_list_store_set(_listStore, &iter, 
						   MODEL_COL, i->first.c_str(),
						   POLYCOUNT_COL, i->second.polyCount,
						   MODELCOUNT_COL, i->second.count, 
						   SKINCOUNT_COL, i->second.skinCount.size(), 
						   -1);
	}
}

} // namespace ui
