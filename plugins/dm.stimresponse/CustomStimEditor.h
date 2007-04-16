#ifndef CUSTOMSTIMEDITOR_H_
#define CUSTOMSTIMEDITOR_H_

#include "StimTypes.h"

typedef struct _GtkWidget GtkWidget;
typedef struct _GtkTreeSelection GtkTreeSelection;

namespace ui {

class CustomStimEditor
{
	struct ListContextMenu {
		GtkWidget* menu;
		GtkWidget* remove;
		GtkWidget* add;
	} _contextMenu;

	struct ListButtons {
		GtkWidget* add;
		GtkWidget* remove;
	} _listButtons;
	
	// The overall hbox of this page
	GtkWidget* _pageHBox;
	
	// The treeview and its selection
	GtkWidget* _list;
	GtkTreeSelection* _selection;
	
	// Reference to the helper object (owned by StimResponseEditor)
	StimTypes& _stimTypes;

public:
	/** greebo: Constructor creates all the widgets
	 */
	CustomStimEditor(StimTypes& stimTypes);

	operator GtkWidget*();
	
private:
	/** greebo: Selects the given ID in the stim type list
	 */
	void selectId(int id);

	/** greebo: Adds/removes a (selected) stim type
	 */
	void addStimType();
	void removeStimType();

	/** greebo: Widget creators
	 */
	void createContextMenu();
	GtkWidget* createListButtons();

	/** greebo: Creates all the widgets
	 */
	void populatePage();

	// GTK Callbacks
	static void onAddStimType(GtkWidget* button, CustomStimEditor* self);
	static void onRemoveStimType(GtkWidget* button, CustomStimEditor* self);
};

} // namespace ui

#endif /*CUSTOMSTIMEDITOR_H_*/
