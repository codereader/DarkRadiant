#include "LayerControl.h"

#include <gtk/gtk.h>
#include "iradiant.h"

#include "layers/LayerSystem.h"

namespace ui {

	namespace {
		const std::string ICON_LAYER_VISIBLE("check.png");
		const std::string ICON_LAYER_HIDDEN("empty.png");
	}

LayerControl::LayerControl(int layerID) :
	_layerID(layerID),
	_hbox(gtk_hbox_new(FALSE, 3))
{
	// Create the toggle button
	_toggle = gtk_toggle_button_new();
	gtk_box_pack_start(GTK_BOX(_hbox), _toggle, FALSE, FALSE, 0); 
	g_signal_connect(G_OBJECT(_toggle), "toggled", G_CALLBACK(onToggle), this);

	// Create the label
	_label = gtk_label_new("");
	gtk_box_pack_start(GTK_BOX(_hbox), _label, FALSE, FALSE, 0); 

	// Read the status from the Layer
	update();
}

GtkWidget* LayerControl::getWidget() const {
	return _hbox;
}

void LayerControl::update() {
	_updateActive = true;

	scene::LayerSystem& layerSystem = scene::getLayerSystem();

	bool layerIsVisible = layerSystem.layerIsVisible(_layerID);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(_toggle), layerIsVisible);
	gtk_label_set_text(GTK_LABEL(_label), layerSystem.getLayerName(_layerID).c_str());

	std::string imageName = layerIsVisible ? ICON_LAYER_VISIBLE : ICON_LAYER_HIDDEN;
	gtk_button_set_image(
		GTK_BUTTON(_toggle), 
		gtk_image_new_from_pixbuf(GlobalRadiant().getLocalPixbufWithMask(imageName))
	);

	_updateActive = false;
}

void LayerControl::onToggle(GtkToggleButton* togglebutton, LayerControl* self) {
	if (self->_updateActive) return;

	scene::getLayerSystem().setLayerVisibility(
		self->_layerID, 
		gtk_toggle_button_get_active(togglebutton) ? true : false
	);
}

} // namespace ui
