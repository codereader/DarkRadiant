#include "ShaderChooser.h"

#include "ishaders.h"
#include "texturelib.h"
#include "gtkutil/TransientWindow.h"
#include "string/string.h"
#include "gtk/gtk.h"

namespace ui {
	
	namespace {
		const std::string LABEL_TITLE = "Choose Shader";
		const std::string SHADER_PREFIXES = "textures";
		const int DEFAULT_SIZE_X = 500;
		const int DEFAULT_SIZE_Y = 500;
	}

// Construct the dialog
ShaderChooser::ShaderChooser(Client* client, GtkWidget* parent, GtkWidget* targetEntry) :
	_client(client),
	_parent(parent), 
	_targetEntry(targetEntry),
	_selector(this, SHADER_PREFIXES)
{
	if (_targetEntry != NULL) {
		_initialShader = gtk_entry_get_text(GTK_ENTRY(_targetEntry));
		// Set the cursor of the tree view to the currently selected shader
		_selector.setSelection(_initialShader);
	}
	
	_dialog = gtkutil::TransientWindow(LABEL_TITLE, GTK_WINDOW(_parent), false);
	gtk_window_set_modal(GTK_WINDOW(_dialog), true);
    gtk_window_set_position(GTK_WINDOW(_dialog), GTK_WIN_POS_CENTER_ON_PARENT);
	
	// Set the default size of the window
	gtk_window_set_default_size(GTK_WINDOW(_dialog), DEFAULT_SIZE_X, DEFAULT_SIZE_Y);
	
	// Construct main VBox, and pack in the ShaderSelector and buttons panel
	GtkWidget* vbx = gtk_vbox_new(false, 3);
	gtk_box_pack_start(GTK_BOX(vbx), _selector, true, true, 0);
	gtk_box_pack_start(GTK_BOX(vbx), createButtons(), false, false, 0);
	gtk_container_add(GTK_CONTAINER(_dialog), vbx);

	// Show all widgets
	gtk_widget_show_all(_dialog);
}

ShaderChooser::~ShaderChooser() {
	gtk_widget_destroy(_dialog);
}

// Construct the buttons
GtkWidget* ShaderChooser::createButtons() {
	GtkWidget* hbx = gtk_hbox_new(false, 3);
	gtk_container_set_border_width(GTK_CONTAINER(hbx), 3);

	GtkWidget* okButton = gtk_button_new_from_stock(GTK_STOCK_OK);
	GtkWidget* cancelButton = gtk_button_new_from_stock(GTK_STOCK_CANCEL);
	
	g_signal_connect(G_OBJECT(okButton), "clicked", G_CALLBACK(callbackOK), this);
	g_signal_connect(G_OBJECT(cancelButton), "clicked", G_CALLBACK(callbackCancel), this);

	gtk_box_pack_end(GTK_BOX(hbx), okButton, false, false, 0);
	gtk_box_pack_end(GTK_BOX(hbx), cancelButton, false, false, 0);
	return hbx;
}

void ShaderChooser::shaderSelectionChanged(const std::string& shaderName, GtkListStore* listStore) {
	if (_targetEntry != NULL) {
		gtk_entry_set_text(GTK_ENTRY(_targetEntry), 
					   	   _selector.getSelection().c_str());
	}
	
	// Propagate the call up to the client (e.g. SurfaceInspector)
	if (_client != NULL) {
		_client->shaderSelectionChanged(shaderName);
	}
	
	// Update the infostore in the ShaderSelector
	GtkTreeIter iter;
	
	// Get the shader, and its image map if possible
	IShaderPtr shader = _selector.getSelectedShader();
	gtk_list_store_append(listStore, &iter);
	gtk_list_store_set(listStore, &iter, 
					   0, "<b>Shader</b>",
					   1, shader->getName(),
					   -1);
	
	TexturePtr tex = shader->getDiffuse();
	std::string dimStr = intToStr(tex->width) + " x " + intToStr(tex->height);
	
	gtk_list_store_append(listStore, &iter);
	gtk_list_store_set(listStore, &iter, 
					   0, "<b>Dimensions</b>",
					   1, dimStr.c_str(),
					   -1);
					   
	// Containing MTR	
	gtk_list_store_append(listStore, &iter);
	gtk_list_store_set(listStore, &iter, 
					   0, "Defined in",
					   1, shader->getShaderFileName(),
					   -1);
}

// Static GTK CALLBACKS
void ShaderChooser::callbackCancel(GtkWidget* w, ShaderChooser* self) {
	// Revert the shadername to the value it had at dialog startup
	
	if (self->_targetEntry != NULL) {
		gtk_entry_set_text(GTK_ENTRY(self->_targetEntry), 
					   	   self->_initialShader.c_str());
					   	   
		// Propagate the call up to the client (e.g. SurfaceInspector)
		if (self->_client != NULL) {
			self->_client->shaderSelectionChanged(
				gtk_entry_get_text(GTK_ENTRY(self->_targetEntry))
			);
		}
	}
	
	delete self;
}

void ShaderChooser::callbackOK(GtkWidget* w, ShaderChooser* self) {
	if (self->_targetEntry != NULL) {
		gtk_entry_set_text(GTK_ENTRY(self->_targetEntry), 
					   	   self->_selector.getSelection().c_str());
	}
	delete self;
}

} // namespace ui
