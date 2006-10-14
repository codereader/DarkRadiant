#include "MediaBrowser.h"

#include "ishaders.h"
#include "generic/callback.h"

#include <gtk/gtkvbox.h>
#include <gtk/gtktreeview.h>
#include <gtk/gtkframe.h>
#include <gtk/gtkcellrenderertext.h>
#include <gtk/gtkscrolledwindow.h>

#include <iostream>

#include <boost/algorithm/string/predicate.hpp>

namespace ui
{

// Constructor
MediaBrowser::MediaBrowser()
: _widget(gtk_vbox_new(FALSE, 0)),
  _treeStore(gtk_tree_store_new(1, G_TYPE_STRING))
{
	// Create the treeview
	GtkWidget* view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(_treeStore));
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(view), FALSE);
	g_signal_connect(G_OBJECT(view), "expose-event", G_CALLBACK(_onExpose), this);
	
	// Single text column
	GtkCellRenderer* rend = gtk_cell_renderer_text_new();
	GtkTreeViewColumn* col = gtk_tree_view_column_new();
	
	gtk_tree_view_column_pack_start(col, rend, FALSE);
	gtk_tree_view_column_set_attributes(col, rend, "text", 0, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);
	
	// Pack the treeview into a scrollwindow, frame and then into the vbox
	GtkWidget* scroll = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), 
								   GTK_POLICY_AUTOMATIC,
								   GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(scroll), view);

	GtkWidget* frame = gtk_frame_new(NULL);
	gtk_container_add(GTK_CONTAINER(frame), scroll);
	gtk_box_pack_start(GTK_BOX(_widget), frame, TRUE, TRUE, 0);
}

/* Callback functor for processing shader names */

namespace {
	
	struct ShaderNameFunctor {
		
		typedef const char* first_argument_type;
		
		// TreeStore to populate
		GtkTreeStore* _store;
		
		// Constructor
		ShaderNameFunctor(GtkTreeStore* store)
		: _store(store) {}
		
		// Functor operator
		
		void operator() (const char* name) {
			std::string rawName(name);
			
			// If the name starts with "textures/", add it to the treestore.
			if (boost::algorithm::istarts_with(rawName, "textures/")) {
				GtkTreeIter iter;
				gtk_tree_store_append(_store, &iter, NULL);
				gtk_tree_store_set(_store, &iter, 0, name, -1);
			}
		}
		
	};
	
} // namespace

/* GTK CALLBACKS */

void MediaBrowser::_onExpose(GtkWidget* widget, GdkEventExpose* ev, MediaBrowser* self) {
	// Populate the tree view if it is not already populated
	static bool _isPopulated = false;
	if (!_isPopulated) {
		ShaderNameFunctor functor(self->_treeStore);
		GlobalShaderSystem().foreachShaderName(makeCallback1(functor));

		_isPopulated = true;	
	}
}

}
