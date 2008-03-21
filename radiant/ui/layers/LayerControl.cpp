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

	// Create the toggle button
	GtkWidget* toggle = gtk_toggle_button_new();
	gtk_box_pack_start(GTK_BOX(_hbox), toggle, FALSE, FALSE, 0); 

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(toggle), layerSystem.layerIsVisible(_layerID));
	g_signal_connect(G_OBJECT(toggle), "toggled", G_CALLBACK(onToggle), this);

	// Create the label
	GtkWidget* label = gtk_label_new(layerSystem.getLayerName(_layerID).c_str());
	gtk_box_pack_start(GTK_BOX(_hbox), label, FALSE, FALSE, 0); 
}

GtkWidget* LayerControl::getWidget() const {
	return _hbox;
}

void LayerControl::onToggle(GtkToggleButton* togglebutton, LayerControl* self) {
	scene::getLayerSystem().setLayerVisibility(
		self->_layerID, 
		gtk_toggle_button_get_active(togglebutton) ? true : false
	);
}

} // namespace ui
