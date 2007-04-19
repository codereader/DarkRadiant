#ifndef STIMEDITOR_H_
#define STIMEDITOR_H_

#include "ClassEditor.h"

namespace ui {

class StimEditor :
	public ClassEditor
{
	struct PropertyWidgets {
		GtkWidget* vbox;
		GtkWidget* active;
		GtkWidget* useBounds;
		GtkWidget* radiusToggle;
		GtkWidget* radiusEntry;
		GtkWidget* timeIntToggle;
		GtkWidget* timeIntEntry;
		GtkWidget* timeUnitLabel;
		
		struct TimerWidgets {
			GtkWidget* toggle;
			GtkWidget* entryHBox;
			GtkWidget* hour;
			GtkWidget* minute;
			GtkWidget* second;
			GtkWidget* millisecond;
			
			GtkWidget* typeToggle;
			
			GtkWidget* reloadHBox;
			GtkWidget* reloadToggle;
			GtkWidget* reloadEntry;
			GtkWidget* reloadLabel;
			
			GtkWidget* waitToggle;
		} timer;
		
		GtkWidget* durationToggle;
		GtkWidget* durationEntry;
		GtkWidget* durationUnitLabel;		
		GtkWidget* magnToggle;
		GtkWidget* magnEntry;
		GtkWidget* falloffToggle;
		GtkWidget* falloffEntry;
		GtkWidget* chanceToggle;
		GtkWidget* chanceEntry;
	} _propertyWidgets;
	
	struct ListContextMenu {
		GtkWidget* menu;
		GtkWidget* remove;
		GtkWidget* add;
		GtkWidget* enable;
		GtkWidget* disable;
		GtkWidget* duplicate;
	} _contextMenu;

	struct ListButtons {
		GtkWidget* add;
		GtkWidget* remove;
	} _listButtons;

public:
	/** greebo: Constructor creates all the widgets
	 */
	StimEditor(StimTypes& stimTypes);

	/** greebo: Sets the new entity (is called by the StimResponseEditor class)
	 */
	virtual void setEntity(SREntityPtr entity);

	/** greebo: Updates the widgets (e.g. after a selection change) 
	 */
	void update();

private:
	/** greebo: Adds a new stim to the list
	 */
	void addSR();

	/** greebo: Gets called when an entry box changes, overrides the 
	 * 			method from the base class.
	 */
	void entryChanged(GtkEditable* editable);

	/** greebo: Updates the associated text fields when a check box
	 * 			is toggled.
	 */
	void checkBoxToggled(GtkToggleButton* toggleButton);

	/** greebo: As the name states, this creates the context menu widgets.
	 */
	void createContextMenu();

	/** greebo: Widget creation helper methods
	 */
	GtkWidget* createPropertyWidgets();
	GtkWidget* createListButtons();

	/** greebo: Gets called when the stim selection gets changed 
	 */
	virtual void selectionChanged();

	void openContextMenu(GtkTreeView* view);

	/** greebo: Creates all the widgets
	 */
	void populatePage();
	
	// Context menu GTK callbacks
	static void onContextMenuAdd(GtkWidget* w, StimEditor* self);
	static void onContextMenuDelete(GtkWidget* w, StimEditor* self);
};

} // namespace ui

#endif /*STIMEDITOR_H_*/
