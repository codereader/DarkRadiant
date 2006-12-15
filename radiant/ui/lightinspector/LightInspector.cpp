#include "LightInspector.h"

#include "iselection.h"
#include "ientity.h"
#include "ieclass.h"
#include "ishaders.h"
#include "iregistry.h"

#include "mainframe.h"
#include "gtkutil/IconTextButton.h"
#include "gtkutil/LeftAlignedLabel.h"
#include "gtkutil/RightAlignment.h"
#include "gtkutil/LeftAlignment.h"
#include "gtkutil/dialog.h"

#include <gtk/gtk.h>
#include <boost/algorithm/string/predicate.hpp>

namespace ui
{

/* CONSTANTS */

namespace {
	
	const char* LIGHTINSPECTOR_TITLE = "Light properties";
	
	const char* NOSHADOW_TEXT = "Do not cast shadows (fast)";
	const char* NOSPECULAR_TEXT = "Skip specular lighting";
	const char* NODIFFUSE_TEXT = "Skip diffuse lighting";
	
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
    
    // Window size
	GdkScreen* scr = gtk_window_get_screen(GTK_WINDOW(_widget));
	gtk_window_set_default_size(GTK_WINDOW(_widget), 
								gint(gdk_screen_get_width(scr) * 0.6), 
								-1);
    
    // Widget must hide not destroy when closed
    g_signal_connect(G_OBJECT(_widget), 
    				 "delete-event",
    				 G_CALLBACK(gtk_widget_hide_on_delete),
    				 NULL);

	// Pack in widgets. 

	// Left-hand panels (volume, colour, options)
	GtkWidget* panels = gtk_vbox_new(FALSE, 12);

	gtk_box_pack_start(GTK_BOX(panels), 
					   gtkutil::LeftAlignedLabel("<b>Light volume</b>"),
					   FALSE, FALSE, 0);

	// Volume type hbox
	GtkWidget* typeBox = gtk_hbox_new(FALSE, 12);
	gtk_box_pack_start(GTK_BOX(typeBox), createPointLightPanel(),
					   FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(typeBox), gtk_vseparator_new(),
					   FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(typeBox), createProjectedPanel(),
					   FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(panels), 
					   gtkutil::LeftAlignment(typeBox, 12),
					   FALSE, FALSE, 0);

	// Light colour
	_colour = gtk_color_button_new();
	gtk_box_pack_start(GTK_BOX(panels), 
					   gtkutil::LeftAlignedLabel("<b>Colour</b>"), 
					   FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(panels), 
					   gtkutil::LeftAlignment(_colour, 12, 0.0),
					   FALSE, FALSE, 0);

	// Options panel
	gtk_box_pack_start(GTK_BOX(panels),
					   gtkutil::LeftAlignedLabel("<b>Options</b>"),
					   FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(panels),
					   gtkutil::LeftAlignment(createOptionsPanel(), 12),
					   FALSE, FALSE, 0);

	GtkWidget* hbx = gtk_hbox_new(FALSE, 18);
	gtk_box_pack_start(GTK_BOX(hbx), panels, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbx), createTextureWidgets(), TRUE, TRUE, 0);

	GtkWidget* vbx = gtk_vbox_new(FALSE, 12);
	gtk_box_pack_start(GTK_BOX(vbx), hbx, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbx), gtk_hseparator_new(), FALSE, FALSE, 0);
	gtk_box_pack_end(GTK_BOX(vbx), createButtons(), FALSE, FALSE, 0);

	gtk_container_set_border_width(GTK_CONTAINER(_widget), 12);
	gtk_container_add(GTK_CONTAINER(_widget), vbx);
	
}

// Create the point light panel
GtkWidget* LightInspector::createPointLightPanel() {

	// Create the point light togglebutton
	_pointLightToggle = gtkutil::IconTextButton("Omni", 
					   						   "pointLight32.png",
					   						   true);
	g_signal_connect(G_OBJECT(_pointLightToggle), 
					 "toggled",
					 G_CALLBACK(_onPointToggle),
					 this);

	// Pack button into box to stop it expanding vertically
	GtkWidget* buttonBox = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(buttonBox), _pointLightToggle, FALSE, FALSE, 0);
	
	return buttonBox;
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

	// Start/end checkbox
	_useStartEnd = gtk_check_button_new_with_label("Use start and end planes");
					
	// VBox for panel
	GtkWidget* vbx = gtk_vbox_new(FALSE, 12);
	gtk_box_pack_start(GTK_BOX(vbx), 
					   gtkutil::LeftAlignment(_projLightToggle), 
					   FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbx), _useStartEnd, FALSE, FALSE, 0);
	return vbx;
}

// Create the options checkboxes
GtkWidget* LightInspector::createOptionsPanel() {
	GtkWidget* vbx = gtk_vbox_new(FALSE, 6);
	gtk_box_pack_start(GTK_BOX(vbx), 
					   gtk_check_button_new_with_label(NOSHADOW_TEXT),
					   FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbx), 
					   gtk_check_button_new_with_label(NOSPECULAR_TEXT),
					   FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbx), 
					   gtk_check_button_new_with_label(NODIFFUSE_TEXT),
					   FALSE, FALSE, 0);
	return vbx;
}

// Create the texture widgets
GtkWidget* LightInspector::createTextureWidgets() {
	
	// VBox contains texture selection widgets
	GtkWidget* vbx = gtk_vbox_new(FALSE, 12);
	
	gtk_box_pack_start(GTK_BOX(vbx), 
					   gtkutil::LeftAlignedLabel("<b>Texture</b>"), 
					   FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbx), 
					   gtkutil::LeftAlignment(_texSelector, 12, 1.0),
					   TRUE, TRUE, 0);
	
	return vbx;
}

// Create the buttons
GtkWidget* LightInspector::createButtons() {
	GtkWidget* hbx = gtk_hbox_new(TRUE, 6);

	GtkWidget* okButton = gtk_button_new_from_stock(GTK_STOCK_OK);
	GtkWidget* cancelButton = gtk_button_new_from_stock(GTK_STOCK_CANCEL);
	
	g_signal_connect(G_OBJECT(okButton), "clicked", 
					 G_CALLBACK(_onOK), this);
	g_signal_connect(G_OBJECT(cancelButton), "clicked", 
					 G_CALLBACK(_onCancel), this);

	gtk_box_pack_end(GTK_BOX(hbx), okButton, TRUE, TRUE, 0);
	gtk_box_pack_end(GTK_BOX(hbx), cancelButton, TRUE, TRUE, 0);

	return gtkutil::RightAlignment(hbx);
}

// Show this dialog
void LightInspector::show() {

	// Prepare to check for a valid selection. We need exactly one object 
	// selected and it must be a light. Anything else results in an error.
	SelectionSystem& s = GlobalSelectionSystem();

	// Abort if selection count is not 1
	if (s.countSelected() != 1) {
		gtkutil::errorDialog("Exactly one light must be selected.");
		return;
	}
	
	// Check the EntityClass to ensure it is a light, otherwise abort
	Entity* e = NodeTypeCast<Entity>::cast(s.ultimateSelected().path().top());
	if (e == NULL 							// not an entity
		|| !e->getEntityClass().isLight())	// not a light
	{
		gtkutil::errorDialog("The selected entity must be a light.");
		return;	
	}

	// Everything OK, set the entity and show the dialog
	_entity = e;
	gtk_widget_show_all(_widget);
	
	// Update the dialog with the correct values from the entity
	getValuesFromEntity();
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
	if (self->_isProjected) {
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(self->_pointLightToggle),
									 FALSE);	
		gtk_widget_set_sensitive(self->_useStartEnd, TRUE);
	}
	else {
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(self->_pointLightToggle),
									 TRUE);	
		gtk_widget_set_sensitive(self->_useStartEnd, FALSE);
	}
}

void LightInspector::_onPointToggle(GtkWidget* b, LightInspector* self) {
	// Set the projected flag
	self->_isProjected = !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(b));
	
	// Set button state based on the value of the flag
	if (self->_isProjected) {
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(self->_projLightToggle),
									 TRUE);	
		gtk_widget_set_sensitive(self->_useStartEnd, TRUE);
	}
	else {
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(self->_projLightToggle),
									 FALSE);
		gtk_widget_set_sensitive(self->_useStartEnd, FALSE);
	}
}

void LightInspector::_onOK(GtkWidget* w, LightInspector* self) {
	
	// Get a local pointer to the entity
	Entity* e = self->_entity;
	
	// Set the "_color" keyvalue
	GdkColor col;
	gtk_color_button_get_color(GTK_COLOR_BUTTON(self->_colour), &col);
	e->setKeyValue("_color", (boost::format("%.2f %.2f %.2f") 
						  	  % (col.red/65535.0)
						  	  % (col.green/65535.0)
						  	  % (col.blue/65535.0)).str());
	
	/* Set shape keyvalues based on the light type, but only if it has changed
	 * in the dialog (to avoid replacing unchanged values with defaults). We
	 * are only changing the status here (pointlight versus projected); the 
	 * actual vectors involved will be set through mouse dragging or editing
	 * in the entity inspector.
	 */
	StringMap& _valueMap = self->_valueMap; // local reference
	if (self->_isProjected) {
		std::cout << "projected" << std::endl;
		// Pointlight changed to projected light
		e->setKeyValue("light_target", _valueMap["light_target"]);
		e->setKeyValue("light_right", _valueMap["light_right"]);
		e->setKeyValue("light_up", _valueMap["light_up"]);

		// Set start and end vectors if enabled, otherwise clear them
		if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(self->_useStartEnd)))
		{
			e->setKeyValue("light_start", _valueMap["light_start"]);
			e->setKeyValue("light_end", _valueMap["light_end"]);	
		}
		else {
			e->setKeyValue("light_start", "");
			e->setKeyValue("light_end", "");	
		}

		// Blank out pointlight values
		e->setKeyValue("light_radius", "");
		e->setKeyValue("light_center", "");
	}
	else {

		// Projected light changed to pointlight
		e->setKeyValue("light_radius", _valueMap["light_radius"]);
		e->setKeyValue("light_center", _valueMap["light_center"]);

		// Blank out projected light values
		e->setKeyValue("light_target", "");
		e->setKeyValue("light_right", "");
		e->setKeyValue("light_up", "");
		e->setKeyValue("light_start", "");
		e->setKeyValue("light_end", "");
	}

	// Write the texture key
	e->setKeyValue("texture", self->_texSelector.getSelection());
	
	// Hide the dialog
	gtk_widget_hide(self->_widget);
}

void LightInspector::_onCancel(GtkWidget* w, LightInspector* self) {
	gtk_widget_hide(self->_widget);
}

// Get keyvals from entity and insert into text entries
void LightInspector::getValuesFromEntity() {

	// Populate the value map with defaults
	_valueMap["light_radius"] = "320 320 320";
	_valueMap["light_center"] = "0 0 0";
	_valueMap["light_target"] = "0 0 -256";
	_valueMap["light_right"] = "128 0 0";
	_valueMap["light_up"] = "0 128 0";
	_valueMap["light_start"] = "0 0 -64";
	_valueMap["light_end"] = "0 0 -256";

	// Now load values from entity, overwriting the defaults if the value is
	// set
	for (StringMap::iterator i = _valueMap.begin();
		 i != _valueMap.end();
		 ++i)
	{
		// Overwrite the map value if the key exists on the entity
		std::string val = _entity->getKeyValue(i->first);
		if (!val.empty())
			i->second = val;
	}

	// Get the colour key from the entity to set the GtkColorButton
	Vector3 colour(_entity->getKeyValue("_color"));
	GdkColor col = { 0,
					 static_cast<guint>(colour.x() * 65535),
					 static_cast<guint>(colour.y() * 65535),
					 static_cast<guint>(colour.z() * 65535) };
	gtk_color_button_set_color(GTK_COLOR_BUTTON(_colour), &col);
	
	// Set the texture selection from the "texture" key
	_texSelector.setSelection(_entity->getKeyValue("texture"));
	
	// Determine whether this is a projected light, and set the toggles
	// appropriately
	if (!_entity->getKeyValue("light_target").empty()
		&& !_entity->getKeyValue("light_right").empty()
		&& !_entity->getKeyValue("light_up").empty())
	{
		// Is a projected light
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(_projLightToggle),
									 TRUE);
	}
	else {
		// Is a point light
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(_pointLightToggle),
									 TRUE);
	}
	
	// If this entity has light_start and light_end keys, set the checkbox
	if (!_entity->getKeyValue("light_start").empty()
		&& !_entity->getKeyValue("light_end").empty())
	{
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(_useStartEnd), TRUE);
	}
	else {
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(_useStartEnd), FALSE);
	}
}

} // namespace ui
