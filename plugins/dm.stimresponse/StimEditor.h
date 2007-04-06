#ifndef STIMEDITOR_H_
#define STIMEDITOR_H_

#include "ClassEditor.h"

namespace ui {

class StimEditor :
	public ClassEditor
{
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

public:
	/** greebo: Constructor creates all the widgets
	 */
	StimEditor(StimTypes& stimTypes);

	/** greebo: Sets the new entity (is called by the StimResponseEditor class)
	 */
	virtual void setEntity(SREntityPtr entity);

private:
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
};

} // namespace ui

#endif /*STIMEDITOR_H_*/
