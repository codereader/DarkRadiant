#include "LightInspector.h"

#include "i18n.h"
#include "ieventmanager.h"
#include "ientity.h"
#include "ieclass.h"
#include "igame.h"
#include "ishaders.h"
#include "iuimanager.h"
#include "iradiant.h"
#include "imainframe.h"

#include "registry/registry.h"
#include "gtkutil/IconTextButton.h"
#include "gtkutil/LeftAlignedLabel.h"
#include "gtkutil/RightAlignment.h"
#include "gtkutil/LeftAlignment.h"
#include "gtkutil/dialog/MessageBox.h"
#include "gtkutil/MultiMonitor.h"

#include "ui/common/ShaderChooser.h" // for static displayLightInfo() function

#include <gtkmm/box.h>
#include <gtkmm/colorbutton.h>
#include <gtkmm/checkbutton.h>
#include <gtkmm/separator.h>
#include <gtkmm/stock.h>
#include <boost/algorithm/string/predicate.hpp>

namespace ui
{

/* CONSTANTS */

namespace
{
	const char* LIGHTINSPECTOR_TITLE = N_("Light properties");

	const char* PARALLEL_TEXT = N_("Parallel");
	const char* NOSHADOW_TEXT = N_("Do not cast shadows (fast)");
	const char* NOSPECULAR_TEXT = N_("Skip specular lighting");
	const char* NODIFFUSE_TEXT = N_("Skip diffuse lighting");

	const std::string RKEY_WINDOW_STATE = "user/ui/lightInspector/window";
	const std::string RKEY_INSTANT_APPLY = "user/ui/lightInspector/instantApply";

	const char* LIGHT_PREFIX_XPATH = "/light/texture//prefix";

	/** greebo: Loads the prefixes from the registry and creates a
	 * 			comma-separated list string
	 */
	inline std::string getPrefixList() {
		std::string prefixes;

		// Get the list of light texture prefixes from the registry
		xml::NodeList prefList = GlobalGameManager().currentGame()->getLocalXPath(LIGHT_PREFIX_XPATH);

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
: gtkutil::PersistentTransientWindow(_(LIGHTINSPECTOR_TITLE), GlobalMainFrame().getTopLevelWindow(), true),
  _isProjected(false),
  _texSelector(Gtk::manage(new ShaderSelector(this, getPrefixList(), true))),
  _updateActive(false)
{
	set_type_hint(Gdk::WINDOW_TYPE_HINT_DIALOG);

    // Window size
	Gdk::Rectangle rect = gtkutil::MultiMonitor::getMonitorForWindow(GlobalMainFrame().getTopLevelWindow());
	set_default_size(static_cast<int>(rect.get_width()/2), -1);

	// Left-hand panels (volume, colour, options)
	Gtk::VBox* panels = Gtk::manage(new Gtk::VBox(false, 12));

	panels->pack_start(
		*Gtk::manage(new gtkutil::LeftAlignedLabel(std::string("<b>") + _("Light volume") + "</b>")),
		false, false, 0);

	// Volume type hbox
	Gtk::HBox* typeBox = Gtk::manage(new Gtk::HBox(false, 12));

	typeBox->pack_start(createPointLightPanel(), false, false, 0);
	typeBox->pack_start(*Gtk::manage(new Gtk::VSeparator()), false, false, 0);
	typeBox->pack_start(createProjectedPanel(), false, false, 0);

	panels->pack_start(*Gtk::manage(new gtkutil::LeftAlignment(*typeBox, 12)), false, false, 0);

	// Light colour
	_colour = Gtk::manage(new Gtk::ColorButton);
	_colour->signal_color_set().connect(sigc::mem_fun(*this, &LightInspector::_onColourChange));

	panels->pack_start(*Gtk::manage(new gtkutil::LeftAlignedLabel(std::string("<b>") + _("Colour") + "</b>")),
					   false, false, 0);
	panels->pack_start(*Gtk::manage(new gtkutil::LeftAlignment(*_colour, 12, 0.0)),
					   false, false, 0);

	// Options panel
	panels->pack_start(*Gtk::manage(new gtkutil::LeftAlignedLabel(std::string("<b>") + _("Options") + "</b>")),
					   false, false, 0);
	panels->pack_start(*Gtk::manage(new gtkutil::LeftAlignment(createOptionsPanel(), 12)),
					   false, false, 0);

	Gtk::HBox* hbx = Gtk::manage(new Gtk::HBox(false, 18));
	hbx->pack_start(*panels, false, false, 0);
	hbx->pack_start(createTextureWidgets(), true, true, 0);

	_mainVBox = Gtk::manage(new Gtk::VBox(false, 12));
	_mainVBox->pack_start(*hbx, true, true, 0);

	// Create an apply button, if instant-apply is disabled
	if (!registry::getValue<bool>(RKEY_INSTANT_APPLY))
	{
		_mainVBox->pack_start(*Gtk::manage(new Gtk::HSeparator), false, false, 0);
		_mainVBox->pack_end(createButtons(), false, false, 0);
	}

	set_border_width(12);
	add(*_mainVBox);

	// Propagate shortcuts that are not processed by this window
	GlobalEventManager().connectDialogWindow(this);

	// Connect the window position tracker
	_windowPosition.loadFromPath(RKEY_WINDOW_STATE);

	_windowPosition.connect(this);
	_windowPosition.applyPosition();
}

LightInspectorPtr& LightInspector::InstancePtr()
{
	static LightInspectorPtr _instancePtr;
	return _instancePtr;
}

void LightInspector::onRadiantShutdown()
{
	// Hide the window, if we're visible
	if (is_visible())
	{
		hide();
	}

	// Tell the position tracker to save the information
	_windowPosition.saveToPath(RKEY_WINDOW_STATE);

	GlobalSelectionSystem().removeObserver(this);
	GlobalEventManager().disconnectDialogWindow(this);

	// Destroy the window
	destroy();

	// Free the shared ptr
	InstancePtr().reset();
}

void LightInspector::shaderSelectionChanged(
	const std::string& shader,
	const Glib::RefPtr<Gtk::ListStore>& listStore)
{
	// Get the shader, and its image map if possible
	MaterialPtr ishader = _texSelector->getSelectedShader();
	// Pass the call to the static member of ShaderSelector
	ShaderSelector::displayLightShaderInfo(ishader, listStore);

	// greebo: Do not write to the entities if this call resulted from an update()
	if (_updateActive) return;

	if (registry::getValue<bool>(RKEY_INSTANT_APPLY))
    {
		std::string commandStr("setLightTexture: ");
		commandStr += _texSelector->getSelection();
		UndoableCommand command(commandStr.c_str());

		// Write the texture key
		setKeyValueAllLights("texture", _texSelector->getSelection());
	}
}

// Create the point light panel
Gtk::Widget& LightInspector::createPointLightPanel()
{
	// Create the point light togglebutton
	_pointLightToggle = Gtk::manage(new gtkutil::IconTextToggleButton(
		_("Omni"), GlobalUIManager().getLocalPixbuf("pointLight32.png")
	));
	_pointLightToggle->signal_toggled().connect(sigc::mem_fun(*this, &LightInspector::_onPointToggle));

	// Pack button into box to stop it expanding vertically
	Gtk::VBox* buttonBox = Gtk::manage(new Gtk::VBox(false, 0));

	buttonBox->pack_start(*_pointLightToggle, false, false, 0);

	return *buttonBox;
}

// Create the projected light panel
Gtk::Widget& LightInspector::createProjectedPanel()
{
	// Create the projected light togglebutton
	_projLightToggle = Gtk::manage(new gtkutil::IconTextToggleButton(
		_("Projected"), GlobalUIManager().getLocalPixbuf("projLight32.png")
	));
	_projLightToggle->signal_toggled().connect(sigc::mem_fun(*this, &LightInspector::_onProjToggle));

	// Start/end checkbox
	_useStartEnd = Gtk::manage(new Gtk::CheckButton(_("Use start/end")));
	_useStartEnd->signal_toggled().connect(sigc::mem_fun(*this, &LightInspector::_onOptionsToggle));

	// VBox for panel
	Gtk::VBox* vbx = Gtk::manage(new Gtk::VBox(false, 12));

	vbx->pack_start(*Gtk::manage(new gtkutil::LeftAlignment(*_projLightToggle)), false, false, 0);
	vbx->pack_start(*_useStartEnd, false, false, 0);

	return *vbx;
}

// Create the options checkboxes
Gtk::Widget& LightInspector::createOptionsPanel()
{
	// Add options boxes to map
	_options["parallel"] = Gtk::manage(new Gtk::CheckButton(_(PARALLEL_TEXT)));
	_options["noshadows"] = Gtk::manage(new Gtk::CheckButton(_(NOSHADOW_TEXT)));
	_options["nospecular"] = Gtk::manage(new Gtk::CheckButton(_(NOSPECULAR_TEXT)));
	_options["nodiffuse"] = Gtk::manage(new Gtk::CheckButton(_(NODIFFUSE_TEXT)));

	_options["parallel"]->signal_toggled().connect(sigc::mem_fun(*this, &LightInspector::_onOptionsToggle));
	_options["noshadows"]->signal_toggled().connect(sigc::mem_fun(*this, &LightInspector::_onOptionsToggle));
	_options["nospecular"]->signal_toggled().connect(sigc::mem_fun(*this, &LightInspector::_onOptionsToggle));
	_options["nodiffuse"]->signal_toggled().connect(sigc::mem_fun(*this, &LightInspector::_onOptionsToggle));

	// Pack checkboxes into a VBox
	Gtk::VBox* vbx = Gtk::manage(new Gtk::VBox(false, 6));

	vbx->pack_start(*_options["parallel"], false, false, 0);
	vbx->pack_start(*_options["noshadows"], false, false, 0);
	vbx->pack_start(*_options["nospecular"], false, false, 0);
	vbx->pack_start(*_options["nodiffuse"], false, false, 0);

	return *vbx;
}

// Create the texture widgets
Gtk::Widget& LightInspector::createTextureWidgets()
{
	// VBox contains texture selection widgets
	Gtk::VBox* vbx = Gtk::manage(new Gtk::VBox(false, 12));

	vbx->pack_start(
		*Gtk::manage(new gtkutil::LeftAlignedLabel(std::string("<b>") + _("Light Texture") + "</b>")),
		false, false, 0);

	vbx->pack_start(*Gtk::manage(new gtkutil::LeftAlignment(*_texSelector, 12, 1.0)), true, true, 0);

	return *vbx;
}

// Create the buttons
Gtk::Widget& LightInspector::createButtons()
{
	Gtk::HBox* hbx = Gtk::manage(new Gtk::HBox(true, 6));

	Gtk::Button* okButton = Gtk::manage(new Gtk::Button(Gtk::Stock::APPLY));

	okButton->signal_clicked().connect(sigc::mem_fun(*this, &LightInspector::_onOK));

	hbx->pack_end(*okButton, true, true, 0);

	return *Gtk::manage(new gtkutil::RightAlignment(*hbx));
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
        _mainVBox->set_sensitive(true);
	}
    else
    {
        // Nothing found, disable the dialog
		_mainVBox->set_sensitive(false);
    }
}

void LightInspector::postUndo()
{
	// Update the LightInspector after an undo operation
	update();
}

void LightInspector::postRedo()
{
	// Update the LightInspector after a redo operation
	update();
}

// Pre-hide callback
void LightInspector::_preHide()
{
	// Remove as observer, an invisible inspector doesn't need to receive events
	GlobalSelectionSystem().removeObserver(this);
	GlobalUndoSystem().removeObserver(this);

	// Save the window position, to make sure
	_windowPosition.readPosition();

    // Explicitly hide the GL widget otherwise it will no longer be drawn
    // after re-showing on Windows.
    _texSelector->hide();
}

// Pre-show callback
void LightInspector::_preShow()
{
	// Register self as observer to receive events
	GlobalUndoSystem().addObserver(this);
	GlobalSelectionSystem().addObserver(this);

	// Restore the position
	_windowPosition.applyPosition();

	// Update the widgets before showing
	update();
}

void LightInspector::selectionChanged(const scene::INodePtr& node, bool isComponent)
{
	update();
}

// Static method to toggle the dialog
void LightInspector::toggleInspector(const cmd::ArgumentList& args)
{
	// Toggle the instance
	Instance().toggleVisibility();
}

LightInspector& LightInspector::Instance()
{
	LightInspectorPtr& instancePtr = InstancePtr();

	if (instancePtr == NULL)
	{
		// Not yet instantiated, do it now
		instancePtr.reset(new LightInspector);

		// Register this instance with GlobalRadiant() at once
		GlobalRadiant().signal_radiantShutdown().connect(
            sigc::mem_fun(*instancePtr, &LightInspector::onRadiantShutdown)
        );
	}

	return *instancePtr;
}

/* GTK CALLBACKS */

void LightInspector::_onProjToggle()
{
	if (_updateActive) return; // avoid callback loops

	// Set the projected flag
	_isProjected = _projLightToggle->get_active();

	// Set button state based on the value of the flag
	_pointLightToggle->set_active(!_isProjected);
	_useStartEnd->set_sensitive(_isProjected);

	if (registry::getValue<bool>(RKEY_INSTANT_APPLY))
	{
		writeToAllEntities();
	}
}

void LightInspector::_onPointToggle()
{
	if (_updateActive) return; // avoid callback loops

	// Set the projected flag
	_isProjected = !_pointLightToggle->get_active();

	// Set button state based on the value of the flag
	_projLightToggle->set_active(_isProjected);
	_useStartEnd->set_sensitive(_isProjected);

	if (registry::getValue<bool>(RKEY_INSTANT_APPLY))
	{
		writeToAllEntities();
	}
}

void LightInspector::_onOK()
{
	writeToAllEntities();
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
    {
		colString = "1.0 1.0 1.0";
    }

	Vector3 colour = string::convert<Vector3>(colString);
	Gdk::Color col;
	col.set_rgb_p(colour.x(), colour.y(), colour.z());
	_colour->set_color(col);

	// Set the texture selection from the "texture" key
	_texSelector->setSelection(entity->getKeyValue("texture"));

	// Determine whether this is a projected light, and set the toggles
	// appropriately
	_isProjected = (!entity->getKeyValue("light_target").empty() &&
					!entity->getKeyValue("light_right").empty() &&
					!entity->getKeyValue("light_up").empty());

	_projLightToggle->set_active(_isProjected);
	_pointLightToggle->set_active(!_isProjected);

	// If this entity has light_start and light_end keys, set the checkbox
	if (!entity->getKeyValue("light_start").empty()
		&& !entity->getKeyValue("light_end").empty())
	{
		_useStartEnd->set_active(true);
	}
	else
	{
		_useStartEnd->set_active(false);
	}

	// Set the options checkboxes
	for (WidgetMap::iterator i = _options.begin(); i != _options.end(); ++i)
	{
		i->second->set_active(entity->getKeyValue(i->first) == "1");
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
	Gdk::Color col = _colour->get_color();

	entity->setKeyValue("_color", (boost::format("%.2f %.2f %.2f")
							  		% (col.get_red_p())
							  		% (col.get_green_p())
							  		% (col.get_blue_p())).str());

	// Write out all vectors to the entity
	for (StringMap::iterator i = _valueMap.begin();
		 i != _valueMap.end();
		 ++i)
	{
		entity->setKeyValue(i->first, i->second);
	}

	// Remove vector keys that should not exist, depending on the lightvolume
	// options
	if (_isProjected)
	{
		// Clear start/end vectors if checkbox is disabled
		if (!_useStartEnd->get_active())
		{
			entity->setKeyValue("light_start", "");
			entity->setKeyValue("light_end", "");
		}

		// Blank out pointlight values
		entity->setKeyValue("light_radius", "");
		entity->setKeyValue("light_center", "");
	}
	else
	{
		// Blank out projected light values
		entity->setKeyValue("light_target", "");
		entity->setKeyValue("light_right", "");
		entity->setKeyValue("light_up", "");
		entity->setKeyValue("light_start", "");
		entity->setKeyValue("light_end", "");
	}

	// Write the texture key
	entity->setKeyValue("texture", _texSelector->getSelection());

	// Write the options
	for (WidgetMap::iterator i = _options.begin(); i != _options.end(); ++i)
	{
		entity->setKeyValue(i->first, i->second->get_active() ? "1" : "0");
	}
}

void LightInspector::_onOptionsToggle()
{
	if (_updateActive) return; // avoid callback loops

	if (registry::getValue<bool>(RKEY_INSTANT_APPLY))
	{
		writeToAllEntities();
	}
}

void LightInspector::_onColourChange()
{
	if (_updateActive) return; // avoid callback loops

	if (registry::getValue<bool>(RKEY_INSTANT_APPLY))
	{
		writeToAllEntities();
	}
}

} // namespace ui
