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

#include "SpawnargLinkedCheckbox.h"
#include "SpawnargLinkedSpinButton.h"

#include <wx/stattext.h>
#include <wx/sizer.h>
#include <wx/button.h>
#include <wx/artprov.h>

#include <boost/bind.hpp>

namespace ui
{

AIEditingPanel::AIEditingPanel(wxWindow* parent) :
	wxPanel(parent, wxID_ANY),
	_queueUpdate(true),
	_entity(NULL)
{
	Connect(wxEVT_PAINT, wxPaintEventHandler(AIEditingPanel::OnPaint), NULL, this);

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
	SetSizer(new wxBoxSizer(wxVERTICAL));

	wxBoxSizer* vbox = new wxBoxSizer(wxVERTICAL);
	GetSizer()->Add(vbox, 1, wxEXPAND | wxALL, 12);

#if 0
	// Populate the map with all the widgets linked to 1/0 spawnargs
	_checkboxes["canOperateDoors"] = new SpawnargLinkedCheckbox(this, _("Can operate Doors"), "canOperateDoors");
	_checkboxes["canLightTorches"] = new SpawnargLinkedCheckbox(this, _("Can light Torches"), "canLightTorches");
	_checkboxes["canOperateSwitchLights"] = new SpawnargLinkedCheckbox(this, _("Can operate Switch Lights"), "canOperateSwitchLights");
	_checkboxes["canOperateElevators"] = new SpawnargLinkedCheckbox(this, _("Can operate Elevators"), "canOperateElevators");
	_checkboxes["canGreet"] = new SpawnargLinkedCheckbox(this, _("Can greet others"), "canGreet");
	_checkboxes["canSearch"] = new SpawnargLinkedCheckbox(this, _("Can search"), "canSearch");
	_checkboxes["is_civilian"] = new SpawnargLinkedCheckbox(this, _("AI is civilian"), "is_civilian");
	_checkboxes["sleeping"] = new SpawnargLinkedCheckbox(this, _("Start sleeping"), "sleeping");
	_checkboxes["lay_down_left"] = new SpawnargLinkedCheckbox(this, _("Lay down to the left"), "lay_down_left");
	_checkboxes["sitting"] = new SpawnargLinkedCheckbox(this, _("Start sitting"), "sitting");
	_checkboxes["patrol"] = new SpawnargLinkedCheckbox(this, _("Patrol"), "patrol");
	_checkboxes["animal_patrol"] = new SpawnargLinkedCheckbox(this, _("Animal Patrol Mode"), "animal_patrol");
	_checkboxes["alert_idle"] = new SpawnargLinkedCheckbox(this, _("Start in Alert Idle State"), "alert_idle");
	_checkboxes["disable_alert_idle"] = new SpawnargLinkedCheckbox(this, _("Disable Alert Idle State"), "disable_alert_idle");
	_checkboxes["drunk"] = new SpawnargLinkedCheckbox(this, _("Drunk"), "drunk");
	_checkboxes["shoulderable"] = new SpawnargLinkedCheckbox(this, _("Body is shoulderable"), "shoulderable");
	_checkboxes["neverdormant"] = new SpawnargLinkedCheckbox(this, _("AI doesn't think outside the player PVS"), "neverdormant", true); // inverse logic

	_checkboxes["can_drown"] = new SpawnargLinkedCheckbox(this, _("AI can drown"), "can_drown");
	_checkboxes["can_be_flatfooted"] = new SpawnargLinkedCheckbox(this, _("AI can be flatfooted"), "can_be_flatfooted");
	_checkboxes["ko_alert_immune"] = new SpawnargLinkedCheckbox(this, _("AI is immune to KOs at high alert levels"), "ko_alert_immune");
	_checkboxes["ko_immune"] = new SpawnargLinkedCheckbox(this, _("AI is immune to KOs"), "ko_immune");
	_checkboxes["gas_immune"] = new SpawnargLinkedCheckbox(this, _("AI is immune to Gas"), "gas_immune");

	_spinButtons["team"] = new SpawnargLinkedSpinButton(this, _("Team"), "team", 0, 99, 1, 0);
	_spinButtons["sit_down_angle"] = new SpawnargLinkedSpinButton(this, _("Sitting Angle"), "sit_down_angle", -179, 180, 1, 0);
	_spinButtons["drunk_acuity_factor"] = new SpawnargLinkedSpinButton(this, _("Drunk Acuity Factor"), "drunk_acuity_factor", 0, 10, 0.1, 2);
	_spinButtons["acuity_vis"] = new SpawnargLinkedSpinButton(this, _("Visual Acuity"), "acuity_vis", 0, 1, 0.01, 2);
	_spinButtons["acuity_aud"] = new SpawnargLinkedSpinButton(this, _("Audio Acuity"), "acuity_aud", 0, 1, 0.01, 2);

	_spinButtons["fov"] = new SpawnargLinkedSpinButton(this, _("Horizontal FOV"), "fov", 0, 360, 1, 0);
	_spinButtons["fov_vert"] = new SpawnargLinkedSpinButton(this, _("Vertical FOV"), "fov_vert", 0, 180, 1, 0);

	_spinButtons["min_interleave_think_dist"] = new SpawnargLinkedSpinButton(this, _("Min. Interleave Distance"), "min_interleave_think_dist", 0, 60000, 50, 0);
	_spinButtons["max_interleave_think_dist"] = new SpawnargLinkedSpinButton(this, _("Max. Interleave Distance"), "max_interleave_think_dist", 0, 60000, 50, 0);

	_spinButtons["health"] = new SpawnargLinkedSpinButton(this, _("Health"), "health", 0, 1000, 5, 0);
	_spinButtons["health_critical"] = new SpawnargLinkedSpinButton(this, _("Critical Health"), "health_critical", 0, 1000, 5, 0);
	_spinButtons["melee_range"] = new SpawnargLinkedSpinButton(this, _("Melee Range"), "melee_range", 0, 200, 1, 0);
#endif
	{
		// Appearance widgets
		vbox->Add(createSectionLabel(_("Appearance")), wxTOP | wxBOTTOM, 6);

		wxFlexGridSizer* table = new wxFlexGridSizer(2, 3, 6, 12);
		vbox->Add(table, 0, wxEXPAND | wxLEFT, 18);

		createChooserRow(table, _("Skin: "), _("Choose skin..."), "icon_skin.png", "skin");
		createChooserRow(table, _("Head: "), _("Choose AI head..."), "icon_model.png", "def_head");
		createChooserRow(table, _("Vocal Set: "), _("Choose Vocal Set..."), "icon_sound.png", "def_vocal_set");
	}

#if 0
	{
		// Behaviour widgets
		vbox->Add(createSectionLabel(_("Behaviour")));
		
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
		vbox->Add(createSectionLabel(_("Abilities")));
		
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
#endif
}

void AIEditingPanel::createChooserRow(wxSizer* table, const std::string& rowLabel, 
									  const std::string& buttonLabel, const std::string& buttonIcon,
									  const std::string& key)
{
	table->Add(new wxStaticText(this, wxID_ANY, rowLabel));

	_labels[key] = new wxStaticText(this, wxID_ANY, "");
	table->Add(_labels[key]);

	// Create the skin browse button
	wxButton* browseButton = new wxButton(this, wxID_ANY, buttonLabel);
	browseButton->SetBitmap(wxArtProvider::GetBitmap(GlobalUIManager().ArtIdPrefix() + buttonIcon));
	browseButton->Bind(wxEVT_BUTTON, boost::bind(&AIEditingPanel::onBrowseButton, this, _1, key));

	table->Add(browseButton, 0, wxEXPAND);
}

wxStaticText* AIEditingPanel::createSectionLabel(const std::string& text)
{
	wxStaticText* label = new wxStaticText(this, wxID_ANY, text);
	label->SetFont(label->GetFont().Bold());

	return label;
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
	IGroupDialog::PagePtr page(new IGroupDialog::Page);

	page->name = "aieditingpanel";
	page->windowLabel = _("AI");
	page->widget = &(Instance());
	page->tabIcon = "icon_ai.png";
	page->tabLabel = _("AI");
	page->insertBefore = "mediabrowser";

	// Add the Media Browser page
	GlobalGroupDialog().addWxPage(page);

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
	Enable(_entity != NULL);
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
	_checkboxes["lay_down_left"]->Enable(_checkboxes["sleeping"]->GetValue());
	_spinButtons["sit_down_angle"]->Enable(_checkboxes["sitting"]->GetValue());
	_spinButtons["drunk_acuity_factor"]->Enable(_checkboxes["drunk"]->GetValue());

	std::for_each(_labels.begin(), _labels.end(), [&] (LabelMap::value_type& pair)
	{
		pair.second->SetLabelText(_entity != NULL ? _entity->getKeyValue(pair.first) : "");
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

	if (GlobalGroupDialog().getWxPage() == this)
	{
		rescanSelection();
	}
	else
	{
		// We don't scan the selection if the page is not visible
		_queueUpdate = true;
	}
}

void AIEditingPanel::OnPaint(wxPaintEvent& ev)
{
	if (_queueUpdate)
	{
		// Wake up from sleep mode and inspect the current selection
		rescanSelection();
	}

	ev.Skip();
}

void AIEditingPanel::onBrowseButton(wxCommandEvent& ev, const std::string& key)
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
