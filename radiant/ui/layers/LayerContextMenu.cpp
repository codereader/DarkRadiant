#include "LayerContextMenu.h"

#include <gtk/gtk.h>

#include "iradiant.h"
#include "gtkutil/IconTextMenuItem.h"
#include "gtkutil/pointer.h"
#include "layers/LayerSystem.h"

namespace ui {

	namespace {
		const std::string LAYER_ICON("layer.png");
	}

LayerContextMenu::LayerContextMenu(OnSelectionFunc& onSelection) :
	_onSelection(onSelection),
	_menu(gtk_menu_new())
{
	// Populate the menu shell with all layers
	scene::getLayerSystem().foreachLayer(*this);
	
	// Show all the items
	gtk_widget_show_all(_menu);
}

void LayerContextMenu::visit(int layerID, std::string layerName) {
	// Create a new menuitem
	GtkWidget* menuItem = gtkutil::IconTextMenuItem(
		GlobalRadiant().getLocalPixbuf(LAYER_ICON), layerName);

	// Connect the "onclick" signal
	g_signal_connect(G_OBJECT(menuItem), "activate", G_CALLBACK(onClick), this);

	// And store the layer id in the GTK object
	g_object_set_data(G_OBJECT(menuItem), "layerID", gint_to_pointer(layerID));

	// Add it to the parent menu
	gtk_menu_shell_append(GTK_MENU_SHELL(_menu), menuItem);
}

LayerContextMenu::operator GtkWidget*() {
	return _menu;
}

void LayerContextMenu::onClick(GtkMenuItem* menuitem, LayerContextMenu* self) {
	// Retrieve the layer id from the object
	int layerID = gpointer_to_int(g_object_get_data(G_OBJECT(menuitem), "layerID")); 

	// Now call the function
	self->_onSelection(layerID);
}

} // namespace ui
