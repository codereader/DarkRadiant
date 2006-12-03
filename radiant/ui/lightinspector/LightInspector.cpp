#include "LightInspector.h"

#include "mainframe.h"

#include <gtk/gtk.h>

namespace ui
{

/* CONSTANTS */

namespace {
	
	const char* LIGHTINSPECTOR_TITLE = "Light properties";
	
}

// Private constructor creates GTK widgets
LightInspector::LightInspector()
: _widget(gtk_window_new(GTK_WINDOW_TOPLEVEL))
{
	// Window properties
	gtk_window_set_transient_for(GTK_WINDOW(_widget), MainFrame_getWindow());
	gtk_window_set_modal(GTK_WINDOW(_widget), TRUE);
	gtk_window_set_title(GTK_WINDOW(_widget), LIGHTINSPECTOR_TITLE);
    gtk_window_set_position(GTK_WINDOW(_widget), GTK_WIN_POS_CENTER_ON_PARENT);
    
    // Widget must hide not destroy when closed
    g_signal_connect(G_OBJECT(_widget), 
    				 "delete-event",
    				 G_CALLBACK(gtk_widget_hide_on_delete),
    				 NULL);

	// Pack in widgets. On the left are two panels with toggle buttons, for
	// editing either pointlights or projected lights. On the right are the
	// texture selection widgets.

	GtkWidget* panels = gtk_vbox_new(FALSE, 3);
	gtk_box_pack_start(GTK_BOX(panels), createPointLightPanel(), TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(panels), createProjectedPanel(), TRUE, TRUE, 0);

	GtkWidget* hbx = gtk_hbox_new(FALSE, 3);
	gtk_box_pack_start(GTK_BOX(hbx), panels, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(hbx), createTextureWidgets(), TRUE, TRUE, 0);

	GtkWidget* vbx = gtk_vbox_new(FALSE, 3);
	gtk_box_pack_start(GTK_BOX(vbx), hbx, TRUE, TRUE, 0);
	gtk_box_pack_end(GTK_BOX(vbx), createButtons(), FALSE, FALSE, 0);

	gtk_container_add(GTK_CONTAINER(_widget), vbx);
	
}

// Create the point light panel
GtkWidget* LightInspector::createPointLightPanel() {
	return gtk_label_new("Point light");
}

// Create the projected light panel
GtkWidget* LightInspector::createProjectedPanel() {
	return gtk_label_new("Projected light");
	
}

// Create the texture widgets
GtkWidget* LightInspector::createTextureWidgets() {
	return gtk_label_new("Texture widgets");
}

// Create the buttons
GtkWidget* LightInspector::createButtons() {
	GtkWidget* hbx = gtk_hbox_new(FALSE, 3);
	gtk_box_pack_end(GTK_BOX(hbx), 
					 gtk_button_new_from_stock(GTK_STOCK_OK), 
					 FALSE, FALSE, 0);
	gtk_box_pack_end(GTK_BOX(hbx), 
					 gtk_button_new_from_stock(GTK_STOCK_CANCEL), 
					 FALSE, FALSE, 0);
	return hbx;
}
// Show this dialog
void LightInspector::show() {
	gtk_widget_show_all(_widget);	
}

// Static method to display the dialog
void LightInspector::displayDialog() {
	// Static instance
	static LightInspector _instance;

	// Show the instance
	_instance.show();	
}

} // namespace ui
