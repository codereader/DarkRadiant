#include "LightInspector.h"

#include "mainframe.h"
#include "gtkutil/IconTextButton.h"

#include <gtk/gtk.h>

namespace ui
{

/* CONSTANTS */

namespace {
	
	const char* LIGHTINSPECTOR_TITLE = "Light properties";
	
}

// Private constructor creates GTK widgets
LightInspector::LightInspector()
: _widget(gtk_window_new(GTK_WINDOW_TOPLEVEL)),
  _isProjected(false)
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
	gtk_box_pack_start(GTK_BOX(panels), createPointLightPanel(), TRUE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(panels), createProjectedPanel(), TRUE, FALSE, 0);

	GtkWidget* hbx = gtk_hbox_new(FALSE, 3);
	gtk_box_pack_start(GTK_BOX(hbx), panels, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(hbx), createTextureWidgets(), TRUE, TRUE, 0);

	GtkWidget* vbx = gtk_vbox_new(FALSE, 3);
	gtk_box_pack_start(GTK_BOX(vbx), hbx, TRUE, TRUE, 0);
	gtk_box_pack_end(GTK_BOX(vbx), createButtons(), FALSE, FALSE, 0);

	gtk_container_set_border_width(GTK_CONTAINER(_widget), 3);
	gtk_container_add(GTK_CONTAINER(_widget), vbx);
	
}

// Create the point light panel
GtkWidget* LightInspector::createPointLightPanel() {
	// Create the point light togglebutton
	_pointLightToggle = gtkutil::IconTextButton("Omni", 
					   						   "pointLight32.png",
					   						   true);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(_pointLightToggle), TRUE);
	g_signal_connect(G_OBJECT(_pointLightToggle), 
					 "toggled",
					 G_CALLBACK(_onPointToggle),
					 this);
	
	// HBox for panel
	GtkWidget* hbx = gtk_hbox_new(FALSE, 3);
	gtk_box_pack_start(GTK_BOX(hbx), _pointLightToggle, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbx), gtk_label_new("Point light"), TRUE, FALSE, 0);
	return hbx;
}

// Create the projected light panel
GtkWidget* LightInspector::createProjectedPanel() {
	// Create the projected light togglebutton
	_projLightToggle = gtkutil::IconTextButton("Projected", 
					   						   "projLight32.png",
					   						   true);
	g_signal_connect(G_OBJECT(_projLightToggle), 
					 "toggled",
					 G_CALLBACK(_onProjToggle),
					 this);

	// HBox for panel
	GtkWidget* hbx = gtk_hbox_new(FALSE, 3);
	gtk_box_pack_start(GTK_BOX(hbx), _projLightToggle, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbx), gtk_label_new("Projected light"), TRUE, FALSE, 0);
	return hbx;
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

/* GTK CALLBACKS */

void LightInspector::_onProjToggle(GtkWidget* b, LightInspector* self) {
	// Set the projected flag
	self->_isProjected = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(b));
	
	// Set button state based on the value of the flag
	if (self->_isProjected)
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(self->_pointLightToggle),
									 FALSE);	
	else
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(self->_pointLightToggle),
									 TRUE);	
}

void LightInspector::_onPointToggle(GtkWidget* b, LightInspector* self) {
	// Set the projected flag
	self->_isProjected = !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(b));
	
	// Set button state based on the value of the flag
	if (self->_isProjected)
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(self->_projLightToggle),
									 TRUE);	
	else
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(self->_projLightToggle),
									 FALSE);	
}

} // namespace ui
