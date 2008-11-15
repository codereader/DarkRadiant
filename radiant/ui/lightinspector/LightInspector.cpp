#include "LightInspector.h"

#include "ieventmanager.h"
#include "ientity.h"
#include "ieclass.h"
#include "ishaders.h"
#include "iregistry.h"
#include "iundo.h"

#include "scenelib.h"
#include "mainframe.h"
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
	
	const char* PARALLEL_TEXT = "Parallel";
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
LightInspector::LightInspector() 
: gtkutil::PersistentTransientWindow(LIGHTINSPECTOR_TITLE, MainFrame_getWindow(), true),
  _isProjected(false),
  _texSelector(this, getPrefixList(), true),
  _updateActive(false)
{
	gtk_window_set_type_hint(GTK_WINDOW(getWindow()), GDK_WINDOW_TYPE_HINT_DIALOG);
	
    // Window size
	GdkScreen* scr = gtk_window_get_screen(GTK_WINDOW(getWindow()));
	gtk_window_set_default_size(GTK_WINDOW(getWindow()), 
								gint(gdk_screen_get_width(scr) * 0.5), 
								-1);
    
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

	gtk_container_set_border_width(GTK_CONTAINER(getWindow()), 12);
	gtk_container_add(GTK_CONTAINER(getWindow()), _mainVBox);
	
	// Register to get notified upon selection change
	GlobalSelectionSystem().addObserver(this);
	
	// Propagate shortcuts that are not processed by this window
	GlobalEventManager().connectDialogWindow(GTK_WINDOW(getWindow()));
	
	// Connect the window position tracker
	xml::NodeList windowStateList = GlobalRegistry().findXPath(RKEY_WINDOW_STATE);
	
	if (windowStateList.size() > 0) {
		_windowPosition.loadFromNode(windowStateList[0]);
	}
	
	_windowPosition.connect(GTK_WINDOW(getWindow()));
	_windowPosition.applyPosition();
}

LightInspectorPtr& LightInspector::InstancePtr() {
	static LightInspectorPtr _instancePtr;
	
	if (_instancePtr == NULL) {
		// Not yet instantiated, do it now
		_instancePtr = LightInspectorPtr(new LightInspector);
		
		// Register this instance with GlobalRadiant() at once
		GlobalRadiant().addEventListener(_instancePtr);
	}
	
	return _instancePtr;
}

void LightInspector::onRadiantShutdown() {

	// Delete all the current window states from the registry  
	GlobalRegistry().deleteXPath(RKEY_WINDOW_STATE);
	
	// Create a new node
	xml::Node node(GlobalRegistry().createKey(RKEY_WINDOW_STATE));
	
	// Tell the position tracker to save the information
	_windowPosition.saveToNode(node);
	
	GlobalSelectionSystem().removeObserver(this);
	GlobalEventManager().disconnectDialogWindow(GTK_WINDOW(getWindow()));
	
	// Destroy the window
	destroy();
}

void LightInspector::shaderSelectionChanged(
	const std::string& shader, 
	GtkListStore* listStore) 
{
	// Get the shader, and its image map if possible
	IShaderPtr ishader = _texSelector.getSelectedShader();
	// Pass the call to the static member of ShaderSelector
	ShaderSelector::displayLightShaderInfo(ishader, listStore);
	
	// greebo: Do not write to the entities if this call resulted from an update()
	if (_updateActive) return;

	if (GlobalRegistry().get(RKEY_INSTANT_APPLY) == "1") 
    {
		std::string commandStr("setLightTexture: ");
		commandStr += _texSelector.getSelection();
		UndoableCommand command(commandStr.c_str());
	
		// Write the texture key
		setKeyValueAllLights("texture", _texSelector.getSelection());
	}
}

// Create the point light panel
GtkWidget* LightInspector::createPointLightPanel() 
{
	// Create the point light togglebutton
	_pointLightToggle = gtkutil::IconTextButton("Omni", 
		GlobalRadiant().getLocalPixbuf("pointLight32.png"),
		true
	);
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
		GlobalRadiant().getLocalPixbuf("projLight32.png"),
		true
	);
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
	_options["parallel"] = gtk_check_button_new_with_label(PARALLEL_TEXT);
	_options["noshadows"] = gtk_check_button_new_with_label(NOSHADOW_TEXT);
	_options["nospecular"] = gtk_check_button_new_with_label(NOSPECULAR_TEXT);
	_options["nodiffuse"] = gtk_check_button_new_with_label(NODIFFUSE_TEXT);

	g_signal_connect(G_OBJECT(_options["parallel"]), "toggled", G_CALLBACK(_onOptionsToggle), this);
	g_signal_connect(G_OBJECT(_options["noshadows"]), "toggled", G_CALLBACK(_onOptionsToggle), this);
	g_signal_connect(G_OBJECT(_options["nospecular"]), "toggled", G_CALLBACK(_onOptionsToggle), this);
	g_signal_connect(G_OBJECT(_options["nodiffuse"]), "toggled", G_CALLBACK(_onOptionsToggle), this);

	// Pack checkboxes into a VBox
	GtkWidget* vbx = gtk_vbox_new(FALSE, 6);
	gtk_box_pack_start(GTK_BOX(vbx), _options["parallel"], FALSE, FALSE, 0);
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

// Update dialog from map
void LightInspector::update() 
{
	// Clear the list of light entities
	_lightEntities.clear();
	
    // Find all selected objects which are lights, and add them to our list of
    // light entities

    class LightEntityFinder
    : public SelectionSystem::Visitor
    {
        // List of light entities to add to
        EntityList& _entityList;

    public:
        
        // Constructor initialises entity list
        LightEntityFinder(EntityList& list)
        : _entityList(list)
        { }

        // Required visit method
        void visit(const scene::INodePtr& node) const
        {
            Entity* ent = Node_getEntity(node);
            if (ent && ent->getEntityClass()->isLight())
            {
                // Add light to list
                _entityList.push_back(ent);
            }
        }
    };
    LightEntityFinder lightFinder(_lightEntities);

    // Find the selected lights
	GlobalSelectionSystem().foreachSelected(lightFinder);

	if (!_lightEntities.empty()) 
    {
        // Update the dialog with the correct values from the first entity
        getValuesFromEntity();

        // Enable the dialog
        gtk_widget_set_sensitive(_mainVBox, TRUE);
	}
    else
    {
        // Nothing found, disable the dialog
        gtk_widget_set_sensitive(_mainVBox, FALSE);
    }
	
}

// Toggle this dialog
void LightInspector::toggle() {
	if (isVisible())
		hide();
	else
		show();
}

// Pre-hide callback
void LightInspector::_preHide() {
	// Save the window position, to make sure
	_windowPosition.readPosition();
}

// Pre-show callback
void LightInspector::_preShow() {
	// Restore the position
	_windowPosition.applyPosition();
	// Update the widgets
	update();
}

void LightInspector::selectionChanged(const scene::INodePtr& node, bool isComponent) {
	update();
}

// Static method to toggle the dialog
void LightInspector::toggleInspector() {
	// Toggle the instance
	Instance().toggle();
}

LightInspector& LightInspector::Instance() {
	return *InstancePtr();
}

/* GTK CALLBACKS */

void LightInspector::_onProjToggle(GtkWidget* b, LightInspector* self) 
{
	if (self->_updateActive) return; // avoid callback loops
	
	// Set the projected flag
	self->_isProjected = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(b)) ? true : false;
	
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
		self->writeToAllEntities();
	}
}

void LightInspector::_onPointToggle(GtkWidget* b, LightInspector* self) {
	if (self->_updateActive) return; // avoid callback loops
	
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
		self->writeToAllEntities();
	}
}

void LightInspector::_onOK(GtkWidget* w, LightInspector* self) {
	self->writeToAllEntities();
}

// Get keyvals from entity and insert into text entries
void LightInspector::getValuesFromEntity() 
{
	// Disable unwanted callbacks
	_updateActive = true;

    // Read values from the first entity in the list of lights (we have to pick
    // one, so might as well use the first).
    assert(!_lightEntities.empty());
    Entity* entity = _lightEntities[0];

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
		std::string val = entity->getKeyValue(i->first);
		if (!val.empty())
			i->second = val;
	}

	// Get the colour key from the entity to set the GtkColorButton. If the 
	// light has no colour key, use a default of white rather than the Vector3
	// default of black (0, 0, 0).
	std::string colString = entity->getKeyValue("_color");
	if (colString.empty())
		colString = "1.0 1.0 1.0";
		 
	Vector3 colour(colString);
	GdkColor col = { 0,
					 static_cast<guint>(colour.x() * 65535),
					 static_cast<guint>(colour.y() * 65535),
					 static_cast<guint>(colour.z() * 65535) };
	gtk_color_button_set_color(GTK_COLOR_BUTTON(_colour), &col);
	
	// Set the texture selection from the "texture" key
	_texSelector.setSelection(entity->getKeyValue("texture"));
	
	// Determine whether this is a projected light, and set the toggles
	// appropriately
	_isProjected = (!entity->getKeyValue("light_target").empty() && 
					!entity->getKeyValue("light_right").empty() && 
					!entity->getKeyValue("light_up").empty());
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(_projLightToggle), _isProjected);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(_pointLightToggle), !_isProjected);
	
	// If this entity has light_start and light_end keys, set the checkbox
	if (!entity->getKeyValue("light_start").empty()
		&& !entity->getKeyValue("light_end").empty())
	{
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(_useStartEnd), TRUE);
	}
	else {
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(_useStartEnd), FALSE);
	}
	
	// Set the options checkboxes
	for (WidgetMap::iterator i = _options.begin(); i != _options.end(); ++i) {
		if (entity->getKeyValue(i->first) == "1")
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(i->second), TRUE);
		else
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(i->second), FALSE);
	}
	
	_updateActive = false;
}

// Write to all entities
void LightInspector::writeToAllEntities()
{
    for (EntityList::iterator i = _lightEntities.begin();
         i != _lightEntities.end();
         ++i)
    {
        setValuesOnEntity(*i); 
    }
}

// Set a given key value on all light entities
void LightInspector::setKeyValueAllLights(const std::string& key,
                                          const std::string& value)
{
    for (EntityList::iterator i = _lightEntities.begin();
         i != _lightEntities.end();
         ++i)
    {
        (*i)->setKeyValue(key, value);
    }
}

// Set the keyvalues on the entity from the dialog widgets
void LightInspector::setValuesOnEntity(Entity* entity) 
{
	UndoableCommand command("setLightProperties");
	
	// Set the "_color" keyvalue
	GdkColor col;
	gtk_color_button_get_color(GTK_COLOR_BUTTON(_colour), &col);
	entity->setKeyValue("_color", (boost::format("%.2f %.2f %.2f") 
						  	  		% (col.red/65535.0)
						  	  		% (col.green/65535.0)
						  	  		% (col.blue/65535.0)).str());
	
	// Write out all vectors to the entity
	for (StringMap::iterator i = _valueMap.begin();
		 i != _valueMap.end();
		 ++i) 
	{
		entity->setKeyValue(i->first, i->second);
	}
		 	
	// Remove vector keys that should not exist, depending on the lightvolume
	// options
	if (_isProjected) {

		// Clear start/end vectors if checkbox is disabled
		if (!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(_useStartEnd))) {
			entity->setKeyValue("light_start", "");
			entity->setKeyValue("light_end", "");	
		}

		// Blank out pointlight values
		entity->setKeyValue("light_radius", "");
		entity->setKeyValue("light_center", "");
	}
	else {

		// Blank out projected light values
		entity->setKeyValue("light_target", "");
		entity->setKeyValue("light_right", "");
		entity->setKeyValue("light_up", "");
		entity->setKeyValue("light_start", "");
		entity->setKeyValue("light_end", "");
	}

	// Write the texture key
	entity->setKeyValue("texture", _texSelector.getSelection());

	// Write the options
	for (WidgetMap::iterator i = _options.begin(); i != _options.end(); ++i) {
		if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(i->second)))
			entity->setKeyValue(i->first, "1");
		else
			entity->setKeyValue(i->first, "0");
	}
}

void LightInspector::_onOptionsToggle(GtkToggleButton* togglebutton, LightInspector *self) 
{
	if (self->_updateActive) return; // avoid callback loops
	
	if (GlobalRegistry().get(RKEY_INSTANT_APPLY) == "1") {
		self->writeToAllEntities();
	}
}

void LightInspector::_onColourChange(GtkColorButton* widget, LightInspector* self) {
	if (self->_updateActive) return; // avoid callback loops
	
	if (GlobalRegistry().get(RKEY_INSTANT_APPLY) == "1") {
		self->writeToAllEntities();
	}
}

} // namespace ui
