#include "LightTextureChooser.h"

#include "ishaders.h"
#include "iuimanager.h"
#include "igroupdialog.h"
#include "texturelib.h"
#include "iregistry.h"
#include "gtkutil/RightAlignment.h"
#include <string>

namespace ui {

namespace {
	const char* LIGHT_PREFIX_XPATH = "game/light/texture//prefix";
	
	/** greebo: Loads the prefixes from the registry and creates a 
	 * 			comma-separated list string
	 */
	inline std::string getPrefixList() {
		std::string prefixes;
		
		// Get the list of light texture prefixes from the registry
		xml::NodeList prefList = GlobalRegistry().findXPath(LIGHT_PREFIX_XPATH);
		
		// Copy the Node contents into the prefix vector	
		for (xml::NodeList::iterator i = prefList.begin();
			 i != prefList.end();
			 ++i)
		{
			prefixes += (prefixes.empty()) ? "" : ",";
			prefixes += i->getContent();
		}
		
		return prefixes;
	}
}

// Construct the dialog
LightTextureChooser::LightTextureChooser() 
:	_widget(gtk_window_new(GTK_WINDOW_TOPLEVEL)),
	_selector(this, getPrefixList(), true) // true >> render a light texture
{
	GtkWidget* gd = GlobalGroupDialog().getDialogWindow();

	gtk_window_set_transient_for(GTK_WINDOW(_widget), GTK_WINDOW(gd));
    gtk_window_set_modal(GTK_WINDOW(_widget), TRUE);
    gtk_window_set_position(GTK_WINDOW(_widget), GTK_WIN_POS_CENTER_ON_PARENT);
	gtk_window_set_title(GTK_WINDOW(_widget), "Choose texture");

	// Set the default size of the window
	gint w, h;
	gtk_window_get_size(GTK_WINDOW(gd), &w, &h);
	gtk_window_set_default_size(GTK_WINDOW(_widget), w, h);
	
	// Construct main VBox, and pack in ShaderSelector and buttons panel
	GtkWidget* vbx = gtk_vbox_new(FALSE, 6);
	gtk_box_pack_start(GTK_BOX(vbx), _selector, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbx), createButtons(), FALSE, FALSE, 0);
	gtk_container_add(GTK_CONTAINER(_widget), vbx);
}

// Construct the buttons
GtkWidget* LightTextureChooser::createButtons() {
	GtkWidget* hbx = gtk_hbox_new(TRUE, 6);
	gtk_container_set_border_width(GTK_CONTAINER(hbx), 3);

	GtkWidget* okButton = gtk_button_new_from_stock(GTK_STOCK_OK);
	GtkWidget* cancelButton = gtk_button_new_from_stock(GTK_STOCK_CANCEL);
	
	g_signal_connect(G_OBJECT(okButton), "clicked", G_CALLBACK(callbackOK), this);
	g_signal_connect(G_OBJECT(cancelButton), "clicked", G_CALLBACK(callbackCancel), this);

	gtk_box_pack_end(GTK_BOX(hbx), okButton, TRUE, TRUE, 0);
	gtk_box_pack_end(GTK_BOX(hbx), cancelButton, TRUE, TRUE, 0);
	return gtkutil::RightAlignment(hbx);
}

// Block for a selection
std::string LightTextureChooser::chooseTexture() {

	// Show all widgets and enter a recursive main loop
	gtk_widget_show_all(_widget);
	gtk_main();
	
	// Return the last selection
	return _selectedTexture;
	
}

void LightTextureChooser::shaderSelectionChanged(
	const std::string& shaderName, 
	GtkListStore* listStore)
{
	// Get the shader, and its image map if possible
	IShaderPtr shader = _selector.getSelectedShader();
	// Pass the call to the static member light shader info
	ShaderSelector::displayLightShaderInfo(shader, listStore);
}

/* GTK CALLBACKS */

void LightTextureChooser::callbackCancel(GtkWidget* w, 
										 LightTextureChooser* self) 
{
	self->_selectedTexture = "";
	gtk_widget_destroy(self->_widget);
	gtk_main_quit();	
}

void LightTextureChooser::callbackOK(GtkWidget* w, LightTextureChooser* self) {
	self->_selectedTexture = self->_selector.getSelection();
	gtk_widget_destroy(self->_widget);
	gtk_main_quit();
}

} // namespace ui
