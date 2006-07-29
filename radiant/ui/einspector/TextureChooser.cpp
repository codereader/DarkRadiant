#include "TextureChooser.h"

#include "groupdialog.h"
#include "ishaders.h"

#include <vector>
#include <string>

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/split.hpp>

namespace ui
{

// Construct the dialog

TextureChooser::TextureChooser(GtkWidget* entry, const std::string& prefixes)
: _entry(entry),
  _widget(gtk_window_new(GTK_WINDOW_TOPLEVEL)),
  _prefixes(prefixes)
{
	GtkWindow* gd = GroupDialog_getWindow();

	gtk_window_set_transient_for(GTK_WINDOW(_widget), gd);
    gtk_window_set_modal(GTK_WINDOW(_widget), TRUE);
    gtk_window_set_position(GTK_WINDOW(_widget), GTK_WIN_POS_CENTER_ON_PARENT);
	gtk_window_set_title(GTK_WINDOW(_widget), "Choose texture");

	// Set the default size of the window to slightly smaller than
	// the parent GroupDialog.
	
	gint w, h;
	gtk_window_get_size(gd, &w, &h);
	gtk_window_set_default_size(GTK_WINDOW(_widget), gint(w / 1.1), gint(h / 1.1));
	
	// Construct main VBox, and pack in TreeView and buttons panel
	
	GtkWidget* vbx = gtk_vbox_new(FALSE, 3);
	gtk_box_pack_start(GTK_BOX(vbx), createTreeView(), TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbx), createButtons(), FALSE, FALSE, 0);
	gtk_container_add(GTK_CONTAINER(_widget), vbx);
	
	// Show all widgets
	gtk_widget_show_all(_widget);
}

// Construct the tree view using a local functor object to retrieve the
// textures from GlobalShaderSystem.

namespace {

	struct ShaderNameFunctor {
	
		typedef const char* first_argument_type;

		// Interesting texture prefixes
		std::vector<std::string> _prefixes;

		// Tree Store to add to
		GtkTreeStore* _store;

		// Constructor
		ShaderNameFunctor(const std::string& pref, GtkTreeStore* store)
		: _store(store) 
		{
			boost::algorithm::split(_prefixes,
									pref,
									boost::algorithm::is_any_of(","));
		
		}
	
		// Functor operator
		void operator() (const char* shaderName) {
			std::string name(shaderName);
			for (std::vector<std::string>::iterator i = _prefixes.begin();
				 i != _prefixes.end();
				 i++)
			{
				if (boost::algorithm::istarts_with(name, *i)) {
					GtkTreeIter iter;
					gtk_tree_store_append(_store, &iter, NULL);
					gtk_tree_store_set(_store, &iter, 0, shaderName, -1);
					break; // don't consider any further prefixes
				}
			}
		}
	};
}

GtkWidget* TextureChooser::createTreeView() {

	// Tree model
	GtkTreeStore* store = gtk_tree_store_new(1, G_TYPE_STRING);	
	
	ShaderNameFunctor func(_prefixes, store);
	GlobalShaderSystem().foreachShaderName(makeCallback1(func));
	
	// Tree view
	GtkWidget* tree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
	g_object_unref(store); // tree view owns the reference now
	
	GtkCellRenderer* rend = gtk_cell_renderer_text_new();
	GtkTreeViewColumn* col = 
		gtk_tree_view_column_new_with_attributes("Texture",
												 rend,
												 "text", 0,
												 NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), col);				
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(tree), FALSE);
	_selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree));

	// Pack into scrolled window
	GtkWidget* scroll = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(scroll), tree);
	return scroll;
}

// Construct the buttons

GtkWidget* TextureChooser::createButtons() {
	GtkWidget* hbx = gtk_hbox_new(FALSE, 3);

	GtkWidget* okButton = gtk_button_new_from_stock(GTK_STOCK_OK);
	GtkWidget* cancelButton = gtk_button_new_from_stock(GTK_STOCK_CANCEL);
	
	g_signal_connect(G_OBJECT(okButton), "clicked", G_CALLBACK(callbackOK), this);
	g_signal_connect(G_OBJECT(cancelButton), "clicked", G_CALLBACK(callbackCancel), this);

	gtk_box_pack_end(GTK_BOX(hbx), okButton, FALSE, FALSE, 0);
	gtk_box_pack_end(GTK_BOX(hbx), cancelButton, FALSE, FALSE, 0);
	return hbx;
}

/* GTK CALLBACKS */

void TextureChooser::callbackCancel(GtkWidget* w, TextureChooser* self) {
	delete self;
}

void TextureChooser::callbackOK(GtkWidget* w, TextureChooser* self) {

	// Get the selection
	GtkTreeIter iter;
	GtkTreeModel* model;
	gtk_tree_selection_get_selected(self->_selection, &model, &iter);

	// Get the value
	GValue val = {0, 0};
	gtk_tree_model_get_value(model, &iter, 0, &val);

	gtk_entry_set_text(GTK_ENTRY(self->_entry), g_value_get_string(&val));
	delete self;
}
	
} // namespace ui
