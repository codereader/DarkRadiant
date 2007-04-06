#ifndef STIMEDITOR_H_
#define STIMEDITOR_H_

#include "ClassEditor.h"

namespace ui {

class StimEditor :
	public ClassEditor
{
	typedef std::map<GtkEditable*, std::string> EntryMap;
	EntryMap _entryWidgets; 
	
	struct PropertyWidgets {
		GtkWidget* vbox;
		GtkWidget* typeList;
		GtkWidget* stimButton;
		GtkWidget* respButton;
		GtkWidget* active;
		GtkWidget* useBounds;
		GtkWidget* radiusToggle;
		GtkWidget* radiusEntry;
		GtkWidget* timeIntToggle;
		GtkWidget* timeIntEntry;
		GtkWidget* timeUnitLabel;
		GtkWidget* magnToggle;
		GtkWidget* magnEntry;
		GtkWidget* falloffToggle;
		GtkWidget* falloffEntry;
		GtkWidget* timerTypeToggle;
		GtkWidget* addMenuItem;
		GtkWidget* deleteMenuItem;
	} _propertyWidgets;

	GtkWidget* _contextMenu;

public:
	/** greebo: Constructor creates all the widgets
	 */
	StimEditor(StimTypes& stimTypes);

	/** greebo: Sets the new entity (is called by the StimResponseEditor class)
	 */
	virtual void setEntity(SREntityPtr entity);

	/** greebo: Adds a new stim to the list
	 */
	void addStim();

	/** greebo: Updates the widgets (e.g. after a selection change) 
	 */
	void update();

private:
	/** greebo: Updates the associated text fields when a check box
	 * 			is toggled.
	 */
	void checkBoxToggled(GtkToggleButton* toggleButton);

	/** greebo: Updates the stim according to the given entry box 
	 */
	void entryChanged(GtkEditable* editable);

	/** greebo: As the name states, this creates the context menu widgets.
	 */
	void createContextMenu();

	/** greebo: Creates the option checkboxes and entry widgets
	 */
	GtkWidget* createPropertyWidgets();

	/** greebo: Gets called when the stim selection gets changed 
	 */
	virtual void selectionChanged();

	void openContextMenu(GtkTreeView* view);
	void removeItem(GtkTreeView* view);

	/** greebo: Creates all the widgets
	 */
	void populatePage();
	
	// Context menu GTK callbacks
	static void onContextMenuAdd(GtkWidget* w, StimEditor* self);
	static void onContextMenuDelete(GtkWidget* w, StimEditor* self);
};

} // namespace ui

#endif /*STIMEDITOR_H_*/
