#include "ShaderChooser.h"

#include "iregistry.h"
#include "ishaders.h"
#include "texturelib.h"
#include "gtkutil/window/PersistentTransientWindow.h"
#include "string/string.h"
#include "gtk/gtk.h"
#include "gdk/gdkkeysyms.h"

namespace ui {
	
	namespace {
		const std::string LABEL_TITLE = "Choose Shader";
		const std::string SHADER_PREFIXES = "textures";
		const int DEFAULT_SIZE_X = 550;
		const int DEFAULT_SIZE_Y = 500;
		const std::string RKEY_WINDOW_STATE = "user/ui/textures/shaderChooser/window";
	}

// Construct the dialog
ShaderChooser::ShaderChooser(ChooserClient* client, GtkWindow* parent, GtkWidget* targetEntry) : 
	gtkutil::BlockingTransientWindow(LABEL_TITLE, parent),
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
	
	// Set the default size and position of the window
	gtk_window_set_position(GTK_WINDOW(getWindow()), GTK_WIN_POS_CENTER_ON_PARENT);
	gtk_window_set_default_size(GTK_WINDOW(getWindow()), DEFAULT_SIZE_X, DEFAULT_SIZE_Y);
	
	// Connect the key handler to catch the ESC event
	g_signal_connect(G_OBJECT(getWindow()), "key-press-event", G_CALLBACK(onKeyPress), this);
	
	// Construct main VBox, and pack in the ShaderSelector and buttons panel
	GtkWidget* vbx = gtk_vbox_new(false, 3);
	gtk_box_pack_start(GTK_BOX(vbx), _selector, true, true, 0);
	gtk_box_pack_start(GTK_BOX(vbx), createButtons(), false, false, 0);
	gtk_container_add(GTK_CONTAINER(getWindow()), vbx);

	// Connect the window position tracker
	xml::NodeList windowStateList = GlobalRegistry().findXPath(RKEY_WINDOW_STATE);
	
	if (windowStateList.size() > 0) {
		_windowPosition.loadFromNode(windowStateList[0]);
	}
	
	_windowPosition.connect(GTK_WINDOW(getWindow()));
	_windowPosition.applyPosition();

	// Show all widgets, this will enter a main loop
	show();
}

void ShaderChooser::shutdown() {
	// Delete all the current window states from the registry
	GlobalRegistry().deleteXPath(RKEY_WINDOW_STATE);
	
	// Create a new node
	xml::Node node(GlobalRegistry().createKey(RKEY_WINDOW_STATE));
	
	// Tell the position tracker to save the information
	_windowPosition.saveToNode(node);
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
	
	// Get the shader, and its image map if possible
	IShaderPtr shader = _selector.getSelectedShader();
	// Pass the call to the static member
	ShaderSelector::displayShaderInfo(shader, listStore);
}

void ShaderChooser::revertShader() {
	// Revert the shadername to the value it had at dialog startup
	if (_targetEntry != NULL) {
		gtk_entry_set_text(GTK_ENTRY(_targetEntry), _initialShader.c_str());
		
		// Propagate the call up to the client (e.g. SurfaceInspector)
		if (_client != NULL) {
			_client->shaderSelectionChanged(
				gtk_entry_get_text(GTK_ENTRY(_targetEntry))
			);
		}
	}
}

// Static GTK CALLBACKS
void ShaderChooser::callbackCancel(GtkWidget* w, ShaderChooser* self) {
	// Revert the shadername to the value it had at dialog startup
	self->revertShader();
	
	self->destroy();
}

void ShaderChooser::callbackOK(GtkWidget* w, ShaderChooser* self) {
	if (self->_targetEntry != NULL) {
		gtk_entry_set_text(GTK_ENTRY(self->_targetEntry), 
					   	   self->_selector.getSelection().c_str());
	}

	self->destroy();
}

gboolean ShaderChooser::onKeyPress(GtkWidget* widget, GdkEventKey* event, ShaderChooser* self) {
	// Check for ESC or ENTER to close the dialog
	switch (event->keyval) {
		case GDK_Escape:
			// Revert the shadername to the value it had at dialog startup
			self->revertShader();

			// Remove this dialog
			self->destroy();
			// Don't propagate the keypress if ESC could be processed
			return TRUE;
			
		case GDK_Return:
			if (self->_targetEntry != NULL) {
				gtk_entry_set_text(GTK_ENTRY(self->_targetEntry), 
					   		   self->_selector.getSelection().c_str());
			}

			// Remove this dialog
			self->destroy();

			return TRUE;
	};

	return FALSE;
}

} // namespace ui
