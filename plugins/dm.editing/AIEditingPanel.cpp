#include "AIEditingPanel.h"

#include "i18n.h"
#include "iselection.h"
#include "ieclass.h"
#include "itextstream.h"
#include "ientityinspector.h"
#include "iradiant.h"
#include "igroupdialog.h"
#include "iuimanager.h"
#include "selectionlib.h"

#include <gtkmm/button.h>
#include <gtkmm/table.h>
#include <gtkmm/image.h>
#include "gtkutil/LeftAlignedLabel.h"
#include "gtkutil/LeftAlignment.h"
#include "gtkutil/ScrolledFrame.h"

#include "SpawnargLinkedCheckbox.h"
#include "SpawnargLinkedSpinButton.h"

namespace ui
{

AIEditingPanel::AIEditingPanel() :
	_queueUpdate(true),
	_vbox(NULL),
	_entity(NULL)
{
	constructWidgets();

	GlobalRadiant().signal_radiantShutdown().connect(
		sigc::mem_fun(*this, &AIEditingPanel::onRadiantShutdown)
	);	

	_selectionChangedSignal = GlobalSelectionSystem().signal_selectionChanged().connect(
		sigc::mem_fun(*this, &AIEditingPanel::onSelectionChanged)
	);
}

void AIEditingPanel::constructWidgets()
{
	_vbox = Gtk::manage(new Gtk::VBox);

	gtkutil::ScrolledFrame* frame = Gtk::manage(new gtkutil::ScrolledFrame(*_vbox));
	frame->set_shadow_type(Gtk::SHADOW_NONE);

	this->pack_start(*frame, true, true, 0);

	_vbox->set_border_width(12);
	_vbox->set_spacing(6);

	// Populate the map with all the widgets linked to 1/0 spawnargs
	_checkboxes["canOperateDoors"] = Gtk::manage(new SpawnargLinkedCheckbox(_("Can operate Doors"), "canOperateDoors"));
	_checkboxes["canLightTorches"] = Gtk::manage(new SpawnargLinkedCheckbox(_("Can light Torches"), "canLightTorches"));
	_checkboxes["canOperateSwitchLights"] = Gtk::manage(new SpawnargLinkedCheckbox(_("Can operate Switch Lights"), "canOperateSwitchLights"));
	_checkboxes["canOperateElevators"] = Gtk::manage(new SpawnargLinkedCheckbox(_("Can operate Elevators"), "canOperateElevators"));
	_checkboxes["canGreet"] = Gtk::manage(new SpawnargLinkedCheckbox(_("Can greet others"), "canGreet"));
	_checkboxes["canSearch"] = Gtk::manage(new SpawnargLinkedCheckbox(_("Can search"), "canSearch"));
	_checkboxes["is_civilian"] = Gtk::manage(new SpawnargLinkedCheckbox(_("AI is civilian"), "is_civilian"));
	_checkboxes["sleeping"] = Gtk::manage(new SpawnargLinkedCheckbox(_("Start sleeping"), "sleeping"));
	_checkboxes["lay_down_left"] = Gtk::manage(new SpawnargLinkedCheckbox(_("Lay down to the left"), "lay_down_left"));
	_checkboxes["sitting"] = Gtk::manage(new SpawnargLinkedCheckbox(_("Start sitting"), "sitting"));
	_checkboxes["patrol"] = Gtk::manage(new SpawnargLinkedCheckbox(_("Patrol"), "patrol"));
	_checkboxes["animal_patrol"] = Gtk::manage(new SpawnargLinkedCheckbox(_("Animal Patrol Mode"), "animal_patrol"));
	_checkboxes["alert_idle"] = Gtk::manage(new SpawnargLinkedCheckbox(_("Start in Alert Idle State"), "alert_idle"));
	_checkboxes["disable_alert_idle"] = Gtk::manage(new SpawnargLinkedCheckbox(_("Disable Alert Idle State"), "disable_alert_idle"));
	_checkboxes["drunk"] = Gtk::manage(new SpawnargLinkedCheckbox(_("Drunk"), "drunk"));
	_checkboxes["shoulderable"] = Gtk::manage(new SpawnargLinkedCheckbox(_("Body is shoulderable"), "shoulderable"));
	_checkboxes["neverdormant"] = Gtk::manage(new SpawnargLinkedCheckbox(_("AI doesn't think outside the player PVS"), "neverdormant", true)); // inverse logic

	_checkboxes["can_drown"] = Gtk::manage(new SpawnargLinkedCheckbox(_("AI can drown"), "can_drown"));
	_checkboxes["can_be_flatfooted"] = Gtk::manage(new SpawnargLinkedCheckbox(_("AI can be flatfooted"), "can_be_flatfooted"));
	_checkboxes["ko_alert_immune"] = Gtk::manage(new SpawnargLinkedCheckbox(_("AI is immune to KOs at high alert levels"), "ko_alert_immune"));
	_checkboxes["ko_immune"] = Gtk::manage(new SpawnargLinkedCheckbox(_("AI is immune to KOs"), "ko_immune"));
	_checkboxes["gas_immune"] = Gtk::manage(new SpawnargLinkedCheckbox(_("AI is immune to Gas"), "gas_immune"));

	_spinButtons["team"] = Gtk::manage(new SpawnargLinkedSpinButton(_("Team"), "team", 0, 99, 1, 0));
	_spinButtons["sit_down_angle"] = Gtk::manage(new SpawnargLinkedSpinButton(_("Sitting Angle"), "sit_down_angle", -179, 180, 1, 0));
	_spinButtons["drunk_acuity_factor"] = Gtk::manage(new SpawnargLinkedSpinButton(_("Drunk Acuity Factor"), "drunk_acuity_factor", 0, 10, 0.1, 2));
	_spinButtons["acuity_vis"] = Gtk::manage(new SpawnargLinkedSpinButton(_("Visual Acuity"), "acuity_vis", 0, 1, 0.01, 2));
	_spinButtons["acuity_aud"] = Gtk::manage(new SpawnargLinkedSpinButton(_("Audio Acuity"), "acuity_aud", 0, 1, 0.01, 2));

	_spinButtons["fov"] = Gtk::manage(new SpawnargLinkedSpinButton(_("Horizontal FOV"), "fov", 0, 360, 1, 0));
	_spinButtons["fov_vert"] = Gtk::manage(new SpawnargLinkedSpinButton(_("Vertical FOV"), "fov_vert", 0, 180, 1, 0));

	_spinButtons["min_interleave_think_dist"] = Gtk::manage(new SpawnargLinkedSpinButton(_("Min. Interleave Distance"), "min_interleave_think_dist", 0, 60000, 50, 0));
	_spinButtons["max_interleave_think_dist"] = Gtk::manage(new SpawnargLinkedSpinButton(_("Max. Interleave Distance"), "max_interleave_think_dist", 0, 60000, 50, 0));

	_spinButtons["health"] = Gtk::manage(new SpawnargLinkedSpinButton(_("Health"), "health", 0, 1000, 5, 0));
	_spinButtons["health_critical"] = Gtk::manage(new SpawnargLinkedSpinButton(_("Critical Health"), "health_critical", 0, 1000, 5, 0));
	_spinButtons["melee_range"] = Gtk::manage(new SpawnargLinkedSpinButton(_("Melee Range"), "melee_range", 0, 200, 1, 0));

	{
		// Appearance widgets
		_vbox->pack_start(*Gtk::manage(new gtkutil::LeftAlignedLabel(
			std::string("<b>") + _("Appearance") + "</b>")), false, false, 0);

		// Skin
		Gtk::HBox* skinRow = Gtk::manage(new Gtk::HBox(false, 6));

		skinRow->pack_start(*Gtk::manage(new gtkutil::LeftAlignedLabel(_("Skin: "))), false, false, 0);

		_labels["skin"] = Gtk::manage(new gtkutil::LeftAlignedLabel(""));
		skinRow->pack_start(*_labels["skin"], true, true, 0);

		// Create the skin browse button
		Gtk::Button* browseButton = Gtk::manage(new Gtk::Button(_("Choose skin...")));
		browseButton->set_image(*Gtk::manage(new Gtk::Image(GlobalUIManager().getLocalPixbuf("icon_skin.png"))));
		browseButton->signal_clicked().connect(sigc::bind(sigc::mem_fun(*this, &AIEditingPanel::onBrowseButton), "skin"));
		skinRow->pack_start(*browseButton, false, false, 0);

		_vbox->pack_start(*Gtk::manage(new gtkutil::LeftAlignment(*skinRow, 18, 1.0)), false, false, 0);

		// Head
		Gtk::HBox* headRow = Gtk::manage(new Gtk::HBox(false, 6));

		headRow->pack_start(*Gtk::manage(new gtkutil::LeftAlignedLabel(_("Head: "))), false, false, 0);

		_labels["def_head"] = Gtk::manage(new gtkutil::LeftAlignedLabel(""));
		headRow->pack_start(*_labels["def_head"], true, true, 0);

		// Create the head browse browse button
		Gtk::Button* headBrowseButton = Gtk::manage(new Gtk::Button(_("Choose AI head...")));
		headBrowseButton->set_image(*Gtk::manage(new Gtk::Image(GlobalUIManager().getLocalPixbuf("icon_model.png"))));
		headBrowseButton->signal_clicked().connect(sigc::bind(sigc::mem_fun(*this, &AIEditingPanel::onBrowseButton), "def_head"));
		headRow->pack_start(*headBrowseButton, false, false, 0);

		_vbox->pack_start(*Gtk::manage(new gtkutil::LeftAlignment(*headRow, 18, 1.0)), false, false, 0);

		// Vocal Set
		Gtk::HBox* vocalSetRow = Gtk::manage(new Gtk::HBox(false, 6));

		vocalSetRow->pack_start(*Gtk::manage(new gtkutil::LeftAlignedLabel(_("Vocal Set: "))), false, false, 0);
		_labels["def_vocal_set"] = Gtk::manage(new gtkutil::LeftAlignedLabel(""));
		vocalSetRow->pack_start(*_labels["def_vocal_set"], true, true, 0);

		// Create the skin browse button
		Gtk::Button* vocalSetBrowseButton = Gtk::manage(new Gtk::Button(_("Choose Vocal Set...")));
		vocalSetBrowseButton->set_image(*Gtk::manage(new Gtk::Image(GlobalUIManager().getLocalPixbuf("icon_sound.png"))));
		vocalSetBrowseButton->signal_clicked().connect(sigc::bind(sigc::mem_fun(*this, &AIEditingPanel::onBrowseButton), "def_vocal_set"));
		vocalSetRow->pack_start(*vocalSetBrowseButton, false, false, 0);

		_vbox->pack_start(*Gtk::manage(new gtkutil::LeftAlignment(*vocalSetRow, 18, 1.0)), false, false, 0);
	}

	{
		// Behaviour widgets
		_vbox->pack_start(*Gtk::manage(new gtkutil::LeftAlignedLabel(
			std::string("<b>") + _("Behaviour") + "</b>")), false, false, 0);
		
		Gtk::Table* table = Gtk::manage(new Gtk::Table(9, 2, false));

		table->set_col_spacings(6);
		table->set_row_spacings(3);

		// Team
		table->attach(*_spinButtons["team"], 0, 1, 0, 1);
		table->attach(*_checkboxes["is_civilian"], 1, 2, 0, 1);

		// Sitting
		table->attach(*_checkboxes["sitting"], 0, 1, 1, 2);
		table->attach(*_spinButtons["sit_down_angle"], 1, 2, 1, 2);

		// Sleeping
		table->attach(*_checkboxes["sleeping"], 0, 1, 2, 3);
		table->attach(*_checkboxes["lay_down_left"], 1, 2, 2, 3);

		// Patrolling
		table->attach(*_checkboxes["patrol"], 0, 1, 3, 4);
		table->attach(*_checkboxes["animal_patrol"], 1, 2, 3, 4);

		// Acuity
		table->attach(*_spinButtons["acuity_vis"], 0, 1, 4, 5);
		table->attach(*_spinButtons["acuity_aud"], 1, 2, 4, 5);

		// FOV
		table->attach(*_spinButtons["fov"], 0, 1, 5, 6);
		table->attach(*_spinButtons["fov_vert"], 1, 2, 5, 6);

		// Drunk
		table->attach(*_checkboxes["drunk"], 0, 1, 6, 7);
		table->attach(*_spinButtons["drunk_acuity_factor"], 1, 2, 6, 7);

		// Alert Idle Control
		table->attach(*_checkboxes["alert_idle"], 0, 1, 7, 8);
		table->attach(*_checkboxes["disable_alert_idle"], 1, 2, 7, 8);

		table->attach(*_checkboxes["shoulderable"], 0, 1, 8, 9);

		_vbox->pack_start(*Gtk::manage(new gtkutil::LeftAlignment(*table, 18, 0)), false, false, 0);
	}

	{
		// Abilities widgets
		_vbox->pack_start(*Gtk::manage(new gtkutil::LeftAlignedLabel(
			std::string("<b>") + _("Abilities") + "</b>")), false, false, 0);
		
		Gtk::Table* table = Gtk::manage(new Gtk::Table(3, 2, false));
		
		table->set_col_spacings(6);
		table->set_row_spacings(3);

		table->attach(*_checkboxes["canOperateDoors"], 0, 1, 0, 1);
		table->attach(*_checkboxes["canOperateElevators"], 1, 2, 0, 1);
		table->attach(*_checkboxes["canLightTorches"], 0, 1, 1, 2);
		table->attach(*_checkboxes["canOperateSwitchLights"], 1, 2, 1, 2);
		table->attach(*_checkboxes["canGreet"], 0, 1, 2, 3);
		table->attach(*_checkboxes["canSearch"], 1, 2, 2, 3);

		_vbox->pack_start(*Gtk::manage(new gtkutil::LeftAlignment(*table, 18, 0)), false, false, 0);
	}

	{
		// Optimization widgets
		_vbox->pack_start(*Gtk::manage(new gtkutil::LeftAlignedLabel(
			std::string("<b>") + _("Optimization") + "</b>")), false, false, 0);
		
		Gtk::Table* table = Gtk::manage(new Gtk::Table(3, 1, false));
		
		table->set_col_spacings(6);
		table->set_row_spacings(3);

		table->attach(*_checkboxes["neverdormant"], 0, 2, 0, 1);
		table->attach(*_spinButtons["min_interleave_think_dist"], 0, 1, 1, 2);
		table->attach(*_spinButtons["max_interleave_think_dist"], 0, 1, 2, 3);
		
		_vbox->pack_start(*Gtk::manage(new gtkutil::LeftAlignment(*table, 18, 0)), false, false, 0);
	}

	{
		// Health / Combat widgets
		_vbox->pack_start(*Gtk::manage(new gtkutil::LeftAlignedLabel(
			std::string("<b>") + _("Health / Combat") + "</b>")), false, false, 0);
		
		Gtk::Table* table = Gtk::manage(new Gtk::Table(5, 2, false));
		
		table->set_col_spacings(6);
		table->set_row_spacings(3);

		table->attach(*_spinButtons["health"], 0, 1, 0, 1);
		table->attach(*_spinButtons["health_critical"], 1, 2, 0, 1);

		table->attach(*_spinButtons["melee_range"], 0, 1, 1, 2);

		table->attach(*_checkboxes["can_drown"], 0, 1, 2, 3);
		table->attach(*_checkboxes["can_be_flatfooted"], 1, 2, 2, 3);

		table->attach(*_checkboxes["ko_immune"], 0, 1, 3, 4);
		table->attach(*_checkboxes["gas_immune"], 1, 2, 3, 4);

		table->attach(*_checkboxes["ko_alert_immune"], 0, 2, 4, 5);

		_vbox->pack_start(*Gtk::manage(new gtkutil::LeftAlignment(*table, 18, 0)), false, false, 0);
	}

	show_all();
}

AIEditingPanel& AIEditingPanel::Instance()
{
	AIEditingPanelPtr& instance = InstancePtr();

	if (!instance)
	{
		instance.reset(new AIEditingPanel);
	}

	return *instance;
}

void AIEditingPanel::Shutdown()
{
	if (InstancePtr() != NULL)
	{
		//Instance().shutdown();
		InstancePtr().reset();
	}
} 

void AIEditingPanel::onRadiantStartup()
{
	// Add the Media Browser page
	GlobalGroupDialog().addPage(
    	"aieditingpanel",	// name
    	"AI", // tab title
    	"icon_ai.png", // tab icon
    	Instance(), // page widget
    	_("AI"),
		"mediabrowser"
	);

	GlobalUndoSystem().addObserver(InstancePtr().get());
}

void AIEditingPanel::onRadiantShutdown()
{
	GlobalUndoSystem().removeObserver(this);

	_selectionChangedSignal.disconnect();
}

Entity* AIEditingPanel::getEntityFromSelection()
{
	Entity* entity = NULL;

	const SelectionInfo& info = GlobalSelectionSystem().getSelectionInfo();

	if (info.entityCount == 1 && info.totalCount == info.entityCount)
	{
		GlobalSelectionSystem().foreachSelected([&] (const scene::INodePtr& node)
		{
			Entity* candidate = Node_getEntity(node);

			if (candidate->isOfType("atdm:ai_base"))
			{
				entity = candidate;
			}
		});
	}

	return entity;
}

void AIEditingPanel::updatePanelSensitivity()
{
	set_sensitive(_entity != NULL);
}

void AIEditingPanel::onKeyInsert(const std::string& key, EntityKeyValue& value)
{
	// On entity key change, we load all values afresh
	updateWidgetsFromSelection();
}

void AIEditingPanel::onKeyChange(const std::string& key, const std::string& val)
{
	// On entity key change, we load all values afresh
	updateWidgetsFromSelection();
}

void AIEditingPanel::onKeyErase(const std::string& key, EntityKeyValue& value)
{
	// On entity key change, we load all values afresh
	updateWidgetsFromSelection();
}

void AIEditingPanel::postUndo()
{
	updateWidgetsFromSelection();
}

void AIEditingPanel::postRedo()
{
	updateWidgetsFromSelection();
}

void AIEditingPanel::updateWidgetsFromSelection()
{
	// Write the entity object to each checkbox even if it is NULL
	// This will update the widget state from the entity's properties
	std::for_each(_checkboxes.begin(), _checkboxes.end(), [&] (CheckboxMap::value_type& pair)
	{
		pair.second->setEntity(_entity);
	});

	std::for_each(_spinButtons.begin(), _spinButtons.end(), [&] (SpinButtonMap::value_type& pair)
	{
		pair.second->setEntity(_entity);
	});

	// Some dependencies
	_checkboxes["lay_down_left"]->set_sensitive(_checkboxes["sleeping"]->get_active());
	_spinButtons["sit_down_angle"]->set_sensitive(_checkboxes["sitting"]->get_active());
	_spinButtons["drunk_acuity_factor"]->set_sensitive(_checkboxes["drunk"]->get_active());

	std::for_each(_labels.begin(), _labels.end(), [&] (LabelMap::value_type& pair)
	{
		pair.second->set_text(_entity != NULL ? _entity->getKeyValue(pair.first) : "");
	}); 
}

void AIEditingPanel::rescanSelection()
{
	_queueUpdate = false;

	// Load the new entity from the selection
	_entity = getEntityFromSelection();

	if (_entity != NULL)
	{
		_entity->attachObserver(this);
	}

	updatePanelSensitivity();
	updateWidgetsFromSelection();
}

void AIEditingPanel::onSelectionChanged(const Selectable& selectable)
{
	// Immediately disconnect from the current entity in any case
	if (_entity != NULL)
	{
		_entity->detachObserver(this);
	}

	if (GlobalGroupDialog().getPage() == this)
	{
		rescanSelection();
	}
	else
	{
		// We don't scan the selection if the page is not visible
		_queueUpdate = true;
	}
}

bool AIEditingPanel::on_expose_event(GdkEventExpose* event)
{
	if (_queueUpdate)
	{
		// Wake up from sleep mode and inspect the current selection
		rescanSelection();
	}

	// Propagate the call
	return Gtk::VBox::on_expose_event(event);
}

void AIEditingPanel::onBrowseButton(const std::string& key)
{
	if (_entity == NULL) return;

	// Look up the property editor dialog
	IPropertyEditorPtr editor = GlobalEntityInspector().getRegisteredPropertyEditor(key);
	IPropertyEditorDialogPtr dialog = boost::dynamic_pointer_cast<IPropertyEditorDialog>(editor);

	if (dialog)
	{
		std::string oldValue = _entity->getKeyValue(key);
		std::string newValue = dialog->runDialog(_entity, key);

		if (newValue != oldValue)
		{
			UndoableCommand cmd("editAIProperty");
			_entity->setKeyValue(key, newValue);
		}
	}
	else
	{
		rError() << "Could not find a property editor implementing the IPropertyEditorDialog interface for key " 
			<< key << std::endl;
	}
}

AIEditingPanelPtr& AIEditingPanel::InstancePtr()
{
	static AIEditingPanelPtr _instancePtr;
	return _instancePtr;
} 

} // namespace
