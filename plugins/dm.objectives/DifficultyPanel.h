#ifndef _DIFFICULTY_PANEL_H_
#define _DIFFICULTY_PANEL_H_

#include <vector>
#include <gtkmm/box.h>

namespace Gtk
{
	class CheckButton;
}

namespace objectives
{

class Objective;

/**
 * greebo: This encapsulates the widgets needed to set the
 * difficulty levels an objective is applicable to.
 */
class DifficultyPanel :
	public Gtk::HBox
{
private:
	Gtk::CheckButton* _allLevels;

	Gtk::HBox* _levelHBox;

	// One toggle for each difficulty level
	std::vector<Gtk::CheckButton*> _toggles;

public:
	// The constructor is preparing the widgets
	DifficultyPanel();

	// Updates the widgets from the settings found on the objective
	void populateFromObjective(const Objective& obj);

	// Updates the objective's difficulty settings
	void writeToObjective(Objective& obj);

private:
	// Callback for checkbox toggle
	void _onCheckBoxToggle(Gtk::CheckButton* button); // button is manually bound
};

} // namespace objectives

#endif /* _DIFFICULTY_PANEL_H_ */
