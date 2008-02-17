#include "TexturePreviewCombo.h"

#include "gtkutil/GLWidgetSentry.h"
#include "gtkutil/ScrolledFrame.h"
#include "gtkutil/StockIconMenuItem.h"
#include "gtkutil/TextColumn.h"

#include "ishaders.h"
#include "texturelib.h"

#include "ShaderSelector.h"

#include <gtk/gtk.h>
#include <GL/glew.h>
#include <boost/bind.hpp>

namespace ui
{

// Constructor. Create GTK widgets.

TexturePreviewCombo::TexturePreviewCombo()
: _widget(gtk_hbox_new(FALSE, 0)),
  _glWidget(false),
  _texName(""),
  _infoStore(gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_STRING)),
  _infoView(gtk_tree_view_new_with_model(GTK_TREE_MODEL(_infoStore))),
  _contextMenu(_infoView)
{
	// Set up the GL preview widget
	GtkWidget* glWidget = _glWidget; // cast to GtkWidget
	gtk_widget_set_size_request(glWidget, 128, 128);
	g_signal_connect(G_OBJECT(glWidget), "expose-event", G_CALLBACK(_onExpose), this);
	GtkWidget* glFrame = gtk_frame_new(NULL);
	gtk_container_add(GTK_CONTAINER(glFrame), glWidget);
	gtk_box_pack_start(GTK_BOX(_widget), glFrame, FALSE, FALSE, 0);
	
	// Set up the info table
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(_infoView), FALSE);
	
	gtk_tree_view_append_column(GTK_TREE_VIEW(_infoView),
								gtkutil::TextColumn("Attribute", 0));
	
	gtk_tree_view_append_column(GTK_TREE_VIEW(_infoView),
								gtkutil::TextColumn("Value", 1));

	// Pack into main widget
	gtk_box_pack_start(
		GTK_BOX(_widget), gtkutil::ScrolledFrame(_infoView), TRUE, TRUE, 0
	);
	
	// Construct the context menu
	_contextMenu.addItem(
		gtkutil::StockIconMenuItem(GTK_STOCK_COPY, "Copy shader name"),
		boost::bind(&TexturePreviewCombo::_onCopyTexName, this)
	);	
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

	// Other properties require a valid shader name	
	if (_texName.empty())
		return;

	IShaderPtr shader = GlobalShaderSystem().getShaderForName(_texName);
	ShaderSelector::displayShaderInfo(shader, _infoStore);
}

// Popup menu callbacks

void TexturePreviewCombo::_onCopyTexName() {
	// Store texture on the clipboard
	GtkClipboard* clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
	gtk_clipboard_set_text(clipboard, _texName.c_str(), static_cast<int>(_texName.size()));
}

// GTK CALLBACKS

void TexturePreviewCombo::_onExpose(GtkWidget* widget, GdkEventExpose* ev, TexturePreviewCombo* self) {
	// Grab the GLWidget with sentry
	gtkutil::GLWidgetSentry sentry(widget);
	
	// Get the viewport size from the GL widget
	GtkRequisition req;
	gtk_widget_size_request(widget, &req);
	glViewport(0, 0, req.width, req.height);

	// Initialise
	glClearColor(0.3f, 0.3f, 0.3f, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glDisable(GL_DEPTH_TEST);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, req.width, 0, req.height, -100, 100);
	glEnable (GL_TEXTURE_2D);

	// If no texture is loaded, leave window blank
	if (self->_texName == "")
		return;

	// Get a reference to the selected shader
	IShaderPtr shader = GlobalShaderSystem().getShaderForName(self->_texName);

	// This is an "ordinary" texture, take the editor image
	TexturePtr tex = shader->getTexture();
	if (tex != NULL) {
		glBindTexture (GL_TEXTURE_2D, tex->texture_number);
		
		// Calculate the correct aspect ratio for preview
		float aspect = float(tex->width) / float(tex->height);
		float hfWidth, hfHeight;
		if (aspect > 1.0) {
			hfWidth = 0.5*req.width;
			hfHeight = 0.5*req.height / aspect;
		}
		else {
			hfHeight = 0.5*req.width;
			hfWidth = 0.5*req.height * aspect;
		}
		
		// Draw a quad to put the texture on
		glPolygonMode (GL_FRONT_AND_BACK, GL_FILL);
		glColor3f(1, 1, 1);
		glBegin(GL_QUADS);
		glTexCoord2i(0, 1); 
		glVertex2f(0.5*req.width - hfWidth, 0.5*req.height - hfHeight);
		glTexCoord2i(1, 1); 
		glVertex2f(0.5*req.width + hfWidth, 0.5*req.height - hfHeight);
		glTexCoord2i(1, 0); 
		glVertex2f(0.5*req.width + hfWidth, 0.5*req.height + hfHeight);
		glTexCoord2i(0, 0);	
		glVertex2f(0.5*req.width - hfWidth, 0.5*req.height + hfHeight);
		glEnd();
	}
}

}
