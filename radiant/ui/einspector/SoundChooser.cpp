#include "SoundChooser.h"

#include "isound.h"
#include "mainframe.h"
#include "gtkutil/TextColumn.h"
#include "gtkutil/ScrolledFrame.h"

#include <gtk/gtk.h>

namespace ui
{

namespace {
	
	// Treestore enum
	enum {
		DISPLAYNAME_COLUMN,
		SHADERNAME_COLUMN,
		N_COLUMNS
	};	
	
}

// Constructor
SoundChooser::SoundChooser()
: _widget(gtk_window_new(GTK_WINDOW_TOPLEVEL)),
  _treeStore(gtk_tree_store_new(2, G_TYPE_STRING, G_TYPE_STRING))
{
	// Set up the window
	gtk_window_set_transient_for(GTK_WINDOW(_widget), MainFrame_getWindow());
	gtk_window_set_modal(GTK_WINDOW(_widget), TRUE);
	gtk_window_set_title(GTK_WINDOW(_widget), "Choose sound");
    gtk_window_set_position(GTK_WINDOW(_widget), GTK_WIN_POS_CENTER_ON_PARENT);
    gtk_window_set_type_hint(GTK_WINDOW(_widget), GDK_WINDOW_TYPE_HINT_DIALOG);
    
	// Set the default size of the window
	GdkScreen* scr = gtk_window_get_screen(GTK_WINDOW(_widget));
	gint w = gdk_screen_get_width(scr);
	gint h = gdk_screen_get_height(scr);
	gtk_window_set_default_size(GTK_WINDOW(_widget), w / 3, h / 2);

    // Delete event
    g_signal_connect(
    	G_OBJECT(_widget), "delete-event", G_CALLBACK(_onDelete), this
    );
    
    // Main vbox
    GtkWidget* vbx = gtk_vbox_new(FALSE, 12);
    gtk_box_pack_start(GTK_BOX(vbx), createTreeView(), TRUE, TRUE, 0);
    
    gtk_container_set_border_width(GTK_CONTAINER(_widget), 12);
    gtk_container_add(GTK_CONTAINER(_widget), vbx);
}

namespace {

/**
 * Visitor class to enumerate sound shaders and add them to the tree store.
 */
class SoundShaderFinder
{
	// Tree store to populate
	GtkTreeStore* _store;
	
public:

	// Constructor
	SoundShaderFinder(GtkTreeStore* store)
	: _store(store)
	{ }
	
	// Functor operator
	void operator() (const ISoundShader& shader) {
		GtkTreeIter iter;
		gtk_tree_store_append(_store, &iter, NULL);
		gtk_tree_store_set(_store, &iter, 
						   DISPLAYNAME_COLUMN, shader.getName().c_str(),
						   -1);	
	}	
	
};


} // namespace

// Create the tree view
GtkWidget* SoundChooser::createTreeView() {
	
	// Tree view with single text column
	GtkWidget* tv = gtk_tree_view_new_with_model(GTK_TREE_MODEL(_treeStore));
	gtk_tree_view_append_column(
		GTK_TREE_VIEW(tv),
		gtkutil::TextColumn("", DISPLAYNAME_COLUMN, false)
	);

	// Populate the tree store with sound shaders
	SoundShaderFinder finder(_treeStore);
	GlobalSoundManager().forEachShader(finder);

	return gtkutil::ScrolledFrame(tv);	
}

// Show and block
std::string SoundChooser::chooseSound() {
	gtk_widget_show_all(_widget);
	gtk_main();
	return "Blah";	
}

/* GTK CALLBACKS */

// Delete dialog
void SoundChooser::_onDelete(GtkWidget* w, SoundChooser* self) {
	gtk_main_quit();	
}

}
