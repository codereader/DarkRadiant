#include "LightInspector.h"

#include "iselection.h"
#include "ientity.h"
#include "ieclass.h"
#include "ishaders.h"
#include "iregistry.h"

#include "mainframe.h"
#include "gtkutil/IconTextButton.h"
#include "gtkutil/LeftAlignedLabel.h"
#include "gtkutil/dialog.h"

#include <gtk/gtk.h>
#include <boost/algorithm/string/predicate.hpp>

namespace ui
{

/* CONSTANTS */

namespace {
	
	const char* LIGHTINSPECTOR_TITLE = "Light properties";
	const char* LIGHT_PREFIX_XPATH = "game/light/texture//prefix";

	GtkAttachOptions EXPFIL = static_cast<GtkAttachOptions>(GTK_EXPAND
															| GTK_FILL);
	
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

	GtkWidget* panels = gtk_vbox_new(FALSE, 6);
	gtk_box_pack_start(GTK_BOX(panels), createPointLightPanel(), FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(panels), gtk_hseparator_new(), FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(panels), createProjectedPanel(), FALSE, FALSE, 0);

	GtkWidget* hbx = gtk_hbox_new(FALSE, 3);
	gtk_box_pack_start(GTK_BOX(hbx), panels, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbx), gtk_vseparator_new(), FALSE, FALSE, 6);
	gtk_box_pack_start(GTK_BOX(hbx), createTextureWidgets(), TRUE, TRUE, 0);

	GtkWidget* vbx = gtk_vbox_new(FALSE, 3);
	gtk_box_pack_start(GTK_BOX(vbx), hbx, TRUE, TRUE, 0);
	gtk_box_pack_end(GTK_BOX(vbx), createButtons(), FALSE, FALSE, 0);

	gtk_container_set_border_width(GTK_CONTAINER(_widget), 3);
	gtk_container_add(GTK_CONTAINER(_widget), vbx);
	
	// Check widget sensitivity (point versus projected panels)
	updatePanels();
	
	// Populate the textures listbox
	populateTextures();
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

	// Pack button into box to stop it expanding vertically
	GtkWidget* buttonBox = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(buttonBox), _pointLightToggle, FALSE, FALSE, 0);
	
	// Table contains entries for radius and center
	_entryMap["radius"] = gtk_entry_new();
	_entryMap["center"] = gtk_entry_new();

	_pointPanel = gtk_table_new(2, 2, FALSE);
	gtk_table_attach(GTK_TABLE(_pointPanel), gtk_label_new("Radius"),
					 0, 1, 0, 1, // left, right, top, bottom
					 GTK_EXPAND, GTK_EXPAND, 3, 3); // xopts, yopts, xpad, ypad
	gtk_table_attach(GTK_TABLE(_pointPanel), gtk_label_new("Center"),
					 0, 1, 1, 2,
					 GTK_EXPAND, GTK_EXPAND, 3, 3);
	gtk_table_attach(GTK_TABLE(_pointPanel), _entryMap["radius"],
					 1, 2, 0, 1,
					 GTK_EXPAND, GTK_EXPAND, 3, 3);
	gtk_table_attach(GTK_TABLE(_pointPanel), _entryMap["center"],
					 1, 2, 1, 2,
					 GTK_EXPAND, GTK_EXPAND, 3, 3);
	
	// Main hbox for panel
	GtkWidget* hbx = gtk_hbox_new(FALSE, 6);
	gtk_box_pack_start(GTK_BOX(hbx), buttonBox, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbx), _pointPanel, TRUE, TRUE, 0);
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

	// Pack button into box to stop it expanding vertically
	GtkWidget* buttonBox = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(buttonBox), _projLightToggle, FALSE, FALSE, 0);

	// Table contains entries for up, right and target
	_entryMap["up"] = gtk_entry_new();
	_entryMap["right"] = gtk_entry_new();
	_entryMap["target"] = gtk_entry_new();

	_projPanel = gtk_table_new(3, 2, FALSE);
	gtk_table_attach(GTK_TABLE(_projPanel), gtk_label_new("Target"),
					 0, 1, 0, 1, // left, right, top, bottom
					 GTK_EXPAND, GTK_EXPAND, 3, 3); // xopts, yopts, xpad, ypad
	gtk_table_attach(GTK_TABLE(_projPanel), gtk_label_new("Up"),
					 0, 1, 1, 2,
					 GTK_EXPAND, GTK_EXPAND, 3, 3);
	gtk_table_attach(GTK_TABLE(_projPanel), gtk_label_new("Right"),
					 0, 1, 2, 3,
					 GTK_EXPAND, GTK_EXPAND, 3, 3);
	gtk_table_attach(GTK_TABLE(_projPanel), _entryMap["target"],
					 1, 2, 0, 1,
					 EXPFIL, GTK_EXPAND, 3, 3);
	gtk_table_attach(GTK_TABLE(_projPanel), _entryMap["up"],
					 1, 2, 1, 2,
					 EXPFIL, GTK_EXPAND, 3, 3);
	gtk_table_attach(GTK_TABLE(_projPanel), _entryMap["right"],
					 1, 2, 2, 3,
					 EXPFIL, GTK_EXPAND, 3, 3);

	// HBox for panel
	GtkWidget* hbx = gtk_hbox_new(FALSE, 6);
	gtk_box_pack_start(GTK_BOX(hbx), buttonBox, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbx), _projPanel, TRUE, TRUE, 0);
	return hbx;
}

// Create the texture widgets
GtkWidget* LightInspector::createTextureWidgets() {
	
	// VBox contains colour and texture selection widgets
	GtkWidget* vbx = gtk_vbox_new(FALSE, 3);
	
	_colour = gtk_color_button_new();
	_texture = gtk_combo_box_entry_new_text();
	
	gtk_box_pack_start(GTK_BOX(vbx), gtkutil::LeftAlignedLabel("Colour"), 
					   FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbx), _colour, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbx), gtkutil::LeftAlignedLabel("Texture"), 
					   FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbx), _texture, FALSE, FALSE, 0);
	
	return vbx;
}

// Create the buttons
GtkWidget* LightInspector::createButtons() {
	GtkWidget* hbx = gtk_hbox_new(FALSE, 3);

	GtkWidget* okButton = gtk_button_new_from_stock(GTK_STOCK_OK);
	GtkWidget* cancelButton = gtk_button_new_from_stock(GTK_STOCK_CANCEL);
	
	g_signal_connect(G_OBJECT(okButton), "clicked", 
					 G_CALLBACK(_onOK), this);
	g_signal_connect(G_OBJECT(cancelButton), "clicked", 
					 G_CALLBACK(_onCancel), this);

	gtk_box_pack_end(GTK_BOX(hbx), okButton, FALSE, FALSE, 0);
	gtk_box_pack_end(GTK_BOX(hbx), cancelButton, FALSE, FALSE, 0);

	return hbx;
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

// Populate the texture dropdown, using a functor to walk the list of textures
// and a list of light-texture prefixes from the game file

namespace {
	
	class LightShaderFunctor {
	
		// Prefixes we are interested in
		std::vector<std::string> _prefixes;
		
		// The ComboBoxEntry we are adding to
		GtkComboBox* _entry;
		
	public:
		
		// Required typedef
		typedef const char* first_argument_type;

		// Constructor
		LightShaderFunctor(GtkWidget* entry)
		: _entry(GTK_COMBO_BOX(entry))
		{
			// Get the list of light texture prefixes from the game file
			xml::NodeList prefList = 
				GlobalRegistry().findXPath(LIGHT_PREFIX_XPATH);
			
			// Copy the Node contents into the prefix vector	
			for (xml::NodeList::iterator i = prefList.begin();
				 i != prefList.end();
				 ++i)
			{
				_prefixes.push_back(i->getContent());
			}
		}
		
		// Functor operator
		void operator() (const char* texture) {
	
			// Check if the texture starts with a light-texture prefix. If so,
			// add it to the ComboBox
			std::string textureName(texture);
			for (std::vector<std::string>::iterator i = _prefixes.begin();
				 i != _prefixes.end();
				 ++i)
			{
				if (boost::algorithm::istarts_with(textureName, *i + "/")) {
					gtk_combo_box_append_text(_entry, texture);
				}
			}
					
		}
		
	}; // class

} // namespace

void LightInspector::populateTextures() {

	// Use the functor to populate the combo box
	LightShaderFunctor functor(_texture);
	GlobalShaderSystem().foreachShaderName(makeCallback1(functor));
	
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

	// Update widget sensitivity
	self->updatePanels();
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

	// Update widget sensitivity
	self->updatePanels();
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
	
	// Set shape keyvalues based on the light type
	if (self->_isProjected) {
		e->setKeyValue("light_target", 
				   gtk_entry_get_text(GTK_ENTRY(self->_entryMap["target"])));
		e->setKeyValue("light_right", 
				   gtk_entry_get_text(GTK_ENTRY(self->_entryMap["right"])));
		e->setKeyValue("light_up", 
				   gtk_entry_get_text(GTK_ENTRY(self->_entryMap["up"])));

		e->setKeyValue("light_radius", "");
		e->setKeyValue("light_center", "");
	}
	else {
		e->setKeyValue("light_radius", 
				   gtk_entry_get_text(GTK_ENTRY(self->_entryMap["radius"])));
		e->setKeyValue("light_center", 
				   gtk_entry_get_text(GTK_ENTRY(self->_entryMap["center"])));

		e->setKeyValue("light_target", "");
		e->setKeyValue("light_right", "");
		e->setKeyValue("light_up", "");
	}
	
	// Hide the dialog
	gtk_widget_hide(self->_widget);
}

void LightInspector::_onCancel(GtkWidget* w, LightInspector* self) {
	gtk_widget_hide(self->_widget);
}

// Update panel state
void LightInspector::updatePanels() {
	if (_isProjected) {
		gtk_widget_set_sensitive(_projPanel, TRUE);	
		gtk_widget_set_sensitive(_pointPanel, FALSE);	
	}
	else {
		gtk_widget_set_sensitive(_projPanel, FALSE);	
		gtk_widget_set_sensitive(_pointPanel, TRUE);
	}
}

// Get keyvals from entity and insert into text entries
void LightInspector::getValuesFromEntity() {

	// First set default values, in case entity does not contain all keys (which
	// it probably won't).
	gtk_entry_set_text(GTK_ENTRY(_entryMap["radius"]), "320 320 320");
	gtk_entry_set_text(GTK_ENTRY(_entryMap["center"]), "0 0 0");
	gtk_entry_set_text(GTK_ENTRY(_entryMap["target"]), "0 0 -256");
	gtk_entry_set_text(GTK_ENTRY(_entryMap["right"]), "128 0 0");
	gtk_entry_set_text(GTK_ENTRY(_entryMap["up"]), "0 128 0");

	// Iterate over each entry in the EntryMap, retrieving the corresponding
	// keyvalue
	for (EntryMap::iterator i = _entryMap.begin();
		 i != _entryMap.end();
		 ++i)
	{
		// Get the value from the entity
		std::string val = _entity->getKeyValue("light_" + i->first);

		// Only set the widget text if the entity has a value, otherwise leave
		// at default
		if (!val.empty()) {
			gtk_entry_set_text(GTK_ENTRY(i->second), val.c_str());
		}
	}
	
	// Get the colour key from the entity to set the GtkColorButton
	Vector3 colour(_entity->getKeyValue("_color"));
	GdkColor col = { 0,
					 static_cast<guint>(colour.x() * 65535),
					 static_cast<guint>(colour.y() * 65535),
					 static_cast<guint>(colour.z() * 65535) };
	gtk_color_button_set_color(GTK_COLOR_BUTTON(_colour), &col);
}

} // namespace ui
