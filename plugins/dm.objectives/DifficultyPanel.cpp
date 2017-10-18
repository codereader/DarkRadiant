#include "DifficultyPanel.h"

#include "Objective.h"

#include "i18n.h"
#include <algorithm>

#include "string/convert.h"
#include "string/split.h"

#include <wx/checkbox.h>
#include <wx/sizer.h>
#include <wx/panel.h>

namespace objectives
{

DifficultyPanel::DifficultyPanel(wxPanel* container)
{
	// Create the main widget
	_allLevels = new wxCheckBox(container, wxID_ANY, _("All Levels"));
	_allLevels->Connect(
		wxEVT_CHECKBOX, wxCommandEventHandler(DifficultyPanel::_onCheckBoxToggle), NULL, this);

	// First pack the "All difficulty levels" toggle
	container->GetSizer()->Add(_allLevels, 0, wxALIGN_CENTER_VERTICAL);

	// Create the various toggles
	// TODO: Connect to optional Difficulty plugin
	_toggles.push_back(new wxCheckBox(container, wxID_ANY, _("Level 1: Easy")));
	_toggles.push_back(new wxCheckBox(container, wxID_ANY, _("Level 2: Hard")));
	_toggles.push_back(new wxCheckBox(container, wxID_ANY, _("Level 3: Expert")));

	// The hbox for the difficulty levels 1..N
	wxBoxSizer* hbox = new wxBoxSizer(wxHORIZONTAL);

	// Then all the other ones
	for (std::size_t i = 0; i < _toggles.size(); i++)
	{
		hbox->Add(_toggles[i], 1, wxALIGN_CENTER_VERTICAL | wxLEFT, 6);

		// Connect the checkbox
		_toggles[i]->Connect(
			wxEVT_CHECKBOX, wxCommandEventHandler(DifficultyPanel::_onCheckBoxToggle), NULL, this);
	}

	container->GetSizer()->Add(hbox, 1, wxALIGN_CENTER_VERTICAL);
}

void DifficultyPanel::populateFromObjective(const Objective& obj)
{
	// De-serialise the difficulty level string
	std::vector<std::string> parts;
	string::split(parts, obj.difficultyLevels, " ");

	// Set the "applies to all difficulty" toggle
	_allLevels->SetValue(obj.difficultyLevels.empty());

	// Set all levels to deactivated
	for (std::size_t i = 0; i < _toggles.size(); i++)
	{
		// See if this level appears in the difficulty string, if yes => toggled
		_toggles[i]->SetValue(
			std::find(parts.begin(), parts.end(), string::to_string(i)) != parts.end()
		);
	}

	updateSensitivity();
}

void DifficultyPanel::updateSensitivity()
{
	// The "All levels" toggle has been changed, set the 1..N checkboxes
	// to the inverse of the togglebutton's status
	for (std::size_t i = 0; i < _toggles.size(); i++)
	{
		_toggles[i]->Enable(!_allLevels->GetValue());
	}
}

void DifficultyPanel::_onCheckBoxToggle(wxCommandEvent& ev)
{
	// Update the sensitivity of the other toggles

	if (ev.GetEventObject() == _allLevels)
	{
		// The "All levels" toggle has been changed, set the 1..N checkboxes
		// to the inverse of the togglebutton's status
		for (std::size_t i = 0; i < _toggles.size(); i++)
		{
			_toggles[i]->Enable(!_allLevels->GetValue());
		}
	}
}

void DifficultyPanel::writeToObjective(Objective& obj)
{
	// Set the difficulty to "all levels" per default
	obj.difficultyLevels = "";

	if (!_allLevels->GetValue())
	{
		// Not applicable to all difficulty levels, form the string
		for (std::size_t i = 0; i < _toggles.size(); i++)
		{
			// Check each toggle button
			if (_toggles[i]->GetValue())
			{
				std::string prefix = (!obj.difficultyLevels.empty()) ? " " : "";
				obj.difficultyLevels += prefix + string::to_string(i);
			}
		}
	}
}

} // namespace objectives
