#include "OverlayDialog.h"
#include "mainframe.h"

#include "ioverlay.h"
#include "iscenegraph.h"

#include <gtk/gtk.h>

namespace ui
{

/* CONSTANTS */
namespace {
	
	const char* DIALOG_TITLE = "Background image";
	
}

// Create GTK stuff in c-tor
OverlayDialog::OverlayDialog()
: _widget(gtk_window_new(GTK_WINDOW_TOPLEVEL))
{
	// Set up the window
    gtk_window_set_position(GTK_WINDOW(_widget), GTK_WIN_POS_CENTER_ON_PARENT);
    gtk_window_set_transient_for(GTK_WINDOW(_widget), MainFrame_getWindow());
    gtk_window_set_title(GTK_WINDOW(_widget), DIALOG_TITLE);
    g_signal_connect(G_OBJECT(_widget), "delete-event",
    				 G_CALLBACK(gtk_widget_hide_on_delete), NULL);
    				 
	// Pack in widgets
	GtkWidget* vbx = gtk_vbox_new(FALSE, 12);
	gtk_box_pack_start(GTK_BOX(vbx), createWidgets(), TRUE, TRUE, 0);
	gtk_box_pack_end(GTK_BOX(vbx), createButtons(), FALSE, FALSE, 0);
	
	gtk_container_set_border_width(GTK_CONTAINER(_widget), 12);
	gtk_container_add(GTK_CONTAINER(_widget), vbx);	
}

// Construct main widgets
GtkWidget* OverlayDialog::createWidgets() {
	GtkWidget* useImage = gtk_check_button_new_with_label(
							"Use background image"); 
	g_signal_connect(G_OBJECT(useImage), "toggled",
					 G_CALLBACK(_onUseImage), this);
	return useImage;
}

// Create button panel
GtkWidget* OverlayDialog::createButtons() {
	GtkWidget* hbx = gtk_hbox_new(FALSE, 6);
	
	GtkWidget* closeButton = gtk_button_new_from_stock(GTK_STOCK_CLOSE);
	g_signal_connect(G_OBJECT(closeButton), "clicked",
					 G_CALLBACK(_onClose), this); 
	
	gtk_box_pack_end(GTK_BOX(hbx), closeButton, FALSE, FALSE, 0);
	return hbx;
}

// Static show method
void OverlayDialog::display() {
	
	// Maintain a static dialog instance and display it on demand
	static OverlayDialog _instance;
	gtk_widget_show_all(_instance._widget);
}

/* GTK CALLBACKS */

// Close button
void OverlayDialog::_onClose(GtkWidget* w, OverlayDialog* self) {
	gtk_widget_hide(self->_widget);
}

// Use image toggle
void OverlayDialog::_onUseImage(GtkToggleButton* w, OverlayDialog* self) {
	
	// Enable or disable the overlay based on checked status
	if (gtk_toggle_button_get_active(w)) {
		GlobalOverlay().show(true);
	}
	else {
		GlobalOverlay().show(false);
	}	
	
	// Refresh
	GlobalSceneGraph().sceneChanged();
}

} // namespace ui
