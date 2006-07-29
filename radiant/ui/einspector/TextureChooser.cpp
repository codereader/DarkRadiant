#include "TextureChooser.h"

#include "groupdialog.h"
#include "ishaders.h"
#include "igl.h"
#include "texturelib.h"

#include "gtkutil/glwidget.h"

#include <vector>
#include <string>
#include <iostream>

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
	gtk_box_pack_start(GTK_BOX(vbx), createPreview(), FALSE, FALSE, 0);
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
	g_signal_connect(G_OBJECT(_selection), "changed", G_CALLBACK(callbackSelChanged), this);

	// Pack into scrolled window
	GtkWidget* scroll = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(scroll), tree);
	return scroll;
}

// Construct the buttons

GtkWidget* TextureChooser::createButtons() {
	GtkWidget* hbx = gtk_hbox_new(FALSE, 3);
	gtk_container_set_border_width(GTK_CONTAINER(hbx), 3);

	GtkWidget* okButton = gtk_button_new_from_stock(GTK_STOCK_OK);
	GtkWidget* cancelButton = gtk_button_new_from_stock(GTK_STOCK_CANCEL);
	
	g_signal_connect(G_OBJECT(okButton), "clicked", G_CALLBACK(callbackOK), this);
	g_signal_connect(G_OBJECT(cancelButton), "clicked", G_CALLBACK(callbackCancel), this);

	gtk_box_pack_end(GTK_BOX(hbx), okButton, FALSE, FALSE, 0);
	gtk_box_pack_end(GTK_BOX(hbx), cancelButton, FALSE, FALSE, 0);
	return hbx;
}

// Construct the GL preview

GtkWidget* TextureChooser::createPreview() {
	// HBox contains the preview GL widget along with a texture attributes
	// pane.
	GtkWidget* hbx = gtk_hbox_new(FALSE, 3);

	// GtkGLExt widget
	_glWidget = glwidget_new(false);
	gtk_widget_set_size_request(_glWidget, 128, 128);
	g_signal_connect(G_OBJECT(_glWidget), "expose-event", G_CALLBACK(callbackGLDraw), this);
	GtkWidget* glFrame = gtk_frame_new(NULL);
	gtk_container_add(GTK_CONTAINER(glFrame), _glWidget);
	gtk_box_pack_start(GTK_BOX(hbx), glFrame, FALSE, FALSE, 0);
	
	// Attributes text box
	GtkWidget* textbox = gtk_text_view_new();
	GtkWidget* frame = gtk_frame_new(NULL);
	gtk_container_add(GTK_CONTAINER(frame), textbox);
	gtk_box_pack_start(GTK_BOX(hbx), frame, TRUE, TRUE, 0);

	return hbx;
}

// Return the selection as a string

const char* TextureChooser::getSelectedName() {
	// Get the selection
	GtkTreeIter iter;
	GtkTreeModel* model;
	gtk_tree_selection_get_selected(_selection, &model, &iter);

	// Get the value
	GValue val = {0, 0};
	gtk_tree_model_get_value(model, &iter, 0, &val);

	// Get the string
	return g_value_get_string(&val);	
}

/* GTK CALLBACKS */

void TextureChooser::callbackCancel(GtkWidget* w, TextureChooser* self) {
	delete self;
}

void TextureChooser::callbackOK(GtkWidget* w, TextureChooser* self) {
	gtk_entry_set_text(GTK_ENTRY(self->_entry), self->getSelectedName());
	delete self;
}

void TextureChooser::callbackGLDraw(GtkWidget* widget, GdkEventExpose* ev, TextureChooser* self) {
	if (glwidget_make_current(widget) != FALSE) {
		// Get the viewport size from the GL widget
		GtkRequisition req;
		gtk_widget_size_request(widget, &req);
		glViewport(0, 0, req.width, req.height);

		// Initialise
		glClearColor(0.3, 0.3, 0.3, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glDisable(GL_DEPTH_TEST);
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(0, req.width, 0, req.height, -100, 100);
		glEnable (GL_TEXTURE_2D);

		// Get the selected texture, and set up OpenGL to render it on
		// the quad.
		IShader* shader = GlobalShaderSystem().getShaderForName(self->getSelectedName());
		std::cout << "Got IShader: " << shader->getName() << std::endl;
		qtexture_t* tex = shader->firstLayer()->texture();
		std::cout << "got texture: " << tex->name << std::endl;
		glBindTexture (GL_TEXTURE_2D, tex->texture_number);				
		
		// Draw a quad to put the texture on
		glPolygonMode (GL_FRONT_AND_BACK, GL_FILL);
		glColor3f(1, 1, 1);
		glBegin(GL_QUADS);
		glTexCoord2i(0, 1);
		glVertex2i(0, 0);
		glTexCoord2i(1, 1);
		glVertex2i(req.height, 0);
		glTexCoord2i(1, 0);
		glVertex2i(req.height, req.height);
		glTexCoord2i(0, 0);
		glVertex2i(0, req.height);
		glEnd();

		// Update GtkGlExt buffer
		glwidget_swap_buffers(widget);
		
		// Release the IShader
		shader->DecRef();
	}
}
	
void TextureChooser::callbackSelChanged(GtkWidget* widget, TextureChooser* self) {
	gtk_widget_queue_draw(self->_glWidget);
}
	
} // namespace ui
