#include "LightInspector.h"

#include "ieventmanager.h"
#include "ientity.h"
#include "ieclass.h"
#include "ishaders.h"
#include "iregistry.h"
#include "iundo.h"

#include "mainframe.h"
#include "gtkutil/TransientWindow.h"
#include "gtkutil/IconTextButton.h"
#include "gtkutil/LeftAlignedLabel.h"
#include "gtkutil/RightAlignment.h"
#include "gtkutil/LeftAlignment.h"
#include "gtkutil/dialog.h"

#include "ui/common/ShaderChooser.h" // for static displayLightInfo() function

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
	
	const std::string RKEY_WINDOW_STATE = "user/ui/lightInspector/window";
	const std::string RKEY_INSTANT_APPLY = "user/ui/lightInspector/instantApply";
	
	const char* LIGHT_PREFIX_XPATH = "game/light/texture//prefix";
		
	/** greebo: Loads the prefixes from the registry and creates a 
	 * 			comma-separated list string
	 */
	inline std::string getPrefixList() {
		std::string prefixes;
		
		// Get the list of light texture prefixes from the registry
		xml::NodeList prefList = GlobalRegistry().findXPath(LIGHT_PREFIX_XPATH);
		
		// Copy the Node contents into the prefix vector	
		for (xml::NodeList::iterator i = prefList.begin();
			 i != prefList.end();
			 ++i)
		{
			prefixes += (prefixes.empty()) ? "" : ",";
			prefixes += i->getContent();
		}
		
		return prefixes;
	}
}

// Private constructor creates GTK widgets
LightInspector::LightInspector() : 
	// Be sure to pass FALSE to the TransientWindow to prevent it from self-destruction
	_widget(gtkutil::TransientWindow(LIGHTINSPECTOR_TITLE, MainFrame_getWindow(), false)),
	_isProjected(false),
	_texSelector(this, getPrefixList(), true),
	_entity(NULL)
{
	gtk_window_set_type_hint(GTK_WINDOW(_widget), GDK_WINDOW_TYPE_HINT_DIALOG);
	
    // Window size
	GdkScreen* scr = gtk_window_get_screen(GTK_WINDOW(_widget));
	gtk_window_set_default_size(GTK_WINDOW(_widget), 
								gint(gdk_screen_get_width(scr) * 0.5), 
								-1);
    
    // Widget must hide not destroy when closed
    g_signal_connect(G_OBJECT(_widget), "delete-event", G_CALLBACK(onDelete), this);

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
	g_signal_connect(G_OBJECT(_colour), "color-set", G_CALLBACK(_onColourChange), this);
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

	_mainVBox = gtk_vbox_new(FALSE, 12);
	gtk_box_pack_start(GTK_BOX(_mainVBox), hbx, TRUE, TRUE, 0);
	
	// Create an apply button, if instant-apply is disabled
	if (GlobalRegistry().get(RKEY_INSTANT_APPLY) == "0") {
		gtk_box_pack_start(GTK_BOX(_mainVBox), gtk_hseparator_new(), FALSE, FALSE, 0);
		gtk_box_pack_end(GTK_BOX(_mainVBox), createButtons(), FALSE, FALSE, 0);
	}

	gtk_container_set_border_width(GTK_CONTAINER(_widget), 12);
	gtk_container_add(GTK_CONTAINER(_widget), _mainVBox);
	
	// Register to get notified upon selection change
	GlobalSelectionSystem().addObserver(this);
	
	// Propagate shortcuts that are not processed by this window
	GlobalEventManager().connectDialogWindow(GTK_WINDOW(_widget));
	
	// Connect the window position tracker
	xml::NodeList windowStateList = GlobalRegistry().findXPath(RKEY_WINDOW_STATE);
	
	if (windowStateList.size() > 0) {
		_windowPosition.loadFromNode(windowStateList[0]);
	}
	
	_windowPosition.connect(GTK_WINDOW(_widget));
	_windowPosition.applyPosition();
}

void LightInspector::shutdown() {
	// Delete all the current window states from the registry  
	GlobalRegistry().deleteXPath(RKEY_WINDOW_STATE);
	
	// Create a new node
	xml::Node node(GlobalRegistry().createKey(RKEY_WINDOW_STATE));
	
	// Tell the position tracker to save the information
	_windowPosition.saveToNode(node);
	
	gtk_widget_hide(_widget);
	
	GlobalSelectionSystem().removeObserver(this);
	GlobalEventManager().disconnectDialogWindow(GTK_WINDOW(_widget));
}

void LightInspector::shaderSelectionChanged(
	const std::string& shader, 
	GtkListStore* listStore) 
{
	// Get the shader, and its image map if possible
	IShaderPtr ishader = _texSelector.getSelectedShader();
	// Pass the call to the static member of ShaderSelector
	ShaderSelector::displayLightShaderInfo(ishader, listStore);
	
	if (GlobalRegistry().get(RKEY_INSTANT_APPLY) == "1") {
		std::string commandStr("setLightTexture: ");
		commandStr += _texSelector.getSelection();
		UndoableCommand command(commandStr.c_str());
	
		// Write the texture key
		_entity->setKeyValue("texture", _texSelector.getSelection());
	}
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
	_useStartEnd = gtk_check_button_new_with_label("Use start/end");
	g_signal_connect(G_OBJECT(_useStartEnd), "toggled", G_CALLBACK(_onOptionsToggle), this);		
		
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

	// Add options boxes to map
	_options["noshadows"] = gtk_check_button_new_with_label(NOSHADOW_TEXT);
	_options["nospecular"] = gtk_check_button_new_with_label(NOSPECULAR_TEXT);
	_options["nodiffuse"] = gtk_check_button_new_with_label(NODIFFUSE_TEXT);

	g_signal_connect(G_OBJECT(_options["noshadows"]), "toggled", G_CALLBACK(_onOptionsToggle), this);
	g_signal_connect(G_OBJECT(_options["nospecular"]), "toggled", G_CALLBACK(_onOptionsToggle), this);
	g_signal_connect(G_OBJECT(_options["nodiffuse"]), "toggled", G_CALLBACK(_onOptionsToggle), this);

	// Pack checkboxes into a VBox
	GtkWidget* vbx = gtk_vbox_new(FALSE, 6);
	gtk_box_pack_start(GTK_BOX(vbx), _options["noshadows"], FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbx), _options["nospecular"], FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbx), _options["nodiffuse"], FALSE, FALSE, 0);
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

	GtkWidget* okButton = gtk_button_new_from_stock(GTK_STOCK_APPLY);
		
	g_signal_connect(G_OBJECT(okButton), "clicked", 
					 G_CALLBACK(_onOK), this);
	
	gtk_box_pack_end(GTK_BOX(hbx), okButton, TRUE, TRUE, 0);
	
	return gtkutil::RightAlignment(hbx);
}

void LightInspector::update() {
	// Set the entity pointer to NULL, just to be sure
	_entity = NULL;
	
	// Prepare to check for a valid selection. We need exactly one object 
	// selected and it must be a light.
	SelectionSystem& s = GlobalSelectionSystem();

	bool sensitive = false;
	
	if (s.countSelected() == 1) {
		// Check the EntityClass to ensure it is a light
		Entity* e = NodeTypeCast<Entity>::cast(s.ultimateSelected().path().top());
		
		if (e != NULL && e->getEntityClass()->isLight()) {
			// Exactly one light found, set the entity pointer
			_entity = e;
			
			// Update the dialog with the correct values from the entity
			getValuesFromEntity();
			
			sensitive = true;
		}
	}
	
	// Set the sensitivity of the widgets
	gtk_widget_set_sensitive(_mainVBox, sensitive);
}

// Toggle this dialog
void LightInspector::toggle() {
	// Pass the call to the utility methods that save/restore the window position
	if (GTK_WIDGET_VISIBLE(_widget)) {
		// Save the window position, to make sure
		_windowPosition.readPosition();
		// Hide the dialog window
		gtk_widget_hide_all(_widget);
	}
	else {
		// Restore the position
		_windowPosition.applyPosition();
		// Update the widgets
		update();
		// Display the dialog
		gtk_widget_show_all(_widget);
	}
}

void LightInspector::selectionChanged(scene::Instance& instance) {
	update();
}

// Static method to toggle the dialog
void LightInspector::toggleInspector() {
	// Toggle the instance
	Instance().toggle();
}

LightInspector& LightInspector::Instance() {
	static LightInspector _instance;
	return _instance;
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
	if (GlobalRegistry().get(RKEY_INSTANT_APPLY) == "1") {
		self->setValuesOnEntity();
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
	if (GlobalRegistry().get(RKEY_INSTANT_APPLY) == "1") {
		self->setValuesOnEntity();
	}
}

void LightInspector::_onOK(GtkWidget* w, LightInspector* self) {
	self->setValuesOnEntity();
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

	// Get the colour key from the entity to set the GtkColorButton. If the 
	// light has no colour key, use a default of white rather than the Vector3
	// default of black (0, 0, 0).
	std::string colString = _entity->getKeyValue("_color");
	if (colString.empty())
		colString = "1.0 1.0 1.0";
		 
	Vector3 colour(colString);
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
	
	// Set the options checkboxes
	for (WidgetMap::iterator i = _options.begin(); i != _options.end(); ++i) {
		if (_entity->getKeyValue(i->first) == "1")
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(i->second), TRUE);
		else
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(i->second), FALSE);
	}
}

// Set the keyvalues on the entity from the dialog widgets
void LightInspector::setValuesOnEntity() {
	UndoableCommand command("setLightProperties");
	
	// Set the "_color" keyvalue
	GdkColor col;
	gtk_color_button_get_color(GTK_COLOR_BUTTON(_colour), &col);
	_entity->setKeyValue("_color", (boost::format("%.2f %.2f %.2f") 
						  	  		% (col.red/65535.0)
						  	  		% (col.green/65535.0)
						  	  		% (col.blue/65535.0)).str());
	
	// Write out all vectors to the entity
	for (StringMap::iterator i = _valueMap.begin();
		 i != _valueMap.end();
		 ++i) 
	{
		_entity->setKeyValue(i->first, i->second);
	}
		 	
	// Remove vector keys that should not exist, depending on the lightvolume
	// options
	if (_isProjected) {

		// Clear start/end vectors if checkbox is disabled
		if (!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(_useStartEnd))) {
			_entity->setKeyValue("light_start", "");
			_entity->setKeyValue("light_end", "");	
		}

		// Blank out pointlight values
		_entity->setKeyValue("light_radius", "");
		_entity->setKeyValue("light_center", "");
	}
	else {

		// Blank out projected light values
		_entity->setKeyValue("light_target", "");
		_entity->setKeyValue("light_right", "");
		_entity->setKeyValue("light_up", "");
		_entity->setKeyValue("light_start", "");
		_entity->setKeyValue("light_end", "");
	}

	// Write the texture key
	_entity->setKeyValue("texture", _texSelector.getSelection());

	// Write the options
	for (WidgetMap::iterator i = _options.begin(); i != _options.end(); ++i) {
		if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(i->second)))
			_entity->setKeyValue(i->first, "1");
		else
			_entity->setKeyValue(i->first, "0");
	}
}

void LightInspector::_onOptionsToggle(GtkToggleButton* togglebutton, LightInspector *self) {
	if (GlobalRegistry().get(RKEY_INSTANT_APPLY) == "1") {
		self->setValuesOnEntity();
	}
}

void LightInspector::_onColourChange(GtkColorButton* widget, LightInspector* self) {
	if (GlobalRegistry().get(RKEY_INSTANT_APPLY) == "1") {
		self->setValuesOnEntity();
	}
}

gboolean LightInspector::onDelete(GtkWidget* widget, GdkEvent* event, LightInspector* self) {
	// Toggle the visibility of the inspector window
	self->toggle();
	
	// Don't propagate the delete event
	return true;
}

} // namespace ui
