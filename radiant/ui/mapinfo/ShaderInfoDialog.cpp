#include "ShaderInfoDialog.h"

#include <gtk/gtk.h>
#include "ieventmanager.h"
#include "iradiant.h"
#include "icounter.h"

#include "string/string.h"
#include "gtkutil/ScrolledFrame.h"
#include "gtkutil/TextColumn.h"
#include "gtkutil/LeftAlignedLabel.h"

namespace ui {

	namespace {
		const int SHADERINFO_DEFAULT_SIZE_X = 600;
	    const int SHADERINFO_DEFAULT_SIZE_Y = 550;
	   	const std::string SHADERINFO_WINDOW_TITLE = "Shader Usage Info";
	   	
	   	enum {
	   		SHADER_COL,
	   		FACE_COUNT_COL,
			PATCH_COUNT_COL,
	   		NUM_COLS
	   	};
	}

ShaderInfoDialog::ShaderInfoDialog() :
	BlockingTransientWindow(SHADERINFO_WINDOW_TITLE, GlobalRadiant().getMainWindow())
{
	gtk_window_set_default_size(GTK_WINDOW(getWindow()), SHADERINFO_DEFAULT_SIZE_X, SHADERINFO_DEFAULT_SIZE_Y);
	gtk_container_set_border_width(GTK_CONTAINER(getWindow()), 12);
	gtk_window_set_type_hint(GTK_WINDOW(getWindow()), GDK_WINDOW_TYPE_HINT_DIALOG);
	
	// Create all the widgets
	populateWindow();
	
	// Propagate shortcuts to the main window
	GlobalEventManager().connectDialogWindow(GTK_WINDOW(getWindow()));
	
	// Show the window and its children, enter the main loop
	show();
}

void ShaderInfoDialog::shutdown() {
	// Stop propagating shortcuts to the main window
	GlobalEventManager().disconnectDialogWindow(GTK_WINDOW(getWindow()));
}

void ShaderInfoDialog::populateWindow() {
	GtkWidget* dialogVBox = gtk_vbox_new(FALSE, 6);
	gtk_container_add(GTK_CONTAINER(getWindow()), dialogVBox);
	
	// Create the list store that contains the shader => count, count map 
	_listStore = gtk_list_store_new(3, G_TYPE_STRING, G_TYPE_INT, G_TYPE_INT);

	// Create the treeview and pack two columns into it
	_treeView = gtk_tree_view_new_with_model(GTK_TREE_MODEL(_listStore));
	gtk_tree_view_set_headers_clickable(GTK_TREE_VIEW(_treeView), TRUE);

	GtkTreeViewColumn* shaderCol = gtkutil::TextColumn("Shader", SHADER_COL);
	gtk_tree_view_column_set_sort_column_id(shaderCol, SHADER_COL);
	
	GtkTreeViewColumn* faceCountCol = gtkutil::TextColumn("Faces", FACE_COUNT_COL);
	gtk_tree_view_column_set_sort_column_id(faceCountCol, FACE_COUNT_COL);

	GtkTreeViewColumn* patchCountCol = gtkutil::TextColumn("Patches", PATCH_COUNT_COL);
	gtk_tree_view_column_set_sort_column_id(patchCountCol, PATCH_COUNT_COL);

	gtk_tree_view_append_column(GTK_TREE_VIEW(_treeView), shaderCol);
	gtk_tree_view_append_column(GTK_TREE_VIEW(_treeView), faceCountCol);
	gtk_tree_view_append_column(GTK_TREE_VIEW(_treeView), patchCountCol);
	
    gtk_box_pack_start(GTK_BOX(dialogVBox), gtkutil::ScrolledFrame(_treeView), TRUE, TRUE, 0);
    
    // Populate the liststore with the entity count information
    for (map::ShaderBreakdown::Map::const_iterator i = _shaderBreakdown.begin(); 
		 i != _shaderBreakdown.end(); 
		 ++i)
	{
		GtkTreeIter iter;
		gtk_list_store_append(_listStore, &iter);
		gtk_list_store_set(_listStore, &iter, 
						   SHADER_COL, i->first.c_str(),
						   FACE_COUNT_COL, i->second.faceCount, 
						   PATCH_COUNT_COL, i->second.patchCount, 
						   -1);
	}
	
	// Finally, add the buttons
	gtk_box_pack_start(GTK_BOX(dialogVBox), createButtons(), FALSE, FALSE, 0);
}

GtkWidget* ShaderInfoDialog::createButtons() {
	GtkWidget* hbox = gtk_hbox_new(FALSE, 6);

	GtkWidget* closeButton = gtk_button_new_from_stock(GTK_STOCK_CLOSE);
	g_signal_connect(G_OBJECT(closeButton), "clicked", G_CALLBACK(onClose), this);

	gtk_box_pack_end(GTK_BOX(hbox), closeButton, FALSE, FALSE, 0);
	
	return hbox;
}

void ShaderInfoDialog::onClose(GtkWidget* widget, ShaderInfoDialog* self) {
	// Call the destroy method which exits the main loop
	self->shutdown();
	self->destroy();
}

void ShaderInfoDialog::showDialog() {
	ShaderInfoDialog dialog; // blocks on instantiation
}

} // namespace ui
