#ifndef _DIFFICULTY_PANEL_H_
#define _DIFFICULTY_PANEL_H_

typedef struct _GtkWidget GtkWidget;
#include <map>
#include <vector>

namespace objectives {

class Objective;

/** 
 * greebo: This encapsulates the widgets needed to set the 
 * difficulty levels an objective is applicable to.
 *
 * Use the getWidget() method to retrive the GtkWidget* pointer
 * for packing into a parent container.
 */
class DifficultyPanel
{
	// The widget storage
	std::map<int, GtkWidget*> _widgets;

	// One toggle for each difficulty level
	std::vector<GtkWidget*> _toggles;

	// TRUE during updates to prevent callbacks from firing
	bool _updateMutex;

public:
	// The constructor is preparing the widgets
	DifficultyPanel();

	// Updates the widgets from the settings found on the objective
	void populateFromObjective(const Objective& obj);

	// Updates the objective's difficulty settings
	void writeToObjective(Objective& obj);

	GtkWidget* getWidget();

private:
	// Callback for checkbox toggle
	static void _onCheckBoxToggle(GtkWidget* togglebutton, DifficultyPanel* self);
};

} // namespace objectives

#endif /* _DIFFICULTY_PANEL_H_ */
