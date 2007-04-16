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
	
	GtkWidget* _pageHBox;
	GtkWidget* _list;
	GtkTreeSelection* _selection;
	
	StimTypes& _stimTypes;

public:
	/** greebo: Constructor creates all the widgets
	 */
	CustomStimEditor(StimTypes& stimTypes);

	operator GtkWidget*();
	
private:
	/** greebo: As the name states, this creates the context menu widgets.
	 */
	void createContextMenu();

	/** greebo: Creates all the widgets
	 */
	void populatePage();
};

} // namespace ui

#endif /*CUSTOMSTIMEDITOR_H_*/
