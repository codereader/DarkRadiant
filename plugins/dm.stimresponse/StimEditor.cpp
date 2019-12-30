#include "StimEditor.h"

#include "string/split.h"
#include "string/convert.h"

#include "i18n.h"
#include "SREntity.h"

#include <wx/menu.h>
#include <wx/checkbox.h>
#include <wx/spinctrl.h>
#include <wx/bmpcbox.h>
#include <wx/sizer.h>

#include "wxutil/ChoiceHelper.h"
#include "wxutil/menu/IconTextMenuItem.h"

namespace ui
{

StimEditor::StimEditor(wxWindow* mainPanel, StimTypes& stimTypes) :
	ClassEditor(mainPanel, stimTypes),
	_mainPanel(mainPanel)
{
	setupPage();

	// Setup the context menu items and connect them to the callbacks
	createContextMenu();

	update();
}

void StimEditor::setupPage()
{
	wxPanel* listPanel = findNamedObject<wxPanel>(_mainPanel, "SREditorStimList");
	createListView(listPanel);

	setupEditPanel();

#ifdef USE_BMP_COMBO_BOX
	_addType = findNamedObject<wxBitmapComboBox>(_mainPanel, "StimTypeComboBox");
#else
	{
		// Type selector box
		wxControl* typeBox = findNamedObject<wxControl>(_mainPanel, "StimTypeComboBox");

		// Replace the bitmap combo with an ordinary one
		wxComboBox* combo = new wxComboBox(typeBox->GetParent(), wxID_ANY);
		typeBox->GetContainingSizer()->Prepend(combo, 1, wxEXPAND | wxRIGHT, 6);
		typeBox->Destroy();

		_addType = combo;
		_addType->SetName("StimTypeComboBox");
	}
#endif

	_addType->Bind(wxEVT_COMBOBOX, std::bind(&StimEditor::onAddTypeSelect, this, std::placeholders::_1));

	auto addButton = findNamedObject<wxButton>(_mainPanel, "AddStimButton");
	auto removeButton = findNamedObject<wxButton>(_mainPanel, "RemoveStimButton");

	addButton->Bind(wxEVT_BUTTON, std::bind(&StimEditor::onAddSR, this, std::placeholders::_1));
	removeButton->Bind(wxEVT_BUTTON, std::bind(&StimEditor::onRemoveSR, this, std::placeholders::_1));
	
	reloadStimTypes();
}

void StimEditor::reloadStimTypes()
{
	if (_stimTypes.getStimMap().empty())
	{
		_stimTypes.reload();
	}

	_stimTypes.populateComboBox(_addType);
	_stimTypes.populateComboBox(_type);
}

void StimEditor::setEntity(const SREntityPtr& entity)
{
	// Pass the call to the base class
	ClassEditor::setEntity(entity);

	if (entity)
	{
		wxutil::TreeModel::Ptr stimStore = _entity->getStimStore();
		_list->AssociateModel(stimStore.get());

		// Trigger column width reevaluation
		stimStore->ItemChanged(stimStore->GetRoot());
	}
	else
	{
		// wxWidgets 3.0.0 crashes when associating a NULL model, so use a dummy model
		// to release the old one
		wxutil::TreeModel* dummyStore = new wxutil::TreeModel(SREntity::getColumns(), true);
		_list->AssociateModel(dummyStore);
	}
}

void StimEditor::setupEditPanel()
{
#ifdef USE_BMP_COMBO_BOX
	// Type Selector
	_type = findNamedObject<wxBitmapComboBox>(_mainPanel, "StimEditorTypeCombo");
#else
	// Response property section
	wxControl* typeBox = findNamedObject<wxControl>(_mainPanel, "StimEditorTypeCombo");

	// Replace the bitmap combo with an ordinary one
	wxComboBox* combo = new wxComboBox(typeBox->GetParent(), wxID_ANY);
	typeBox->GetContainingSizer()->Add(combo, 1, wxEXPAND);
	typeBox->Destroy();

	_type = combo;
	_type->SetName("StimEditorTypeCombo");
#endif

	_stimTypes.populateComboBox(_type);
	_type->Connect(wxEVT_COMBOBOX, wxCommandEventHandler(StimEditor::onStimTypeSelect), NULL, this); 

	_propertyWidgets.active = findNamedObject<wxCheckBox>(_mainPanel, "StimEditorActive");

	// Timer Time
	_propertyWidgets.timer.toggle = findNamedObject<wxCheckBox>(_mainPanel, "StimEditorActivationTimer");
	_propertyWidgets.timer.entryHBox = findNamedObject<wxPanel>(_mainPanel, "StimEditorActivationTimerPanel");

	_propertyWidgets.timer.hour = findNamedObject<wxSpinCtrl>(_mainPanel, "StimEditorAcivationTimerHour");
	_propertyWidgets.timer.minute = findNamedObject<wxSpinCtrl>(_mainPanel, "StimEditorAcivationTimerMinute");
	_propertyWidgets.timer.second = findNamedObject<wxSpinCtrl>(_mainPanel, "StimEditorAcivationTimerSecond");
	_propertyWidgets.timer.millisecond = findNamedObject<wxSpinCtrl>(_mainPanel, "StimEditorAcivationTimerMS");

	// Timer type
	_propertyWidgets.timer.typeToggle = findNamedObject<wxCheckBox>(_mainPanel, "StimEditorTimerRestarts");
	_propertyWidgets.timer.reloadHBox = findNamedObject<wxPanel>(_mainPanel, "StimEditorTimerRestartPanel");

	_propertyWidgets.timer.reloadEntry = findNamedObject<wxSpinCtrl>(_mainPanel, "StimEditorTimerReloadsTimes");
	_propertyWidgets.timer.reloadEntry->SetMinClientSize(wxSize(_propertyWidgets.timer.reloadEntry->GetCharWidth() * 9, -1));

	_propertyWidgets.timer.reloadToggle = findNamedObject<wxCheckBox>(_mainPanel, "StimEditorTimerReloads");

	_propertyWidgets.timer.waitToggle = findNamedObject<wxCheckBox>(_mainPanel, "StimEditorTimerWaitsForStart");

	// Time Interval
	_propertyWidgets.timeIntToggle = findNamedObject<wxCheckBox>(_mainPanel, "StimEditorTimeInterval");
	_propertyWidgets.timeIntEntry = findNamedObject<wxSpinCtrl>(_mainPanel, "StimEditorTimeIntervalValue");
	_propertyWidgets.timeUnitLabel = findNamedObject<wxStaticText>(_mainPanel, "StimEditorTimeIntervalUnitLabel");

	// Duration
	_propertyWidgets.durationToggle = findNamedObject<wxCheckBox>(_mainPanel, "StimEditorDuration");
	_propertyWidgets.durationEntry = findNamedObject<wxSpinCtrl>(_mainPanel, "StimEditorDurationValue");
	_propertyWidgets.durationUnitLabel = findNamedObject<wxStaticText>(_mainPanel, "StimEditorDurationUnitLabel");

	// Radius / Use Bounds
	_propertyWidgets.radiusToggle = findNamedObject<wxCheckBox>(_mainPanel, "StimEditorRadius");
	_propertyWidgets.radiusEntry = findNamedObject<wxSpinCtrl>(_mainPanel, "StimEditorRadiusValue");
	_propertyWidgets.useBounds = findNamedObject<wxCheckBox>(_mainPanel, "StimEditorRadiusUseBounds");

	// Final Radius
	_propertyWidgets.finalRadiusToggle = findNamedObject<wxCheckBox>(_mainPanel, "StimEditorRadiusChangesOverTime");
	_propertyWidgets.finalRadiusEntry = findNamedObject<wxSpinCtrl>(_mainPanel, "StimEditorRadiusChangesOverTimeValue");

	// Magnitude
	_propertyWidgets.magnToggle = findNamedObject<wxCheckBox>(_mainPanel, "StimEditorMagnitude");
	_propertyWidgets.magnEntry = findNamedObject<wxSpinCtrl>(_mainPanel, "StimEditorMagnitudeValue");

	// Falloff exponent
	_propertyWidgets.falloffToggle = findNamedObject<wxCheckBox>(_mainPanel, "StimEditorMagnitudeFalloff");

	wxPanel* falloffPanel = findNamedObject<wxPanel>(_mainPanel, "StimEditorMagnitudePanel");

	_propertyWidgets.falloffEntry = new wxSpinCtrlDouble(falloffPanel, wxID_ANY);
	_propertyWidgets.falloffEntry->SetRange(-10, +10);
	_propertyWidgets.falloffEntry->SetIncrement(0.1);
	_propertyWidgets.falloffEntry->SetValue(0);
	_propertyWidgets.falloffEntry->SetMinClientSize(wxSize(7*_propertyWidgets.falloffEntry->GetCharWidth(), -1));

	falloffPanel->GetSizer()->Add(_propertyWidgets.falloffEntry, 2);

	// Max fire count
	_propertyWidgets.maxFireCountToggle = findNamedObject<wxCheckBox>(_mainPanel, "StimEditorMaxFireCount");
	_propertyWidgets.maxFireCountEntry = findNamedObject<wxSpinCtrl>(_mainPanel, "StimEditorMaxFireCountValue");

	// Chance variable
	_propertyWidgets.chanceToggle = findNamedObject<wxCheckBox>(_mainPanel, "StimEditorChance");
	wxPanel* chancePanel = findNamedObject<wxPanel>(_mainPanel, "StimEditorChanceValuePanel");

	_propertyWidgets.chanceEntry = new wxSpinCtrlDouble(chancePanel, wxID_ANY);
	_propertyWidgets.chanceEntry->SetRange(0.0, 1.0);
	_propertyWidgets.chanceEntry->SetIncrement(0.01);
	_propertyWidgets.chanceEntry->SetValue(0);

	chancePanel->GetSizer()->Add(_propertyWidgets.chanceEntry, 1);

	// Velocity variable
	_propertyWidgets.velocityToggle = findNamedObject<wxCheckBox>(_mainPanel, "StimEditorVelocity");
	_propertyWidgets.velocityEntry = findNamedObject<wxTextCtrl>(_mainPanel, "StimEditorVelocityValue");

	// Bounds mins and maxs
	_propertyWidgets.bounds.toggle = findNamedObject<wxCheckBox>(_mainPanel, "StimEditorBounds");
	_propertyWidgets.bounds.panel = findNamedObject<wxPanel>(_mainPanel, "StimEditorBoundsPanel");
	_propertyWidgets.bounds.minEntry = findNamedObject<wxTextCtrl>(_mainPanel, "StimEditorBoundsMinValue");
	_propertyWidgets.bounds.maxEntry = findNamedObject<wxTextCtrl>(_mainPanel, "StimEditorBoundsMaxValue");
	_propertyWidgets.bounds.minEntry->SetMinClientSize(wxSize(100, -1));
	_propertyWidgets.bounds.maxEntry->SetMinClientSize(wxSize(100, -1));

	// Associae the entry fields to stim property keys
	connectSpinButton(_propertyWidgets.radiusEntry, "radius");
	connectSpinButton(_propertyWidgets.finalRadiusEntry, "radius_final");
	connectSpinButton(_propertyWidgets.timeIntEntry, "time_interval");
	connectSpinButton(_propertyWidgets.magnEntry, "magnitude");
	connectSpinButton(_propertyWidgets.falloffEntry, "falloffexponent");
	connectSpinButton(_propertyWidgets.chanceEntry, "chance");
	connectSpinButton(_propertyWidgets.maxFireCountEntry, "max_fire_count");
	connectSpinButton(_propertyWidgets.durationEntry, "duration");
	connectSpinButton(_propertyWidgets.timer.reloadEntry, "timer_reload");

	// These four don't have a direct entity key associated
	connectSpinButton(_propertyWidgets.timer.hour, "");
	connectSpinButton(_propertyWidgets.timer.minute, "");
	connectSpinButton(_propertyWidgets.timer.second, "");
	connectSpinButton(_propertyWidgets.timer.millisecond, "");

	// Connect text entries
	connectEntry(_propertyWidgets.velocityEntry, "velocity");
	connectEntry(_propertyWidgets.bounds.minEntry, "bounds_mins");
	connectEntry(_propertyWidgets.bounds.maxEntry, "bounds_maxs");

	// Connect the checkboxes
	connectCheckButton(_propertyWidgets.active);
	connectCheckButton(_propertyWidgets.useBounds);
	connectCheckButton(_propertyWidgets.radiusToggle);
	connectCheckButton(_propertyWidgets.finalRadiusToggle);
	connectCheckButton(_propertyWidgets.timeIntToggle);
	connectCheckButton(_propertyWidgets.magnToggle);
	connectCheckButton(_propertyWidgets.falloffToggle);
	connectCheckButton(_propertyWidgets.timer.typeToggle);
	connectCheckButton(_propertyWidgets.chanceToggle);
	connectCheckButton(_propertyWidgets.maxFireCountToggle);
	connectCheckButton(_propertyWidgets.durationToggle);
	connectCheckButton(_propertyWidgets.timer.toggle);
	connectCheckButton(_propertyWidgets.timer.reloadToggle);
	connectCheckButton(_propertyWidgets.timer.waitToggle);
	connectCheckButton(_propertyWidgets.velocityToggle);
	connectCheckButton(_propertyWidgets.bounds.toggle);
}

std::string StimEditor::getTimerString()
{
	std::string hour = string::to_string(_propertyWidgets.timer.hour->GetValue());
	std::string minute = string::to_string(_propertyWidgets.timer.minute->GetValue());
	std::string second = string::to_string(_propertyWidgets.timer.second->GetValue());
	std::string ms = string::to_string(_propertyWidgets.timer.millisecond->GetValue());

	return hour + ":" + minute + ":" + second + ":" + ms;
}

void StimEditor::spinButtonChanged(wxSpinCtrl* ctrl)
{
	// Pass the call to the base class
	ClassEditor::spinButtonChanged(ctrl);

	// These four time entries are not in the SpinButtonMap, treat them separately
	if (ctrl == _propertyWidgets.timer.hour ||
		ctrl == _propertyWidgets.timer.minute ||
		ctrl == _propertyWidgets.timer.second ||
		ctrl == _propertyWidgets.timer.millisecond)
	{
		setProperty("timer_time", getTimerString());
	}
}

void StimEditor::checkBoxToggled(wxCheckBox* toggleButton)
{
	bool active = toggleButton->GetValue();

	if (toggleButton == _propertyWidgets.active)
	{
		setProperty("state", active ? "1" : "0");
	}
	else if (toggleButton == _propertyWidgets.useBounds)
	{
		setProperty("use_bounds", active ? "1" : "");
	}
	else if (toggleButton == _propertyWidgets.timer.typeToggle)
	{
		setProperty("timer_type", active ? "RELOAD" : "");
	}
	else if (toggleButton == _propertyWidgets.radiusToggle)
	{
		setProperty("radius", active ? "10" : "");

		// Clear final radius if disabled
		if (!active)
		{
			setProperty("radius_final", "");
		}
	}
	else if (toggleButton == _propertyWidgets.finalRadiusToggle)
	{
		setProperty("radius_final", active ? "10" : "");
	}
	else if (toggleButton == _propertyWidgets.magnToggle)
	{
		setProperty("magnitude", active ? "10" : "");
	}
	else if (toggleButton == _propertyWidgets.maxFireCountToggle)
	{
		setProperty("max_fire_count", active ? "10" : "");
	}
	else if (toggleButton == _propertyWidgets.falloffToggle)
	{
		setProperty("falloffexponent", active ? "1" : "");
	}
	else if (toggleButton == _propertyWidgets.timeIntToggle)
	{
		setProperty("time_interval", active ? "1000" : "");
	}
	else if (toggleButton == _propertyWidgets.chanceToggle)
	{
		std::string entryText = string::to_string(_propertyWidgets.chanceEntry->GetValue());

		setProperty("chance", active ? entryText : "");
	}
	else if (toggleButton == _propertyWidgets.velocityToggle)
	{
		std::string entryText = _propertyWidgets.velocityEntry->GetValue().ToStdString();

		// Enter a default value for the entry text, if it's empty up till now.
		if (active)
		{
			entryText += (entryText.empty()) ? "0 0 100" : "";
		}
		else
		{
			entryText = "";
		}

		setProperty("velocity", entryText);
	}
	else if (toggleButton == _propertyWidgets.bounds.toggle)
	{
		std::string entryText = _propertyWidgets.bounds.minEntry->GetValue().ToStdString();

		// Enter a default value for the entry text, if it's empty up till now.
		if (active)
		{
			entryText += (entryText.empty()) ? "-10 -10 -10" : "";
		}
		else
		{
			entryText = "";
		}

		setProperty("bounds_mins", entryText);

		entryText = _propertyWidgets.bounds.maxEntry->GetValue().ToStdString();

		// Enter a default value for the entry text, if it's empty up till now.
		if (active)
		{
			entryText += (entryText.empty()) ? "10 10 10" : "";
		}
		else
		{
			entryText = "";
		}

		setProperty("bounds_maxs", entryText);
	}
	else if (toggleButton == _propertyWidgets.durationToggle)
	{
		setProperty("duration", active ? "1000" : "");

		// Clear final radius if disabled
		if (!active)
		{
			setProperty("radius_final", "");
		}
	}
	else if (toggleButton == _propertyWidgets.timer.toggle)
	{
		std::string timerStr = getTimerString();
		setProperty("timer_time", active ? timerStr : "");
	}
	else if (toggleButton == _propertyWidgets.timer.reloadToggle)
	{
		setProperty("timer_reload", active ? "1" : "");
	}
	else if (toggleButton == _propertyWidgets.timer.waitToggle)
	{
		setProperty("timer_waitforstart", active ? "1" : "");
	}
}

void StimEditor::openSRListContextMenu()
{
	_list->PopupMenu(_contextMenu.menu.get());
}

void StimEditor::addSR()
{
	if (_entity == NULL) return;

	// Create a new StimResponse object
	int id = _entity->add();

	// Get a reference to the newly allocated object
	StimResponse& sr = _entity->get(id);
	sr.set("class", "S");

	// Get the selected stim type name from the combo box
	std::string name = getStimTypeIdFromSelector(_addType);
	sr.set("type", (!name.empty()) ? name : _stimTypes.getFirstName());

	sr.set("state", "1");

	// Update the list stores AFTER the type has been set
	_entity->updateListStores();

	// Select the newly created stim
	selectId(id);
}

// Create the context menus
void StimEditor::createContextMenu()
{
	// Menu widgets (is not packed, hence create a shared_ptr)
	_contextMenu.menu.reset(new wxMenu);

	_contextMenu.enable =
		_contextMenu.menu->Append(new wxutil::IconTextMenuItem(_("Activate"), "sr_stim.png"));
	_contextMenu.disable = 
		_contextMenu.menu->Append(new wxutil::IconTextMenuItem(_("Deactivate"), "sr_stim_inactive.png"));
	_contextMenu.duplicate =
		_contextMenu.menu->Append(new wxutil::StockIconTextMenuItem(_("Duplicate"), wxART_COPY));
	_contextMenu.remove = 
		_contextMenu.menu->Append(new wxutil::StockIconTextMenuItem(_("Delete"), wxART_DELETE));

	// Connect up the signals
	_contextMenu.menu->Connect(_contextMenu.remove->GetId(), wxEVT_MENU, 
		wxCommandEventHandler(StimEditor::onContextMenuDelete), NULL, this);
	_contextMenu.menu->Connect(_contextMenu.enable->GetId(), wxEVT_MENU, 
		wxCommandEventHandler(StimEditor::onContextMenuEnable), NULL, this);
	_contextMenu.menu->Connect(_contextMenu.disable->GetId(), wxEVT_MENU, 
		wxCommandEventHandler(StimEditor::onContextMenuDisable), NULL, this);
	_contextMenu.menu->Connect(_contextMenu.duplicate->GetId(), wxEVT_MENU, 
		wxCommandEventHandler(StimEditor::onContextMenuDuplicate), NULL, this);
}

void StimEditor::update()
{
	_updatesDisabled = true; // avoid unwanted callbacks

	wxPanel* mainPanel = findNamedObject<wxPanel>(_mainPanel, "SREditorStimPanel");

	int id = getIdFromSelection();

	if (id > 0)
	{
		// Update all the widgets
		mainPanel->Enable(true);

		StimResponse& sr = _entity->get(id);

		std::string typeToFind = sr.get("type");

		// Get the iter into the liststore pointing at the correct STIM_YYYY type
		wxutil::ChoiceHelper::SelectComboItemByStoredString(_type, typeToFind);

		// Active
		_propertyWidgets.active->SetValue(sr.get("state") == "1");

		// Use Radius
		bool useRadius = (sr.get("radius") != "");
		_propertyWidgets.radiusToggle->SetValue(useRadius);
		_propertyWidgets.radiusEntry->SetValue(string::convert<int>(sr.get("radius")));
		_propertyWidgets.radiusEntry->Enable(useRadius);

		// Use Bounds
		_propertyWidgets.useBounds->SetValue(sr.get("use_bounds") == "1" && useRadius);
		_propertyWidgets.useBounds->Enable(useRadius);

		// Use Duration
		bool useDuration = (sr.get("duration") != "");
		_propertyWidgets.durationToggle->SetValue(useDuration);
		_propertyWidgets.durationEntry->SetValue(string::convert<int>(sr.get("duration")));
		_propertyWidgets.durationEntry->Enable(useDuration);
		_propertyWidgets.durationUnitLabel->Enable(useDuration);

		// Use Time interval
		bool useTimeInterval = (sr.get("time_interval") != "");
		_propertyWidgets.timeIntToggle->SetValue(useTimeInterval);
		_propertyWidgets.timeIntEntry->SetValue(string::convert<int>(sr.get("time_interval")));
		_propertyWidgets.timeIntEntry->Enable(useTimeInterval);
		_propertyWidgets.timeUnitLabel->Enable(useTimeInterval);

		// Use Final radius (duration must be enabled for this to work)
		bool useFinalRadius = (sr.get("radius_final") != "");
		_propertyWidgets.finalRadiusToggle->SetValue(useFinalRadius && useDuration);
		_propertyWidgets.finalRadiusEntry->SetValue(string::convert<int>(sr.get("radius_final")));
		_propertyWidgets.finalRadiusToggle->Enable(useRadius && useDuration);
		_propertyWidgets.finalRadiusEntry->Enable(useFinalRadius && useDuration && useRadius);

		// Timer time
		bool useTimerTime = !sr.get("timer_time").empty();
		_propertyWidgets.timer.toggle->SetValue(useTimerTime);
		_propertyWidgets.timer.toggle->Enable(true);
		_propertyWidgets.timer.entryHBox->Enable(useTimerTime);

		// Split the property string and distribute the parts into the entry fields
		std::vector<std::string> parts;
		std::string timerTime = sr.get("timer_time");
		string::split(parts, timerTime, ":");

		std::string hour = (parts.size() > 0) ? parts[0] : "";
		std::string minute = (parts.size() > 1) ? parts[1] : "";
		std::string second = (parts.size() > 2) ? parts[2] : "";
		std::string ms = (parts.size() > 3) ? parts[3] : "";

		_propertyWidgets.timer.hour->SetValue(string::convert<int>(hour));
		_propertyWidgets.timer.minute->SetValue(string::convert<int>(minute));
		_propertyWidgets.timer.second->SetValue(string::convert<int>(second));
		_propertyWidgets.timer.millisecond->SetValue(string::convert<int>(ms));

		_propertyWidgets.timer.waitToggle->SetValue(
			useTimerTime && sr.get("timer_waitforstart") == "1"
		);
		_propertyWidgets.timer.waitToggle->Enable(useTimerTime);

		// Timer Type
		bool useTimerType = sr.get("timer_type") == "RELOAD" && useTimerTime;
		_propertyWidgets.timer.typeToggle->SetValue(useTimerType);
		_propertyWidgets.timer.typeToggle->Enable(useTimerTime);

		bool userTimerReload = useTimerType && !sr.get("timer_reload").empty();
		_propertyWidgets.timer.reloadToggle->SetValue(userTimerReload);
		_propertyWidgets.timer.reloadToggle->Enable(useTimerType);
		_propertyWidgets.timer.reloadEntry->SetValue(string::convert<int>(sr.get("timer_reload")));
		_propertyWidgets.timer.reloadHBox->Enable(userTimerReload);

		// Use Magnitude
		bool useMagnitude = (sr.get("magnitude") != "");
		_propertyWidgets.magnToggle->SetValue(useMagnitude);
		_propertyWidgets.magnEntry->SetValue(string::convert<int>(sr.get("magnitude")));
		_propertyWidgets.magnEntry->Enable(useMagnitude);

		// Use falloff exponent widgets
		bool useFalloff = (sr.get("falloffexponent") != "");

		_propertyWidgets.falloffToggle->SetValue(useFalloff);
		_propertyWidgets.falloffEntry->SetValue(string::convert<double>(sr.get("falloffexponent")));
		_propertyWidgets.falloffToggle->Enable(useMagnitude);
		_propertyWidgets.falloffEntry->Enable(useMagnitude && useFalloff);

		// Use Chance
		bool useChance = (sr.get("chance") != "");
		_propertyWidgets.chanceToggle->SetValue(useChance);
		_propertyWidgets.chanceEntry->SetValue(string::convert<double>(sr.get("chance")));
		_propertyWidgets.chanceEntry->Enable(useChance);

		// Use Max Fire Count
		bool useMaxFireCount = (sr.get("max_fire_count") != "");
		_propertyWidgets.maxFireCountToggle->SetValue(useMaxFireCount);
		_propertyWidgets.maxFireCountEntry->SetValue(string::convert<int>(sr.get("max_fire_count")));
		_propertyWidgets.maxFireCountEntry->Enable(useMaxFireCount);

		// Use Velocity
		bool useVelocity = (sr.get("velocity") != "");
		_propertyWidgets.velocityToggle->SetValue(useVelocity);
		_propertyWidgets.velocityEntry->SetValue(sr.get("velocity"));
		_propertyWidgets.velocityEntry->Enable(useVelocity);

		// Use Bounds mins/max
		bool useBoundsMinMax = (sr.get("bounds_mins") != "");
		_propertyWidgets.bounds.toggle->SetValue(useBoundsMinMax);
		_propertyWidgets.bounds.minEntry->SetValue(sr.get("bounds_mins"));
		_propertyWidgets.bounds.maxEntry->SetValue(sr.get("bounds_maxs"));
		_propertyWidgets.bounds.panel->Enable(useBoundsMinMax);

		// Disable the editing of inherited properties completely
		if (sr.inherited())
		{
			mainPanel->Enable(false);
		}

		// If there is anything selected, the duplicate item is always active
		_contextMenu.menu->Enable(_contextMenu.duplicate->GetId(), true);

		// Update the delete context menu item
		_contextMenu.menu->Enable(_contextMenu.remove->GetId(), !sr.inherited());

		// Update the "enable/disable" menu items
		bool state = sr.get("state") == "1";
		_contextMenu.menu->Enable(_contextMenu.enable->GetId(), !state);
		_contextMenu.menu->Enable(_contextMenu.disable->GetId(), state);
	}
	else
	{
		mainPanel->Enable(false);

		// Disable the "non-Add" context menu items
		_contextMenu.menu->Enable(_contextMenu.remove->GetId(), false);
		_contextMenu.menu->Enable(_contextMenu.enable->GetId(), false);
		_contextMenu.menu->Enable(_contextMenu.disable->GetId(), false);
		_contextMenu.menu->Enable(_contextMenu.duplicate->GetId(), false);
	}

	_updatesDisabled = false;
}

void StimEditor::selectionChanged()
{
	update();
}

// Delete context menu items activated
void StimEditor::onContextMenuDelete(wxCommandEvent& ev)
{
	// Delete the selected stim from the list
	removeSR();
}

// Delete context menu items activated
void StimEditor::onContextMenuAdd(wxCommandEvent& ev)
{
	addSR();
}

} // namespace ui
