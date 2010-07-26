#include "LayerControl.h"

#include "i18n.h"
#include <gtk/gtk.h>
#include "iradiant.h"
#include "ieventmanager.h"
#include "idialogmanager.h"
#include "gtkutil/dialog.h"
#include "gtkutil/LeftAlignedLabel.h"
#include "gtkutil/EntryAbortedException.h"

#include "layers/LayerSystem.h"
#include "LayerControlDialog.h"

namespace ui {

	namespace {
		const std::string ICON_LAYER_VISIBLE("check.png");
		const std::string ICON_LAYER_HIDDEN("empty.png");
		const std::string ICON_DELETE("delete.png");

		enum {
			WIDGET_TOGGLE,
			WIDGET_LABEL_BUTTON,
			WIDGET_RENAME_BUTTON,
			WIDGET_DELETE_BUTTON,
			WIDGET_BUTTON_HBOX,
		};
	}

LayerControl::LayerControl(int layerID) :
	_layerID(layerID),
	_tooltips(gtk_tooltips_new())
{
	// Create the toggle button
	_widgets[WIDGET_TOGGLE] = gtk_toggle_button_new();
	g_signal_connect(G_OBJECT(_widgets[WIDGET_TOGGLE]), "toggled", G_CALLBACK(onToggle), this);

	// Create the label
	_widgets[WIDGET_LABEL_BUTTON] = gtk_button_new_with_label("");
	g_signal_connect(G_OBJECT(_widgets[WIDGET_LABEL_BUTTON]), "clicked", G_CALLBACK(onLayerSelect), this);
	
	_widgets[WIDGET_DELETE_BUTTON] = gtk_button_new();
	gtk_button_set_image(
		GTK_BUTTON(_widgets[WIDGET_DELETE_BUTTON]), 
		gtk_image_new_from_stock(GTK_STOCK_DELETE, GTK_ICON_SIZE_SMALL_TOOLBAR)
	);
	g_signal_connect(G_OBJECT(_widgets[WIDGET_DELETE_BUTTON]), "clicked", G_CALLBACK(onDelete), this);

	_widgets[WIDGET_RENAME_BUTTON] = gtk_button_new();
	gtk_button_set_image(
		GTK_BUTTON(_widgets[WIDGET_RENAME_BUTTON]), 
		gtk_image_new_from_stock(GTK_STOCK_EDIT, GTK_ICON_SIZE_SMALL_TOOLBAR)
	);
	g_signal_connect(G_OBJECT(_widgets[WIDGET_RENAME_BUTTON]), "clicked", G_CALLBACK(onRename), this);

	_widgets[WIDGET_BUTTON_HBOX] = gtk_hbox_new(FALSE, 6);
	gtk_box_pack_start(GTK_BOX(_widgets[WIDGET_BUTTON_HBOX]), _widgets[WIDGET_RENAME_BUTTON], FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(_widgets[WIDGET_BUTTON_HBOX]), _widgets[WIDGET_DELETE_BUTTON], FALSE, FALSE, 0);

	// Enable the tooltips group for the help mouseover texts
	gtk_tooltips_enable(_tooltips);
	gtk_tooltips_set_tip(_tooltips, _widgets[WIDGET_LABEL_BUTTON], _("Click to select all in layer, hold SHIFT to deselect"), "");
	gtk_tooltips_set_tip(_tooltips, _widgets[WIDGET_RENAME_BUTTON], _("Rename this layer"), "");
	gtk_tooltips_set_tip(_tooltips, _widgets[WIDGET_DELETE_BUTTON], _("Delete this layer"), "");
	gtk_tooltips_set_tip(_tooltips, _widgets[WIDGET_TOGGLE], _("Toggle layer visibility"), "");

	// Read the status from the Layer
	update();
}

GtkWidget* LayerControl::getLabelButton() const {
	return _widgets.find(WIDGET_LABEL_BUTTON)->second;
}

GtkWidget* LayerControl::getButtons() const {
	return _widgets.find(WIDGET_BUTTON_HBOX)->second;
}

GtkWidget* LayerControl::getToggle() const {
	return _widgets.find(WIDGET_TOGGLE)->second;
}

void LayerControl::update() {
	_updateActive = true;

	scene::LayerSystem& layerSystem = scene::getLayerSystem();

	bool layerIsVisible = layerSystem.layerIsVisible(_layerID);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(_widgets[WIDGET_TOGGLE]), layerIsVisible);

	gtk_button_set_label(GTK_BUTTON(_widgets[WIDGET_LABEL_BUTTON]), layerSystem.getLayerName(_layerID).c_str());

	std::string imageName = layerIsVisible ? ICON_LAYER_VISIBLE : ICON_LAYER_HIDDEN;
	gtk_button_set_image(
		GTK_BUTTON(_widgets[WIDGET_TOGGLE]), 
		gtk_image_new_from_pixbuf(GlobalUIManager().getLocalPixbufWithMask(imageName))
	);

	// Don't allow deleting or renaming layer 0
	gtk_widget_set_sensitive(_widgets[WIDGET_DELETE_BUTTON], _layerID != 0);
	gtk_widget_set_sensitive(_widgets[WIDGET_RENAME_BUTTON], _layerID != 0);

	// Don't allow selection of hidden layers
	gtk_widget_set_sensitive(_widgets[WIDGET_LABEL_BUTTON], layerIsVisible ? TRUE : FALSE);

	_updateActive = false;
}

void LayerControl::onToggle(GtkToggleButton* togglebutton, LayerControl* self) {
	if (self->_updateActive) return;

	scene::getLayerSystem().setLayerVisibility(
		self->_layerID, 
		gtk_toggle_button_get_active(togglebutton) ? true : false
	);
}

void LayerControl::onDelete(GtkWidget* button, LayerControl* self)
{
	// Ask the about the deletion
	std::string msg = _("Do you really want to delete this layer?");
	msg += "\n<b>" + scene::getLayerSystem().getLayerName(self->_layerID) + "</b>";

	Gtk::Window* topLevel = Glib::wrap(GTK_WINDOW(gtk_widget_get_toplevel(button)), true);
	
	IDialogPtr box = GlobalDialogManager().createMessageBox(
		_("Confirm Layer Deletion"), msg, IDialog::MESSAGE_ASK, Glib::RefPtr<Gtk::Window>(topLevel)
	);

	if (box->run() == IDialog::RESULT_YES)
	{
		scene::getLayerSystem().deleteLayer(
			scene::getLayerSystem().getLayerName(self->_layerID)
		);
		LayerControlDialog::Instance().refresh();
	}
}

void LayerControl::onRename(GtkWidget* button, LayerControl* self) {

	Gtk::Window* topLevel = Glib::wrap(GTK_WINDOW(gtk_widget_get_toplevel(self->_widgets[WIDGET_TOGGLE])), true);

	while (true) {
		// Query the name of the new layer from the user
		std::string newLayerName;

		try {
			newLayerName = gtkutil::textEntryDialog(
				_("Rename Layer"), 
				_("Enter new Layer Name"), 
				scene::getLayerSystem().getLayerName(self->_layerID),
				Glib::RefPtr<Gtk::Window>(topLevel)
			);
		}
		catch (gtkutil::EntryAbortedException e) {
			break;
		}

		// Attempt to rename the layer, this will return -1 if the operation fails
		bool success = scene::getLayerSystem().renameLayer(self->_layerID, newLayerName);

		if (success) {
			// Reload the widgets, we're done here
			self->update();
			break;
		}
		else {
			// Wrong name, let the user try again
			gtkutil::errorDialog(_("Could not rename layer, please try again."), Glib::RefPtr<Gtk::Window>(topLevel));
			continue; 
		}
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
