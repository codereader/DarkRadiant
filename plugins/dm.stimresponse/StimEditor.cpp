#include "StimEditor.h"

#include "gtkutil/LeftAlignment.h"
#include "gtkutil/RightAlignedLabel.h"
#include "gtkutil/LeftAlignedLabel.h"
#include "gtkutil/ScrolledFrame.h"
#include "gtkutil/TreeModel.h"
#include "gtkutil/StockIconMenuItem.h"
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/classification.hpp>
#include "string/convert.h"

#include "i18n.h"
#include "SREntity.h"

#include <gtkmm/box.h>
#include <gtkmm/menu.h>
#include <gtkmm/menuitem.h>
#include <gtkmm/table.h>
#include <gtkmm/checkbutton.h>
#include <gtkmm/spinbutton.h>
#include <gtkmm/treeview.h>
#include <gtkmm/entry.h>
#include <gtkmm/stock.h>
#include <gtkmm/combobox.h>

namespace ui
{

StimEditor::StimEditor(StimTypes& stimTypes) :
	ClassEditor(stimTypes)
{
	populatePage();

	// Setup the context menu items and connect them to the callbacks
	createContextMenu();
}

void StimEditor::populatePage()
{
	Gtk::HBox* srHBox = Gtk::manage(new Gtk::HBox(false, 12));
	pack_start(*srHBox, true, true, 0);

	Gtk::VBox* vbox = Gtk::manage(new Gtk::VBox(false, 6));
	vbox->pack_start(*Gtk::manage(new gtkutil::ScrolledFrame(*_list)), true, true, 0);

	// Create the type selector plus buttons and pack them
	vbox->pack_start(createListButtons(), false, false, 0);

	srHBox->pack_start(*vbox, false, false, 0);

	// The property pane
	srHBox->pack_start(createPropertyWidgets(), true, true, 0);
}

void StimEditor::setEntity(const SREntityPtr& entity)
{
	// Pass the call to the base class
	ClassEditor::setEntity(entity);

	if (entity != NULL)
	{
		const Glib::RefPtr<Gtk::ListStore>& stimStore = _entity->getStimStore();
		_list->set_model(stimStore);
	}
}

Gtk::Widget& StimEditor::createPropertyWidgets()
{
	_propertyWidgets.vbox = Gtk::manage(new Gtk::VBox(false, 6));

	// Type Selector
	_type = createStimTypeSelector();
	_propertyWidgets.vbox->pack_start(*_type.hbox, false, false, 0);

	_type.list->signal_changed().connect(sigc::mem_fun(*this, &StimEditor::onStimTypeSelect));

	// Create the table for the widget alignment
	Gtk::Table* table = Gtk::manage(new Gtk::Table(12, 2, false));
	table->set_row_spacings(6);
	table->set_col_spacings(6);

	_propertyWidgets.vbox->pack_start(*table, false, false, 0);

	int curRow = 0;

	// Active
	_propertyWidgets.active = Gtk::manage(new Gtk::CheckButton(_("Active")));
	 table->attach(*_propertyWidgets.active, 0, 2, curRow, curRow+1);

	 curRow++;

	// Timer Time
	_propertyWidgets.timer.toggle = Gtk::manage(new Gtk::CheckButton(_("Activation Timer:")));

	_propertyWidgets.timer.hour =  Gtk::manage(new Gtk::SpinButton(
		*Gtk::manage(new Gtk::Adjustment(0, 0, 200)), 0, 0));
	_propertyWidgets.timer.minute = Gtk::manage(new Gtk::SpinButton(
		*Gtk::manage(new Gtk::Adjustment(0, 0, 59)), 0, 0));
	_propertyWidgets.timer.second = Gtk::manage(new Gtk::SpinButton(
		*Gtk::manage(new Gtk::Adjustment(0, 0, 59)), 0, 0));
	_propertyWidgets.timer.millisecond = Gtk::manage(new Gtk::SpinButton(
		*Gtk::manage(new Gtk::Adjustment(0, 0, 999, 10)), 0, 0));

	_propertyWidgets.timer.entryHBox = Gtk::manage(new Gtk::HBox(false, 3));

	Gtk::HBox* entryHBox = _propertyWidgets.timer.entryHBox; // shortcut
	entryHBox->pack_start(*_propertyWidgets.timer.hour, false, false, 0);
	entryHBox->pack_start(*Gtk::manage(new Gtk::Label("h")), false, false, 0);
	entryHBox->pack_start(*_propertyWidgets.timer.minute, false, false, 0);
	entryHBox->pack_start(*Gtk::manage(new Gtk::Label("m")), false, false, 0);
	entryHBox->pack_start(*_propertyWidgets.timer.second, false, false, 0);
	entryHBox->pack_start(*Gtk::manage(new Gtk::Label("s")), false, false, 0);
	entryHBox->pack_start(*_propertyWidgets.timer.millisecond, false, false, 0);
	entryHBox->pack_start(*Gtk::manage(new gtkutil::LeftAlignedLabel("ms")), false, false, 0);

	table->attach(*_propertyWidgets.timer.toggle, 0, 1, 1, 2, Gtk::FILL, Gtk::FILL, 0, 0);
	table->attach(*_propertyWidgets.timer.entryHBox, 1, 2, curRow, curRow+1);

	curRow++;

	// Timer type
	Gtk::HBox* timerTypeHBox = Gtk::manage(new Gtk::HBox(false, 12));
	_propertyWidgets.timer.typeToggle = Gtk::manage(new Gtk::CheckButton(_("Timer restarts after firing")));

	_propertyWidgets.timer.reloadHBox = Gtk::manage(new Gtk::HBox(false, 3));

	_propertyWidgets.timer.reloadEntry = Gtk::manage(new Gtk::SpinButton(
		*Gtk::manage(new Gtk::Adjustment(0, 0, 1000)), 0, 0));
	_propertyWidgets.timer.reloadEntry->set_size_request(50, -1);

	_propertyWidgets.timer.reloadToggle = Gtk::manage(new Gtk::CheckButton(_("Timer reloads")));
	_propertyWidgets.timer.reloadLabel = Gtk::manage(new gtkutil::LeftAlignedLabel(_("times")));

	_propertyWidgets.timer.reloadHBox->pack_start(*_propertyWidgets.timer.reloadEntry, false, false, 0);
	_propertyWidgets.timer.reloadHBox->pack_start(*_propertyWidgets.timer.reloadLabel, true, true, 0);

	timerTypeHBox->pack_start(*_propertyWidgets.timer.typeToggle, false, false, 0);
	timerTypeHBox->pack_start(*_propertyWidgets.timer.reloadToggle, false, false, 0);
	timerTypeHBox->pack_start(*_propertyWidgets.timer.reloadHBox, true, true, 0);

	table->attach(*timerTypeHBox, 0, 2, curRow, curRow+1);

	curRow++;

	_propertyWidgets.timer.waitToggle =
		Gtk::manage(new Gtk::CheckButton(_("Timer waits for start (when disabled: starts at spawn time)")));
	table->attach(*_propertyWidgets.timer.waitToggle, 0, 2, curRow, curRow+1);

	curRow++;

	// Time Interval
	Gtk::HBox* timeHBox = Gtk::manage(new Gtk::HBox(false, 6));
	_propertyWidgets.timeIntToggle = Gtk::manage(new Gtk::CheckButton(_("Time interval:")));
	_propertyWidgets.timeIntEntry = Gtk::manage(new Gtk::SpinButton(
		*Gtk::manage(new Gtk::Adjustment(0, 0, 9999999, 10)), 0, 0));
	_propertyWidgets.timeUnitLabel = Gtk::manage(new gtkutil::RightAlignedLabel(_("ms")));

	timeHBox->pack_start(*_propertyWidgets.timeIntEntry, true, true, 0);
	timeHBox->pack_start(*_propertyWidgets.timeUnitLabel, false, false, 0);

	table->attach(*_propertyWidgets.timeIntToggle, 0, 1, curRow, curRow+1, Gtk::FILL, Gtk::FILL, 0, 0);
	table->attach(*timeHBox, 1, 2, curRow, curRow+1, Gtk::FILL, Gtk::FILL, 0, 0);

	curRow++;

	// Duration
	Gtk::HBox* durationHBox = Gtk::manage(new Gtk::HBox(false, 6));
	_propertyWidgets.durationToggle = Gtk::manage(new Gtk::CheckButton(_("Duration:")));
	_propertyWidgets.durationEntry = Gtk::manage(new Gtk::SpinButton(
		*Gtk::manage(new Gtk::Adjustment(0, 0, 9999999, 10)), 0, 0));
	_propertyWidgets.durationUnitLabel = Gtk::manage(new gtkutil::RightAlignedLabel(_("ms")));

	durationHBox->pack_start(*_propertyWidgets.durationEntry, true, true, 0);
	durationHBox->pack_start(*_propertyWidgets.durationUnitLabel, false, false, 0);

	table->attach(*_propertyWidgets.durationToggle, 0, 1, curRow, curRow+1, Gtk::FILL, Gtk::FILL, 0, 0);
	table->attach(*durationHBox, 1, 2, curRow, curRow+1, Gtk::FILL, Gtk::FILL, 0, 0);

	curRow++;

	// Radius / Use Bounds
	Gtk::HBox* radiusHBox = Gtk::manage(new Gtk::HBox(false, 0));
	_propertyWidgets.radiusToggle = Gtk::manage(new Gtk::CheckButton(_("Radius:")));
	_propertyWidgets.radiusEntry = Gtk::manage(new Gtk::SpinButton(
		*Gtk::manage(new Gtk::Adjustment(0, 0, 99999, 1)), 0, 1));
	_propertyWidgets.useBounds = Gtk::manage(new Gtk::CheckButton(_("Use bounds")));
	radiusHBox->pack_start(*_propertyWidgets.radiusEntry, true, true, 0);
	radiusHBox->pack_start(*_propertyWidgets.useBounds, false, false, 6);

	table->attach(*_propertyWidgets.radiusToggle, 0, 1, curRow, curRow+1, Gtk::FILL, Gtk::FILL, 0, 0);
	table->attach(*radiusHBox, 1, 2, curRow, curRow+1, Gtk::FILL, Gtk::FILL, 0, 0);

	curRow++;

	// Final Radius
	Gtk::HBox* finalRadiusHBox = Gtk::manage(new Gtk::HBox(false, 0));
	_propertyWidgets.finalRadiusToggle = Gtk::manage(new Gtk::CheckButton(_("Radius changes over time to:")));

	_propertyWidgets.finalRadiusEntry = Gtk::manage(new Gtk::SpinButton(
		*Gtk::manage(new Gtk::Adjustment(0, 0, 99999, 1)), 0, 1));
	finalRadiusHBox->pack_start(*_propertyWidgets.finalRadiusEntry, true, true, 0);

	table->attach(*_propertyWidgets.finalRadiusToggle, 0, 1, curRow, curRow+1, Gtk::FILL, Gtk::FILL, 0, 0);
	table->attach(*finalRadiusHBox, 1, 2, curRow, curRow+1, Gtk::FILL, Gtk::FILL, 0, 0);

	curRow++;

	// Magnitude
	_propertyWidgets.magnToggle = Gtk::manage(new Gtk::CheckButton(_("Magnitude:")));

	Gtk::HBox* magnHBox = Gtk::manage(new Gtk::HBox(false, 6));
	_propertyWidgets.magnEntry = Gtk::manage(new Gtk::SpinButton(
		*Gtk::manage(new Gtk::Adjustment(0, 0, 10000, 1)), 0, 2));
	_propertyWidgets.magnEntry->set_width_chars(7);

	// Falloff exponent
	_propertyWidgets.falloffToggle = Gtk::manage(new Gtk::CheckButton(_("Falloff Exponent:")));
	_propertyWidgets.falloffEntry = Gtk::manage(new Gtk::SpinButton(
		*Gtk::manage(new Gtk::Adjustment(0, -10, 10, 0.1)), 0, 2));
	_propertyWidgets.falloffEntry->set_width_chars(7);

	magnHBox->pack_start(*_propertyWidgets.magnEntry, false, false, 0);
	magnHBox->pack_start(*_propertyWidgets.falloffToggle, false, false, 0);
	magnHBox->pack_start(*_propertyWidgets.falloffEntry, true, true, 0);

	table->attach(*_propertyWidgets.magnToggle, 0, 1, curRow, curRow+1, Gtk::FILL, Gtk::FILL, 0, 0);
	table->attach(*magnHBox, 1, 2, curRow, curRow+1, Gtk::FILL, Gtk::FILL, 0, 0);

	curRow++;

	// Max fire count
	_propertyWidgets.maxFireCountToggle = Gtk::manage(new Gtk::CheckButton(_("Max Fire Count:")));
	_propertyWidgets.maxFireCountEntry = Gtk::manage(new Gtk::SpinButton(
		*Gtk::manage(new Gtk::Adjustment(0, 0, 1000000)), 0, 0));
	_propertyWidgets.maxFireCountEntry->set_width_chars(7);

	table->attach(*_propertyWidgets.maxFireCountToggle, 0, 1, curRow, curRow+1, Gtk::FILL, Gtk::FILL, 0, 0);
	table->attach(*_propertyWidgets.maxFireCountEntry, 1, 2, curRow, curRow+1, Gtk::FILL, Gtk::FILL, 0, 0);

	curRow++;

	// Chance variable
	_propertyWidgets.chanceToggle = Gtk::manage(new Gtk::CheckButton(_("Chance:")));
	_propertyWidgets.chanceEntry = Gtk::manage(new Gtk::SpinButton(
		*Gtk::manage(new Gtk::Adjustment(0, 0, 1.0, 0.01)), 0, 2));

	table->attach(*_propertyWidgets.chanceToggle, 0, 1, curRow, curRow+1, Gtk::FILL, Gtk::FILL, 0, 0);
	table->attach(*_propertyWidgets.chanceEntry, 1, 2, curRow, curRow+1, Gtk::FILL, Gtk::FILL, 0, 0);

	curRow++;

	// Velocity variable
	_propertyWidgets.velocityToggle = Gtk::manage(new Gtk::CheckButton(_("Velocity:")));
	_propertyWidgets.velocityEntry = Gtk::manage(new Gtk::Entry);

	table->attach(*_propertyWidgets.velocityToggle, 0, 1, curRow, curRow+1, Gtk::FILL, Gtk::FILL, 0, 0);
	table->attach(*_propertyWidgets.velocityEntry, 1, 2, curRow, curRow+1, Gtk::FILL, Gtk::FILL, 0, 0);

	curRow++;

	// Bounds mins and maxs
	_propertyWidgets.bounds.hbox = Gtk::manage(new Gtk::HBox(false, 6));
	_propertyWidgets.bounds.toggle = Gtk::manage(new Gtk::CheckButton(_("Bounds ")));
	_propertyWidgets.bounds.minLabel = Gtk::manage(new Gtk::Label(_("Min:")));
	_propertyWidgets.bounds.maxLabel = Gtk::manage(new Gtk::Label(_("Max:")));
	_propertyWidgets.bounds.minEntry = Gtk::manage(new Gtk::Entry);
	_propertyWidgets.bounds.maxEntry = Gtk::manage(new Gtk::Entry);
	_propertyWidgets.bounds.minEntry->set_size_request(100, -1);
	_propertyWidgets.bounds.maxEntry->set_size_request(100, -1);

	_propertyWidgets.bounds.hbox->pack_start(*_propertyWidgets.bounds.minLabel, false, false, 0);
	_propertyWidgets.bounds.hbox->pack_start(*_propertyWidgets.bounds.minEntry, true, true, 0);
	_propertyWidgets.bounds.hbox->pack_start(*_propertyWidgets.bounds.maxLabel, false, false, 0);
	_propertyWidgets.bounds.hbox->pack_start(*_propertyWidgets.bounds.maxEntry, true, true, 0);

	table->attach(*_propertyWidgets.bounds.toggle, 0, 1, curRow, curRow+1, Gtk::FILL, Gtk::FILL, 0, 0);
	table->attach(*_propertyWidgets.bounds.hbox, 1, 2, curRow, curRow+1, Gtk::FILL, Gtk::FILL, 0, 0);

	curRow++;

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

	return *_propertyWidgets.vbox;
}

std::string StimEditor::getTimerString()
{
	std::string hour = string::to_string(_propertyWidgets.timer.hour->get_value_as_int());
	std::string minute = string::to_string(_propertyWidgets.timer.minute->get_value_as_int());
	std::string second = string::to_string(_propertyWidgets.timer.second->get_value_as_int());
	std::string ms = string::to_string(_propertyWidgets.timer.millisecond->get_value_as_int());

	return hour + ":" + minute + ":" + second + ":" + ms;
}

void StimEditor::spinButtonChanged(Gtk::SpinButton* spinButton)
{
	// Pass the call to the base class
	ClassEditor::spinButtonChanged(spinButton);

	// These four time entries are not in the SpinButtonMap, treat them separately
	if (spinButton == _propertyWidgets.timer.hour ||
		spinButton == _propertyWidgets.timer.minute ||
		spinButton == _propertyWidgets.timer.second ||
		spinButton == _propertyWidgets.timer.millisecond)
	{
		setProperty("timer_time", getTimerString());
	}
}

void StimEditor::checkBoxToggled(Gtk::CheckButton* toggleButton)
{
	bool active = toggleButton->get_active();

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
		std::string entryText = string::to_string(_propertyWidgets.chanceEntry->get_value());

		setProperty("chance", active ? entryText : "");
	}
	else if (toggleButton == _propertyWidgets.velocityToggle)
	{
		std::string entryText = _propertyWidgets.velocityEntry->get_text();

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
		std::string entryText = _propertyWidgets.bounds.minEntry->get_text();

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

		entryText = _propertyWidgets.bounds.maxEntry->get_text();

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

void StimEditor::openContextMenu(Gtk::TreeView* view)
{
	_contextMenu.menu->popup(1, gtk_get_current_event_time());
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
	std::string name = getStimTypeIdFromSelector(_addType.list);
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
	_contextMenu.menu.reset(new Gtk::Menu());

	// Each menu gets a delete item
	_contextMenu.remove =
		Gtk::manage(new gtkutil::StockIconMenuItem(Gtk::Stock::DELETE, _("Delete")));
	//_contextMenu.add = Gtk::manage(new gtkutil::StockIconMenuItem(Gtk::Stock::ADD, "Add Stim"));
	_contextMenu.disable =
		Gtk::manage(new gtkutil::StockIconMenuItem(Gtk::Stock::NO, _("Deactivate")));
	_contextMenu.enable =
		Gtk::manage(new gtkutil::StockIconMenuItem(Gtk::Stock::YES, _("Activate")));
	_contextMenu.duplicate =
		Gtk::manage(new gtkutil::StockIconMenuItem(Gtk::Stock::COPY, _("Duplicate")));

	//_contextMenu.menu->append(*_contextMenu.add);
	_contextMenu.menu->append(*_contextMenu.enable);
	_contextMenu.menu->append(*_contextMenu.disable);
	_contextMenu.menu->append(*_contextMenu.duplicate);
	_contextMenu.menu->append(*_contextMenu.remove);

	// Connect up the signals
	_contextMenu.remove->signal_activate().connect(sigc::mem_fun(*this, &StimEditor::onContextMenuDelete));
	_contextMenu.enable->signal_activate().connect(sigc::mem_fun(*this, &StimEditor::onContextMenuEnable));
	_contextMenu.disable->signal_activate().connect(sigc::mem_fun(*this, &StimEditor::onContextMenuDisable));
	_contextMenu.duplicate->signal_activate().connect(sigc::mem_fun(*this, &StimEditor::onContextMenuDuplicate));

	// Show menus (not actually visible until popped up)
	_contextMenu.menu->show_all();
}

void StimEditor::update()
{
	_updatesDisabled = true; // avoid unwanted callbacks

	int id = getIdFromSelection();

	if (id > 0)
	{
		// Update all the widgets
		_propertyWidgets.vbox->set_sensitive(true);

		StimResponse& sr = _entity->get(id);

		// Get the iter into the liststore pointing at the correct STIM_YYYY type
		_type.list->set_active(_stimTypes.getIterForName(sr.get("type")));

		// Active
		_propertyWidgets.active->set_active(sr.get("state") == "1");

		// Use Radius
		bool useRadius = (sr.get("radius") != "");
		_propertyWidgets.radiusToggle->set_active(useRadius);
		_propertyWidgets.radiusEntry->set_value(string::convert<float>(sr.get("radius")));
		_propertyWidgets.radiusEntry->set_sensitive(useRadius);

		// Use Bounds
		_propertyWidgets.useBounds->set_active(sr.get("use_bounds") == "1" && useRadius);
		_propertyWidgets.useBounds->set_sensitive(useRadius);

		// Use Duration
		bool useDuration = (sr.get("duration") != "");
		_propertyWidgets.durationToggle->set_active(useDuration);
		_propertyWidgets.durationEntry->set_value(string::convert<int>(sr.get("duration")));
		_propertyWidgets.durationEntry->set_sensitive(useDuration);
		_propertyWidgets.durationUnitLabel->set_sensitive(useDuration);

		// Use Time interval
		bool useTimeInterval = (sr.get("time_interval") != "");
		_propertyWidgets.timeIntToggle->set_active(useTimeInterval);
		_propertyWidgets.timeIntEntry->set_value(string::convert<int>(sr.get("time_interval")));
		_propertyWidgets.timeIntEntry->set_sensitive(useTimeInterval);
		_propertyWidgets.timeUnitLabel->set_sensitive(useTimeInterval);

		// Use Final radius (duration must be enabled for this to work)
		bool useFinalRadius = (sr.get("radius_final") != "");
		_propertyWidgets.finalRadiusToggle->set_active(useFinalRadius && useDuration);
		_propertyWidgets.finalRadiusEntry->set_value(string::convert<float>(sr.get("radius_final")));
		_propertyWidgets.finalRadiusToggle->set_sensitive(useRadius && useDuration);
		_propertyWidgets.finalRadiusEntry->set_sensitive(useFinalRadius && useDuration && useRadius);

		// Timer time
		bool useTimerTime = !sr.get("timer_time").empty();
		_propertyWidgets.timer.toggle->set_active(useTimerTime);
		_propertyWidgets.timer.toggle->set_sensitive(true);
		_propertyWidgets.timer.entryHBox->set_sensitive(useTimerTime);

		// Split the property string and distribute the parts into the entry fields
		std::vector<std::string> parts;
		std::string timerTime = sr.get("timer_time");
		boost::algorithm::split(parts, timerTime, boost::algorithm::is_any_of(":"));

		std::string hour = (parts.size() > 0) ? parts[0] : "";
		std::string minute = (parts.size() > 1) ? parts[1] : "";
		std::string second = (parts.size() > 2) ? parts[2] : "";
		std::string ms = (parts.size() > 3) ? parts[3] : "";

		_propertyWidgets.timer.hour->set_value(string::convert<int>(hour));
		_propertyWidgets.timer.minute->set_value(string::convert<int>(minute));
		_propertyWidgets.timer.second->set_value(string::convert<int>(second));
		_propertyWidgets.timer.millisecond->set_value(string::convert<int>(ms));

		_propertyWidgets.timer.waitToggle->set_active(
			useTimerTime && sr.get("timer_waitforstart") == "1"
		);
		_propertyWidgets.timer.waitToggle->set_sensitive(useTimerTime);

		// Timer Type
		bool useTimerType = sr.get("timer_type") == "RELOAD" && useTimerTime;
		_propertyWidgets.timer.typeToggle->set_active(useTimerType);
		_propertyWidgets.timer.typeToggle->set_sensitive(useTimerTime);

		bool userTimerReload = useTimerType && !sr.get("timer_reload").empty();
		_propertyWidgets.timer.reloadToggle->set_active(userTimerReload);
		_propertyWidgets.timer.reloadToggle->set_sensitive(useTimerType);
		_propertyWidgets.timer.reloadEntry->set_value(string::convert<int>(sr.get("timer_reload")));
		_propertyWidgets.timer.reloadHBox->set_sensitive(userTimerReload);

		// Use Magnitude
		bool useMagnitude = (sr.get("magnitude") != "");
		_propertyWidgets.magnToggle->set_active(useMagnitude);
		_propertyWidgets.magnEntry->set_value(string::convert<float>(sr.get("magnitude")));
		_propertyWidgets.magnEntry->set_sensitive(useMagnitude);

		// Use falloff exponent widgets
		bool useFalloff = (sr.get("falloffexponent") != "");

		_propertyWidgets.falloffToggle->set_active(useFalloff);
		_propertyWidgets.falloffEntry->set_value(string::convert<float>(sr.get("falloffexponent")));
		_propertyWidgets.falloffToggle->set_sensitive(useMagnitude);
		_propertyWidgets.falloffEntry->set_sensitive(useMagnitude && useFalloff);

		// Use Chance
		bool useChance = (sr.get("chance") != "");
		_propertyWidgets.chanceToggle->set_active(useChance);
		_propertyWidgets.chanceEntry->set_value(string::convert<float>(sr.get("chance")));
		_propertyWidgets.chanceEntry->set_sensitive(useChance);

		// Use Max Fire Count
		bool useMaxFireCount = (sr.get("max_fire_count") != "");
		_propertyWidgets.maxFireCountToggle->set_active(useMaxFireCount);
		_propertyWidgets.maxFireCountEntry->set_value(string::convert<float>(sr.get("max_fire_count")));
		_propertyWidgets.maxFireCountEntry->set_sensitive(useMaxFireCount);

		// Use Velocity
		bool useVelocity = (sr.get("velocity") != "");
		_propertyWidgets.velocityToggle->set_active(useVelocity);
		_propertyWidgets.velocityEntry->set_text(sr.get("velocity"));
		_propertyWidgets.velocityEntry->set_sensitive(useVelocity);

		// Use Bounds mins/max
		bool useBoundsMinMax = (sr.get("bounds_mins") != "");
		_propertyWidgets.bounds.toggle->set_active(useBoundsMinMax);
		_propertyWidgets.bounds.minEntry->set_text(sr.get("bounds_mins"));
		_propertyWidgets.bounds.maxEntry->set_text(sr.get("bounds_maxs"));
		_propertyWidgets.bounds.hbox->set_sensitive(useBoundsMinMax);

		// Disable the editing of inherited properties completely
		if (sr.inherited())
		{
			_propertyWidgets.vbox->set_sensitive(false);
		}

		// If there is anything selected, the duplicate item is always active
		_contextMenu.duplicate->set_sensitive(true);

		// Update the delete context menu item
		_contextMenu.remove->set_sensitive(!sr.inherited());

		// Update the "enable/disable" menu items
		bool state = sr.get("state") == "1";
		_contextMenu.enable->set_sensitive(!state);
		_contextMenu.disable->set_sensitive(state);
	}
	else
	{
		_propertyWidgets.vbox->set_sensitive(false);
		// Disable the "non-Add" context menu items
		_contextMenu.remove->set_sensitive(false);
		_contextMenu.enable->set_sensitive(false);
		_contextMenu.disable->set_sensitive(false);
		_contextMenu.duplicate->set_sensitive(false);
	}

	_updatesDisabled = false;
}

void StimEditor::selectionChanged()
{
	update();
}

// Delete context menu items activated
void StimEditor::onContextMenuDelete()
{
	// Delete the selected stim from the list
	removeSR();
}

// Delete context menu items activated
void StimEditor::onContextMenuAdd()
{
	addSR();
}

} // namespace ui
