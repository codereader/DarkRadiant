#include "DifficultyPanel.h"

#include "Objective.h"

#include "i18n.h"
#include <algorithm>

#include "string/convert.h"
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>

#include <gtkmm/box.h>
#include <gtkmm/checkbutton.h>

namespace objectives
{

DifficultyPanel::DifficultyPanel() :
	Gtk::HBox(false, 6)
{
	// Create the main widget
	_allLevels = Gtk::manage(new Gtk::CheckButton(_("All Levels")));
	_allLevels->signal_toggled().connect(
		sigc::bind(sigc::mem_fun(*this, &DifficultyPanel::_onCheckBoxToggle), _allLevels)
	);

	// First pack the "All difficulty levels" toggle
	pack_start(*_allLevels, true, true, 0);

	// Create the various toggles
	// TODO: Connect to optional Difficulty plugin
	_toggles.push_back(Gtk::manage(new Gtk::CheckButton(_("Level 1: Easy"))));
	_toggles.push_back(Gtk::manage(new Gtk::CheckButton(_("Level 2: Hard"))));
	_toggles.push_back(Gtk::manage(new Gtk::CheckButton(_("Level 3: Expert"))));

	// The hbox for the difficulty levels 1..N
	_levelHBox = Gtk::manage(new Gtk::HBox(true, 6));

	// Then all the other ones
	for (std::size_t i = 0; i < _toggles.size(); i++)
	{
		_levelHBox->pack_start(*_toggles[i], true, true, 0);

		// Connect the checkbox
		_toggles[i]->signal_toggled().connect(
			sigc::bind(sigc::mem_fun(*this, &DifficultyPanel::_onCheckBoxToggle), _toggles[i])
		);
	}

	pack_start(*_levelHBox, true, true, 0);
}

void DifficultyPanel::populateFromObjective(const Objective& obj)
{
	// De-serialise the difficulty level string
	std::vector<std::string> parts;
	boost::algorithm::split(parts, obj.difficultyLevels, boost::algorithm::is_any_of(" "));

	// Set the "applies to all difficulty" toggle
	_allLevels->set_active(obj.difficultyLevels.empty());

	// Set all levels to deactivated
	for (std::size_t i = 0; i < _toggles.size(); i++)
	{
		// See if this level appears in the difficulty string, if yes => toggled
		_toggles[i]->set_active(
			std::find(parts.begin(), parts.end(), string::to_string(i)) != parts.end()
		);
	}

	// Sensitivity is automatically handled
}

void DifficultyPanel::_onCheckBoxToggle(Gtk::CheckButton* button)
{
	// Update the sensitivity of the other toggles

	if (button == _allLevels)
	{
		// The "All levels" toggle has been changed, set the 1..N checkboxes
		// to the inverse of the togglebutton's status
		_levelHBox->set_sensitive(!_allLevels->get_active());
	}
}

void DifficultyPanel::writeToObjective(Objective& obj)
{
	// Set the difficulty to "all levels" per default
	obj.difficultyLevels = "";

	if (!_allLevels->get_active())
	{
		// Not applicable to all difficulty levels, form the string
		for (std::size_t i = 0; i < _toggles.size(); i++)
		{
			// Check each toggle button
			if (_toggles[i]->get_active())
			{
				std::string prefix = (!obj.difficultyLevels.empty()) ? " " : "";
				obj.difficultyLevels += prefix + string::to_string(i);
			}
		}
	}
}

} // namespace objectives
