#include "DifficultyPanel.h"

#include "Objective.h"

#include <gtk/gtk.h>
#include <algorithm>

#include "string/string.h"
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>

namespace objectives {

	namespace {

		enum {
			WIDGET_MAIN,			// the main container
			WIDGET_TOGGLE_ALL_DIFF,	// checkbox for "all difficulty levels" checkbox
			WIDGET_TOGGLE_HBOX,		// hbox for the various difficulty level checkboxes
		};

		typedef std::vector<std::string> StringParts;

	} // namespace

DifficultyPanel::DifficultyPanel() :
	_updateMutex(false)
{
	// Create the main widget
	_widgets[WIDGET_MAIN] = gtk_hbox_new(FALSE, 6);
	_widgets[WIDGET_TOGGLE_ALL_DIFF] = gtk_check_button_new_with_label("All Levels");

	// First pack the "All difficulty levels" toggle
	gtk_box_pack_start(GTK_BOX(_widgets[WIDGET_MAIN]), 
		_widgets[WIDGET_TOGGLE_ALL_DIFF], TRUE, TRUE, 0);

	g_signal_connect(G_OBJECT(_widgets[WIDGET_TOGGLE_ALL_DIFF]), 
		"toggled", G_CALLBACK(_onCheckBoxToggle), this);

	// Create the various toggles
	// TODO: Connect to optional Difficulty plugin
	_toggles.push_back(gtk_check_button_new_with_label("Level 1: Easy"));
	_toggles.push_back(gtk_check_button_new_with_label("Level 2: Hard"));
	_toggles.push_back(gtk_check_button_new_with_label("Level 3: Expert"));

	// The hbox for the difficulty levels 1..N
	_widgets[WIDGET_TOGGLE_HBOX] = gtk_hbox_new(TRUE, 6);

	// Then all the other ones
	for (std::size_t i = 0; i < _toggles.size(); i++) {
		gtk_box_pack_start(GTK_BOX(_widgets[WIDGET_TOGGLE_HBOX]), _toggles[i], TRUE, TRUE, 0);

		// Connect the checkbox
		g_signal_connect(G_OBJECT(_toggles[i]), "toggled", G_CALLBACK(_onCheckBoxToggle), this);
	}

	gtk_box_pack_start(GTK_BOX(_widgets[WIDGET_MAIN]), _widgets[WIDGET_TOGGLE_HBOX], TRUE, TRUE, 0);
}

void DifficultyPanel::populateFromObjective(const Objective& obj) {
	// De-serialise the difficulty level string
	StringParts parts;
	boost::algorithm::split(parts, obj.difficultyLevels, boost::algorithm::is_any_of(" "));

	// Set the "applies to all difficulty" toggle
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(
		_widgets[WIDGET_TOGGLE_ALL_DIFF]), obj.difficultyLevels.empty() ? TRUE : FALSE);

	// Set all levels to deactivated
	for (std::size_t i = 0; i < _toggles.size(); i++) {
		// See if this level appears in the difficulty string, if yes => toggled
		gtk_toggle_button_set_active(
			GTK_TOGGLE_BUTTON(_toggles[i]),
			(std::find(parts.begin(), parts.end(), intToStr(i)) != parts.end()) ? TRUE : FALSE
		);
	}

	// Sensitivity is automatically handled
}

void DifficultyPanel::_onCheckBoxToggle(GtkWidget* togglebutton, DifficultyPanel* self) {
	// prevent infinite-loops during updates
	if (self->_updateMutex) return; 

	// Update the sensitivity of the other toggles

	if (togglebutton == self->_widgets[WIDGET_TOGGLE_ALL_DIFF]) {
		// The "All levels" toggle has been changed, set the 1..N checkboxes 
		// to the inverse of the togglebutton's status
		gtk_widget_set_sensitive(
			self->_widgets[WIDGET_TOGGLE_HBOX],
			!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(togglebutton))
		);
	}
}

void DifficultyPanel::writeToObjective(Objective& obj) {
	// Set the difficulty to "all levels" per default
	obj.difficultyLevels = "";

	if (!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(_widgets[WIDGET_TOGGLE_ALL_DIFF])))
	{
		// Not applicable to all difficulty levels, form the string
		for (std::size_t i = 0; i < _toggles.size(); i++) {
			// Check each toggle button
			if (gtk_toggle_button_get_active(
				GTK_TOGGLE_BUTTON(_toggles[i])))
			{
				std::string prefix = (!obj.difficultyLevels.empty()) ? " " : "";
				obj.difficultyLevels += prefix + intToStr(i);
			}
		}
	}
}

GtkWidget* DifficultyPanel::getWidget() {
	return _widgets[WIDGET_MAIN];
}

} // namespace objectives
