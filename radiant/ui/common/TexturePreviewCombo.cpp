#include "TexturePreviewCombo.h"

#include "gtkutil/glwidget.h"
#include "gtkutil/GLWidgetSentry.h"

#include "ishaders.h"
#include "texturelib.h"

#include <gtk/gtkhbox.h>
#include <gtk/gtkframe.h>
#include <gtk/gtkcontainer.h>
#include <gtk/gtkliststore.h>
#include <gtk/gtktreeview.h>
#include <gtk/gtkcellrenderertext.h>
#include <gtk/gtkscrolledwindow.h>

#include <GL/glew.h>

namespace ui
{

// Constructor. Create GTK widgets.

TexturePreviewCombo::TexturePreviewCombo()
: _widget(gtk_hbox_new(FALSE, 0)),
  _glWidget(glwidget_new(false)),
  _texName(""),
  _infoStore(gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_STRING))
{
	// Set up the GL preview widget
	gtk_widget_set_size_request(_glWidget, 128, 128);
	g_signal_connect(G_OBJECT(_glWidget), "expose-event", G_CALLBACK(_onExpose), this);
	GtkWidget* glFrame = gtk_frame_new(NULL);
	gtk_container_add(GTK_CONTAINER(glFrame), _glWidget);
	gtk_box_pack_start(GTK_BOX(_widget), glFrame, FALSE, FALSE, 0);
	
	// Set up the info table
	GtkWidget* view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(_infoStore));
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(view), FALSE);
	
	GtkCellRenderer* rend;
	GtkTreeViewColumn* col;
	
	rend = gtk_cell_renderer_text_new();
	col = gtk_tree_view_column_new_with_attributes("Attribute",
												   rend,
												   "text", 0,
												   NULL);
	g_object_set(G_OBJECT(rend), "weight", 700, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);
	
	rend = gtk_cell_renderer_text_new();
	col = gtk_tree_view_column_new_with_attributes("Value",
												   rend,
												   "text", 1,
												   NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);

	GtkWidget* scroll = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),
								   GTK_POLICY_AUTOMATIC,
								   GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(scroll), view);

	GtkWidget* attFrame = gtk_frame_new(NULL);
	gtk_container_add(GTK_CONTAINER(attFrame), scroll);

	gtk_box_pack_start(GTK_BOX(_widget), attFrame, TRUE, TRUE, 0);
	
}

// Update the selected texture

void TexturePreviewCombo::setTexture(const std::string& tex) {
	_texName = tex;
	refreshInfoTable();
	gtk_widget_queue_draw(_glWidget);
}

// Refresh the info table

void TexturePreviewCombo::refreshInfoTable() {
	// Prepare the list
	gtk_list_store_clear(_infoStore);
	GtkTreeIter iter;

	// Texture name
	gtk_list_store_append(_infoStore, &iter);
	gtk_list_store_set(_infoStore, &iter, 
					   0, "Shader",
					   1, _texName.c_str(),
					   -1);

	// Other properties require a valid shader name	
	if (_texName.empty())
		return;
		
	IShaderPtr shader = GlobalShaderSystem().getShaderForName(_texName);

	// Containing MTR	
	gtk_list_store_append(_infoStore, &iter);
	gtk_list_store_set(_infoStore, &iter, 
					   0, "Defined in",
					   1, shader->getShaderFileName(),
					   -1);
					   
	// Release the shader
	shader->DecRef();

}


// GTK CALLBACKS

void TexturePreviewCombo::_onExpose(GtkWidget* widget, GdkEventExpose* ev, TexturePreviewCombo* self) {
	// Grab the GLWidget with sentry
	gtkutil::GLWidgetSentry sentry(widget);
	
	// Initialise viewport
	glClearColor(0.3, 0.3, 0.3, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_TEXTURE_2D);

	// If no texture is loaded, leave window blank
	if (self->_texName == "")
		return;
	
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, 1, 0, 1, -1, 1);
	
	// Get a reference to the selected shader
	IShaderPtr shader = GlobalShaderSystem().getShaderForName(self->_texName);
	
	// Bind the texture from the shader
	TexturePtr tex = shader->getTexture();
	glBindTexture(GL_TEXTURE_2D, tex->texture_number);

	// Calculate the correct aspect ratio for preview
	float aspect = float(tex->width) / float(tex->height);
	float hfWidth, hfHeight;
	if (aspect > 1.0) {
		hfWidth = 0.5;
		hfHeight = 0.5 / aspect;
	}
	else {
		hfHeight = 0.5;
		hfWidth = 0.5 * aspect;
	}
	
	// Draw a polygon
	glPolygonMode (GL_FRONT_AND_BACK, GL_FILL);
	glColor3f(1, 1, 1);
	glBegin(GL_QUADS);
		glTexCoord2i(0, 1); glVertex2f(0.5 - hfWidth, 0.5 - hfHeight);
		glTexCoord2i(1, 1); glVertex2f(0.5 + hfWidth, 0.5 - hfHeight);
		glTexCoord2i(1, 0); glVertex2f(0.5 + hfWidth, 0.5 + hfHeight);
		glTexCoord2i(0, 0);	glVertex2f(0.5 - hfWidth, 0.5 + hfHeight);
	glEnd();
	
	// Release the shader
	shader->DecRef();
}

}
