#include "AIEditingPanel.h"

#include "i18n.h"
#include "iselection.h"
#include "ieclass.h"
#include "imap.h"
#include "itextstream.h"
#include "ui/ientityinspector.h"
#include "iundo.h"
#include "selectionlib.h"

#include "SpawnargLinkedCheckbox.h"
#include "SpawnargLinkedSpinButton.h"

#include <wx/stattext.h>
#include <wx/sizer.h>
#include <wx/button.h>
#include "wxutil/Bitmap.h"
#include <wx/panel.h>
#include <wx/scrolwin.h>

#include <functional>

namespace ui
{

AIEditingPanel::AIEditingPanel(wxWindow* parent) :
    DockablePanel(parent),
	_mainPanel(new wxScrolledWindow(this, wxID_ANY)),
	_entity(nullptr),
    _rescanSelectionOnIdle(true)
{
    SetSizer(new wxBoxSizer(wxVERTICAL));
    GetSizer()->Add(_mainPanel, 1, wxEXPAND);

	constructWidgets();
}

AIEditingPanel::~AIEditingPanel()
{
    if (panelIsActive())
    {
        disconnectListeners();
    }
}

void AIEditingPanel::onPanelActivated()
{
    connectListeners();

    _rescanSelectionOnIdle = true;
    requestIdleCallback();
}

void AIEditingPanel::onPanelDeactivated()
{
    disconnectListeners();
}

void AIEditingPanel::connectListeners()
{
    _selectionChangedSignal = GlobalSelectionSystem().signal_selectionChanged().connect(
        sigc::mem_fun(*this, &AIEditingPanel::onSelectionChanged)
    );

    _undoHandler = GlobalMapModule().signal_postUndo().connect(
        sigc::mem_fun(*this, &AIEditingPanel::updateWidgetsFromSelection));
    _redoHandler = GlobalMapModule().signal_postRedo().connect(
        sigc::mem_fun(*this, &AIEditingPanel::updateWidgetsFromSelection));
}

void AIEditingPanel::disconnectListeners()
{
    _undoHandler.disconnect();
    _redoHandler.disconnect();

    _selectionChangedSignal.disconnect();

    if (_entity != nullptr)
    {
        _entity->detachObserver(this);
        _entity = nullptr;
    }
}

void AIEditingPanel::onIdle()
{
    if (_rescanSelectionOnIdle)
    {
        _rescanSelectionOnIdle = false;
        rescanSelection();
    }
}

void AIEditingPanel::constructWidgets()
{
	_mainPanel->SetScrollRate(0, 3);
	_mainPanel->SetSizer(new wxBoxSizer(wxVERTICAL));

	wxBoxSizer* vbox = new wxBoxSizer(wxVERTICAL);
	_mainPanel->GetSizer()->Add(vbox, 1, wxEXPAND | wxALL, 12);

	// Populate the map with all the widgets linked to 1/0 spawnargs
	_checkboxes["canOperateDoors"] = new SpawnargLinkedCheckbox(_mainPanel, _("Can operate Doors"), "canOperateDoors");
	_checkboxes["canLightTorches"] = new SpawnargLinkedCheckbox(_mainPanel, _("Can light Torches"), "canLightTorches");
	_checkboxes["canOperateSwitchLights"] = new SpawnargLinkedCheckbox(_mainPanel, _("Can operate Switch Lights"), "canOperateSwitchLights");
	_checkboxes["canOperateElevators"] = new SpawnargLinkedCheckbox(_mainPanel, _("Can operate Elevators"), "canOperateElevators");
	_checkboxes["canGreet"] = new SpawnargLinkedCheckbox(_mainPanel, _("Can greet others"), "canGreet");
	_checkboxes["canSearch"] = new SpawnargLinkedCheckbox(_mainPanel, _("Can search"), "canSearch");
	_checkboxes["is_civilian"] = new SpawnargLinkedCheckbox(_mainPanel, _("AI is civilian"), "is_civilian");
	_checkboxes["sleeping"] = new SpawnargLinkedCheckbox(_mainPanel, _("Start sleeping"), "sleeping");
	_checkboxes["lay_down_left"] = new SpawnargLinkedCheckbox(_mainPanel, _("Lay down to the left"), "lay_down_left");
	_checkboxes["sitting"] = new SpawnargLinkedCheckbox(_mainPanel, _("Start sitting"), "sitting");
	_checkboxes["patrol"] = new SpawnargLinkedCheckbox(_mainPanel, _("Patrol"), "patrol");
	_checkboxes["animal_patrol"] = new SpawnargLinkedCheckbox(_mainPanel, _("Animal Patrol Mode"), "animal_patrol");
	_checkboxes["alert_idle"] = new SpawnargLinkedCheckbox(_mainPanel, _("Start in Alert Idle State"), "alert_idle");
	_checkboxes["disable_alert_idle"] = new SpawnargLinkedCheckbox(_mainPanel, _("Disable Alert Idle State"), "disable_alert_idle");
	_checkboxes["drunk"] = new SpawnargLinkedCheckbox(_mainPanel, _("Drunk"), "drunk");
	_checkboxes["shoulderable"] = new SpawnargLinkedCheckbox(_mainPanel, _("Body is shoulderable"), "shoulderable");
	_checkboxes["neverdormant"] = new SpawnargLinkedCheckbox(_mainPanel, _("AI doesn't think outside the player PVS"), "neverdormant", true); // inverse logic

	_checkboxes["can_drown"] = new SpawnargLinkedCheckbox(_mainPanel, _("AI can drown"), "can_drown");
	_checkboxes["can_drown"]->setDefaultValueForMissingKeyValue(true);

	_checkboxes["can_be_flatfooted"] = new SpawnargLinkedCheckbox(_mainPanel, _("AI can be flatfooted"), "can_be_flatfooted");
	_checkboxes["ko_alert_immune"] = new SpawnargLinkedCheckbox(_mainPanel, _("AI is immune to KOs at high alert levels"), "ko_alert_immune");
	_checkboxes["ko_immune"] = new SpawnargLinkedCheckbox(_mainPanel, _("AI is immune to KOs"), "ko_immune");
	_checkboxes["gas_immune"] = new SpawnargLinkedCheckbox(_mainPanel, _("AI is immune to Gas"), "gas_immune");

	_spinButtons["team"] = new SpawnargLinkedSpinButton(_mainPanel, _("Team"), "team", 0, 99, 1, 0);
	_spinButtons["rank"] = new SpawnargLinkedSpinButton(_mainPanel, _("Rank"), "rank", 0, 99, 1, 0);
	_spinButtons["sit_down_angle"] = new SpawnargLinkedSpinButton(_mainPanel, _("Sitting Angle"), "sit_down_angle", -179, 180, 1, 0);
	_spinButtons["drunk_acuity_factor"] = new SpawnargLinkedSpinButton(_mainPanel, _("Drunk Acuity Factor"), "drunk_acuity_factor", 0, 10, 0.1, 2);
	_spinButtons["acuity_vis"] = new SpawnargLinkedSpinButton(_mainPanel, _("Visual Acuity"), "acuity_vis", 0, 100, 1, 0);
	_spinButtons["acuity_aud"] = new SpawnargLinkedSpinButton(_mainPanel, _("Audio Acuity"), "acuity_aud", 0, 100, 1, 0);

	_spinButtons["fov"] = new SpawnargLinkedSpinButton(_mainPanel, _("Horizontal FOV"), "fov", 0, 360, 1, 0);
	_spinButtons["fov_vert"] = new SpawnargLinkedSpinButton(_mainPanel, _("Vertical FOV"), "fov_vert", 0, 180, 1, 0);

	_spinButtons["min_interleave_think_dist"] = new SpawnargLinkedSpinButton(_mainPanel, _("Min. Interleave Distance"), "min_interleave_think_dist", 0, 60000, 50, 0);
	_spinButtons["max_interleave_think_dist"] = new SpawnargLinkedSpinButton(_mainPanel, _("Max. Interleave Distance"), "max_interleave_think_dist", 0, 60000, 50, 0);

	_spinButtons["health"] = new SpawnargLinkedSpinButton(_mainPanel, _("Health"), "health", 0, 9999999, 5, 0);
	_spinButtons["health_critical"] = new SpawnargLinkedSpinButton(_mainPanel, _("Critical Health"), "health_critical", 0, 1000, 5, 0);
	_spinButtons["melee_range"] = new SpawnargLinkedSpinButton(_mainPanel, _("Melee Range"), "melee_range", 0, 200, 1, 0);

	{
		// Appearance widgets
		vbox->Add(createSectionLabel(_("Appearance")), 0, wxTOP | wxBOTTOM, 6);

		wxFlexGridSizer* table = new wxFlexGridSizer(3, 3, 4, 12);
		vbox->Add(table, 0, wxEXPAND | wxLEFT, 18);

		createChooserRow(table, _("Skin: "), _("Choose skin..."), "icon_skin.png", "skin");
		createChooserRow(table, _("Head: "), _("Choose AI head..."), "icon_model.png", "def_head");
		createChooserRow(table, _("Vocal Set: "), _("Choose Vocal Set..."), "icon_sound.png", "def_vocal_set");
	}

	{
		// Behaviour widgets
		vbox->Add(createSectionLabel(_("Behaviour")), 0, wxTOP | wxBOTTOM, 6);
		
		wxGridSizer* table = new wxGridSizer(10, 2, 4, 12);
		vbox->Add(table, 0, wxLEFT, 18);

		// Team & Rank
		table->Add(createSpinButtonHbox(_spinButtons["team"]), 0, wxALIGN_CENTER_VERTICAL | wxEXPAND);
		table->Add(createSpinButtonHbox(_spinButtons["rank"]), 0, wxALIGN_CENTER_VERTICAL | wxEXPAND);

		// Civilian flag
		table->Add(_checkboxes["is_civilian"], 0, wxALIGN_CENTER_VERTICAL);
		table->Add(new wxPanel(_mainPanel, wxID_ANY), 0); // empty dummy

		// Sitting
		table->Add(_checkboxes["sitting"], 0, wxALIGN_CENTER_VERTICAL);
		table->Add(createSpinButtonHbox(_spinButtons["sit_down_angle"]), 0, wxALIGN_CENTER_VERTICAL | wxEXPAND);

		// Sleeping
		table->Add(_checkboxes["sleeping"], 0, wxALIGN_CENTER_VERTICAL);
		table->Add(_checkboxes["lay_down_left"], 0, wxALIGN_CENTER_VERTICAL);

		// Patrolling
		table->Add(_checkboxes["patrol"], 0, wxALIGN_CENTER_VERTICAL);
		table->Add(_checkboxes["animal_patrol"], 0, wxALIGN_CENTER_VERTICAL);

		// Acuity
		table->Add(createSpinButtonHbox(_spinButtons["acuity_vis"]), 0, wxALIGN_CENTER_VERTICAL | wxEXPAND);
		table->Add(createSpinButtonHbox(_spinButtons["acuity_aud"]), 0, wxALIGN_CENTER_VERTICAL | wxEXPAND);

		// FOV
		table->Add(createSpinButtonHbox(_spinButtons["fov"]), 0, wxALIGN_CENTER_VERTICAL | wxEXPAND);
		table->Add(createSpinButtonHbox(_spinButtons["fov_vert"]), 0, wxALIGN_CENTER_VERTICAL | wxEXPAND);

		// Drunk
		table->Add(_checkboxes["drunk"], 0, wxALIGN_CENTER_VERTICAL);
		table->Add(createSpinButtonHbox(_spinButtons["drunk_acuity_factor"]), 0, wxALIGN_CENTER_VERTICAL | wxEXPAND);

		// Alert Idle Control
		table->Add(_checkboxes["alert_idle"], 0, wxALIGN_CENTER_VERTICAL);
		table->Add(_checkboxes["disable_alert_idle"], 0, wxALIGN_CENTER_VERTICAL);

		table->Add(_checkboxes["shoulderable"], 0, wxALIGN_CENTER_VERTICAL);
	}

	{
		// Abilities widgets
		vbox->Add(createSectionLabel(_("Abilities")), 0, wxTOP | wxBOTTOM, 6);
		
		wxGridSizer* table = new wxGridSizer(3, 2, 4, 12);
		vbox->Add(table, 0, wxLEFT, 18);

		table->Add(_checkboxes["canOperateDoors"], 0, wxALIGN_CENTER_VERTICAL);
		table->Add(_checkboxes["canOperateElevators"], 0, wxALIGN_CENTER_VERTICAL);
		table->Add(_checkboxes["canLightTorches"], 0, wxALIGN_CENTER_VERTICAL);
		table->Add(_checkboxes["canOperateSwitchLights"], 0, wxALIGN_CENTER_VERTICAL);
		table->Add(_checkboxes["canGreet"], 0, wxALIGN_CENTER_VERTICAL);
		table->Add(_checkboxes["canSearch"], 0, wxALIGN_CENTER_VERTICAL);
	}

	{
		// Optimization widgets
		vbox->Add(createSectionLabel(_("Optimization")), 0, wxTOP | wxBOTTOM, 6);
		
		wxGridSizer* table = new wxGridSizer(3, 1, 4, 12);
		vbox->Add(table, 0, wxLEFT, 18);

		table->Add(_checkboxes["neverdormant"], 0, wxALIGN_CENTER_VERTICAL);
		table->Add(createSpinButtonHbox(_spinButtons["min_interleave_think_dist"]), 0, wxALIGN_CENTER_VERTICAL | wxEXPAND);
		table->Add(createSpinButtonHbox(_spinButtons["max_interleave_think_dist"]), 0, wxALIGN_CENTER_VERTICAL | wxEXPAND);
	}

	{
		// Health / Combat widgets
		vbox->Add(createSectionLabel(_("Health / Combat")), 0, wxTOP | wxBOTTOM, 6);
		
		wxGridSizer* table = new wxGridSizer(5, 2, 4, 12);
		vbox->Add(table, 0, wxLEFT, 18);

		table->Add(createSpinButtonHbox(_spinButtons["health"]), 0, wxALIGN_CENTER_VERTICAL | wxEXPAND);
		table->Add(createSpinButtonHbox(_spinButtons["health_critical"]), 0, wxALIGN_CENTER_VERTICAL | wxEXPAND);

		table->Add(createSpinButtonHbox(_spinButtons["melee_range"]), 0, wxALIGN_CENTER_VERTICAL | wxEXPAND);

		table->Add(_checkboxes["can_drown"], 0, wxALIGN_CENTER_VERTICAL);
		table->Add(_checkboxes["can_be_flatfooted"], 0, wxALIGN_CENTER_VERTICAL);

		table->Add(_checkboxes["ko_immune"], 0, wxALIGN_CENTER_VERTICAL);
		table->Add(_checkboxes["gas_immune"], 0, wxALIGN_CENTER_VERTICAL);

		table->Add(_checkboxes["ko_alert_immune"], 0, wxALIGN_CENTER_VERTICAL);
	}
}

wxSizer* AIEditingPanel::createSpinButtonHbox(SpawnargLinkedSpinButton* spinButton)
{
	wxBoxSizer* hbox = new wxBoxSizer(wxHORIZONTAL);

	wxStaticText* label = new wxStaticText(spinButton->GetParent(), wxID_ANY, spinButton->getLabel() + ":");
	hbox->Add(label, 1, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
	hbox->Add(spinButton, 0, wxALIGN_CENTER_VERTICAL);

	return hbox;
}

void AIEditingPanel::createChooserRow(wxSizer* table, const std::string& rowLabel, 
									  const std::string& buttonLabel, const std::string& buttonIcon,
									  const std::string& key)
{
	table->Add(new wxStaticText(_mainPanel, wxID_ANY, rowLabel), 0, wxALIGN_CENTER_VERTICAL);

	_labels[key] = new wxStaticText(_mainPanel, wxID_ANY, "");
	table->Add(_labels[key], 1, wxALIGN_CENTER_VERTICAL);

	// Create the skin browse button
	wxButton* browseButton = new wxButton(_mainPanel, wxID_ANY, buttonLabel);
	browseButton->SetBitmap(wxutil::GetLocalBitmap(buttonIcon));
	browseButton->Bind(wxEVT_BUTTON, std::bind(&AIEditingPanel::onBrowseButton, this, std::placeholders::_1, key));

	table->Add(browseButton, 0, wxALIGN_RIGHT);
}

wxStaticText* AIEditingPanel::createSectionLabel(const std::string& text)
{
	wxStaticText* label = new wxStaticText(_mainPanel, wxID_ANY, text);
	label->SetFont(label->GetFont().Bold());

	return label;
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

			if (candidate != nullptr && candidate->isOfType("atdm:ai_base"))
			{
				entity = candidate;
			}
		});
	}

	return entity;
}

void AIEditingPanel::updatePanelSensitivity()
{
	_mainPanel->Enable(_entity != nullptr);
	_mainPanel->Layout();
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
		pair.second->SetLabelText(_entity != nullptr ? _entity->getKeyValue(pair.first) : "");
	}); 
}

void AIEditingPanel::rescanSelection()
{
	// Load the new entity from the selection
	auto newEntity = getEntityFromSelection();

    if (newEntity != _entity)
    {
        // Disconnect from any previous entity
        if (_entity != nullptr)
        {
            _entity->detachObserver(this);
            _entity = nullptr; // issue #4282
        }

        if (newEntity != nullptr)
        {
            _entity = newEntity;
            _entity->attachObserver(this);
        }
    }

	updatePanelSensitivity();
	updateWidgetsFromSelection();
}

void AIEditingPanel::onSelectionChanged(const ISelectable&)
{
    _rescanSelectionOnIdle = true;
    requestIdleCallback();
}

void AIEditingPanel::onBrowseButton(wxCommandEvent& ev, const std::string& key)
{
	if (_entity == nullptr) return;

	// Look up the property editor dialog
    auto dialog = GlobalEntityInspector().createDialog(key);

	if (dialog)
	{
		std::string oldValue = _entity->getKeyValue(key);
		std::string newValue = dialog->runDialog(_entity, key);

		if (newValue != oldValue)
		{
			UndoableCommand cmd("editAIProperty");
			_entity->setKeyValue(key, newValue);
			_mainPanel->Layout();
		}
	}
	else
	{
		rError() << "Could not find a property editor implementing the IPropertyEditorDialog interface for key " 
			<< key << std::endl;
	}
}

} // namespace
