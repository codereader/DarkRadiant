#include "LayerControl.h"

#include "layers/LayerSystem.h"
#include <gtk/gtk.h>

namespace ui {

LayerControl::LayerControl(int layerID) :
	_layerID(layerID),
	_hbox(gtk_hbox_new(FALSE, 3))
{
	// Shortcut reference
	scene::LayerSystem& layerSystem = scene::getLayerSystem();

	GtkWidget* toggle = gtk_toggle_button_new();
	gtk_box_pack_start(GTK_BOX(_hbox), toggle, FALSE, FALSE, 0); 

	gtk_toggle_button_set_active(
		GTK_TOGGLE_BUTTON(toggle), 
		layerSystem.layerIsVisible(_layerID)
	);

	GtkWidget* label = gtk_label_new(layerSystem.getLayerName(_layerID).c_str());

	gtk_box_pack_start(GTK_BOX(_hbox), label, FALSE, FALSE, 0); 
}

GtkWidget* LayerControl::getWidget() const {
	return _hbox;
}

} // namespace ui
