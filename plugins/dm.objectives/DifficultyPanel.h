#pragma once

#include <vector>
#include <wx/event.h>

class wxPanel;
class wxCheckBox;

namespace objectives
{

class Objective;

/**
 * greebo: This encapsulates the widgets needed to set the
 * difficulty levels an objective is applicable to.
 */
class DifficultyPanel :
	public wxEvtHandler
{
private:
	wxCheckBox* _allLevels;

	// One toggle for each difficulty level
	std::vector<wxCheckBox*> _toggles;

public:
	// The constructor is preparing the widgets
	DifficultyPanel(wxPanel* container);

	// Updates the widgets from the settings found on the objective
	void populateFromObjective(const Objective& obj);

	// Updates the objective's difficulty settings
	void writeToObjective(Objective& obj);

private:
	// Callback for checkbox toggle
	void _onCheckBoxToggle(wxCommandEvent& ev); // button is manually bound
};

} // namespace objectives
