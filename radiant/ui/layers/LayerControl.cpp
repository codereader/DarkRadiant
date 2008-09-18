#include "LayerControl.h"

#include <gtk/gtk.h>
#include "iradiant.h"
#include "ieventmanager.h"
#include "gtkutil/LeftAlignedLabel.h"
#include "gtkutil/messagebox.h"

#include "layers/LayerSystem.h"
#include "LayerControlDialog.h"

namespace ui {

	namespace {
		const std::string ICON_LAYER_VISIBLE("check.png");
		const std::string ICON_LAYER_HIDDEN("empty.png");
		const std::string ICON_DELETE("delete.png");
	}

LayerControl::LayerControl(int layerID) :
	_layerID(layerID),
	_tooltips(gtk_tooltips_new())
{
	// Create the toggle button
	_toggle = gtk_toggle_button_new();
	g_signal_connect(G_OBJECT(_toggle), "toggled", G_CALLBACK(onToggle), this);

	// Create the label
	_labelButton = gtk_button_new_with_label("");
	g_signal_connect(G_OBJECT(_labelButton), "clicked", G_CALLBACK(onLayerSelect), this);
	
	_deleteButton = gtk_button_new();
	gtk_button_set_image(
		GTK_BUTTON(_deleteButton), 
		gtk_image_new_from_pixbuf(GlobalRadiant().getLocalPixbufWithMask(ICON_DELETE))
	);
	g_signal_connect(G_OBJECT(_deleteButton), "clicked", G_CALLBACK(onDelete), this);

	// Enable the tooltips group for the help mouseover texts
	gtk_tooltips_enable(_tooltips);
	gtk_tooltips_set_tip(_tooltips, _labelButton, "Click to select all in layer, hold SHIFT to deselect", "");
	gtk_tooltips_set_tip(_tooltips, _deleteButton, "Delete this layer", "");
	gtk_tooltips_set_tip(_tooltips, _toggle, "Toggle layer visibility", "");

	// Read the status from the Layer
	update();
}

GtkWidget* LayerControl::getLabelButton() const {
	return _labelButton;
}

GtkWidget* LayerControl::getButtons() const {
	return _deleteButton;
}

GtkWidget* LayerControl::getToggle() const {
	return _toggle;
}

void LayerControl::update() {
	_updateActive = true;

	scene::LayerSystem& layerSystem = scene::getLayerSystem();

	bool layerIsVisible = layerSystem.layerIsVisible(_layerID);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(_toggle), layerIsVisible);

	gtk_button_set_label(GTK_BUTTON(_labelButton), layerSystem.getLayerName(_layerID).c_str());

	std::string imageName = layerIsVisible ? ICON_LAYER_VISIBLE : ICON_LAYER_HIDDEN;
	gtk_button_set_image(
		GTK_BUTTON(_toggle), 
		gtk_image_new_from_pixbuf(GlobalRadiant().getLocalPixbufWithMask(imageName))
	);

	// Don't allow deletion of layer 0
	gtk_widget_set_sensitive(_deleteButton, _layerID != 0);

	_updateActive = false;
}

void LayerControl::onToggle(GtkToggleButton* togglebutton, LayerControl* self) {
	if (self->_updateActive) return;

	scene::getLayerSystem().setLayerVisibility(
		self->_layerID, 
		gtk_toggle_button_get_active(togglebutton) ? true : false
	);
}

void LayerControl::onDelete(GtkWidget* button, LayerControl* self) {
	// Ask the about the deletion
	std::string msg = "Do you really want to delete this layer?\n<b>" + 
		scene::getLayerSystem().getLayerName(self->_layerID) + "</b>";

	EMessageBoxReturn returnValue = gtk_MessageBox(
		GTK_WIDGET(GlobalRadiant().getMainWindow()), 
		msg.c_str(), "Delete Layer", eMB_YESNO, eMB_ICONQUESTION
	);
	
	if (returnValue == eIDYES) {
		scene::getLayerSystem().deleteLayer(
			scene::getLayerSystem().getLayerName(self->_layerID)
		);
		LayerControlDialog::Instance().refresh();
	}
}

void LayerControl::onLayerSelect(GtkWidget* button, LayerControl* self) {
	// By default, we SELECT the layer
	bool selected = true;

	// The user can choose to DESELECT the layer when holding down shift
	if ((GlobalEventManager().getModifierState() & GDK_SHIFT_MASK) != 0) {
		selected = false;
	}

	// Set the entire layer to selected
	GlobalLayerSystem().setSelected(self->_layerID, selected);
}

} // namespace ui
