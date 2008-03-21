#include "LayerControl.h"

#include <gtk/gtk.h>

namespace ui {

LayerControl::LayerControl(int layerID) :
	_layerID(layerID),
	_hbox(gtk_hbox_new(FALSE, 3))
{
	GtkWidget* toggle = gtk_toggle_button_new();
	gtk_box_pack_start(GTK_BOX(_hbox), toggle, FALSE, FALSE, 0); 
}

LayerControl::operator GtkWidget*() {
	return _hbox;
}

} // namespace ui
