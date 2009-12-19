#include "EntityInfoTab.h"

#include <gtk/gtk.h>
#include "iradiant.h"
#include "icounter.h"

#include "string/string.h"
#include "gtkutil/ScrolledFrame.h"
#include "gtkutil/TextColumn.h"
#include "gtkutil/LeftAlignedLabel.h"

namespace ui {

	namespace {
		const std::string TAB_NAME("Entities");
		const std::string TAB_ICON("cmenu_add_entity.png");

	   	enum {
	   		ECLASS_COL,
	   		COUNT_COL,
	   		NUM_COLS
	   	};
	}

EntityInfoTab::EntityInfoTab() :
	_widget(gtk_vbox_new(FALSE, 6))
{
	// Create all the widgets
	populateTab();
}

GtkWidget* EntityInfoTab::getWidget() {
	return _widget;
}

std::string EntityInfoTab::getLabel() {
	return TAB_NAME;
}

std::string EntityInfoTab::getIconName() {
	return TAB_ICON;
}

void EntityInfoTab::populateTab() {
	// Set the outer space of the vbox
	gtk_container_set_border_width(GTK_CONTAINER(_widget), 12);

	// Create the list store that contains the eclass => count map 
	_listStore = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_INT);

	// Create the treeview and pack two columns into it
	_treeView = gtk_tree_view_new_with_model(GTK_TREE_MODEL(_listStore));
	gtk_tree_view_set_headers_clickable(GTK_TREE_VIEW(_treeView), TRUE);

	GtkTreeViewColumn* eclassCol = gtkutil::TextColumn("Entity Class", ECLASS_COL);
	gtk_tree_view_column_set_sort_column_id(eclassCol, ECLASS_COL);
	
	GtkTreeViewColumn* countCol = gtkutil::TextColumn("Count", COUNT_COL);
	gtk_tree_view_column_set_sort_column_id(countCol, COUNT_COL);

	gtk_tree_view_append_column(GTK_TREE_VIEW(_treeView), eclassCol);
	gtk_tree_view_append_column(GTK_TREE_VIEW(_treeView), countCol);
	
    gtk_box_pack_start(GTK_BOX(_widget), gtkutil::ScrolledFrame(_treeView), TRUE, TRUE, 0);
    
    // Populate the liststore with the entity count information
    for (map::EntityBreakdown::Map::const_iterator i = _entityBreakdown.begin(); 
		 i != _entityBreakdown.end(); 
		 i++)
	{
		GtkTreeIter iter;
		gtk_list_store_append(_listStore, &iter);
		gtk_list_store_set(_listStore, &iter, 
						   ECLASS_COL, i->first.c_str(),
						   COUNT_COL, i->second, 
						   -1);
	}
	
	// The table containing the primitive statistics
	GtkTable* table = GTK_TABLE(gtk_table_new(3, 2, FALSE));
	gtk_box_pack_start(GTK_BOX(_widget), GTK_WIDGET(table), FALSE, FALSE, 0);
	
	_brushCount = gtkutil::LeftAlignedLabel("");
	_patchCount = gtkutil::LeftAlignedLabel("");
	_entityCount = gtkutil::LeftAlignedLabel("");
	
	GtkWidget* brushLabel = gtkutil::LeftAlignedLabel("Brushes:");
	GtkWidget* patchLabel = gtkutil::LeftAlignedLabel("Patches:");
	GtkWidget* entityLabel = gtkutil::LeftAlignedLabel("Entities:");
	
	gtk_widget_set_size_request(brushLabel, 75, -1);
	gtk_widget_set_size_request(patchLabel, 75, -1);
	gtk_widget_set_size_request(entityLabel, 75, -1);
	
	gtk_table_attach(table, brushLabel, 0, 1, 0, 1,
					(GtkAttachOptions) (0),
					(GtkAttachOptions) (0), 0, 0);
	gtk_table_attach(table, patchLabel, 0, 1, 1, 2,
					(GtkAttachOptions) (0),
					(GtkAttachOptions) (0), 0, 0);
	gtk_table_attach(table, entityLabel, 0, 1, 2, 3,
					(GtkAttachOptions) (0),
					(GtkAttachOptions) (0), 0, 0);
	
	std::string bc = "<b>" + sizetToStr(GlobalCounters().getCounter(counterBrushes).get()) + "</b>";
	std::string pc = "<b>" + sizetToStr(GlobalCounters().getCounter(counterPatches).get()) + "</b>";
	std::string ec = "<b>" + sizetToStr(GlobalCounters().getCounter(counterEntities).get()) + "</b>";
	
	gtk_label_set_markup(GTK_LABEL(_brushCount), bc.c_str());
	gtk_label_set_markup(GTK_LABEL(_patchCount), pc.c_str());
	gtk_label_set_markup(GTK_LABEL(_entityCount), ec.c_str());
	
	gtk_table_attach_defaults(table, _brushCount, 1, 2, 0, 1);
	gtk_table_attach_defaults(table, _patchCount, 1, 2, 1, 2);
	gtk_table_attach_defaults(table, _entityCount, 1, 2, 2, 3);
}

} // namespace ui
