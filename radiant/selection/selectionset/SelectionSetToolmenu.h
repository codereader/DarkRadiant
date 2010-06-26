#ifndef _SELECTION_SET_TOOL_MENU_H_
#define _SELECTION_SET_TOOL_MENU_H_

#include "iselectionset.h"
#include <boost/shared_ptr.hpp>

typedef struct _GtkToolItem GtkToolItem;
typedef struct _GtkWidget GtkWidget;
typedef struct _GtkComboBox GtkComboBox;
typedef struct _GtkEntry GtkEntry;
typedef struct _GtkListStore GtkListStore;
typedef struct _GtkToolButton GtkToolButton;

namespace selection
{

class SelectionSetToolmenu :
	public ISelectionSetManager::Observer
{
private:
	GtkToolItem* _toolItem;

	GtkListStore* _listStore;
	GtkToolItem* _clearSetsButton;

	GtkWidget* _entry;

public:
	SelectionSetToolmenu();

	~SelectionSetToolmenu();

	// Get the tool item widget for packing this control into a GtkToolbar
	GtkToolItem* getToolItem();

	// Observer implementation
	void onSelectionSetsChanged();

private:
	// Updates the available list items and widget sensitivity
	void update();

	static void onSelectionChanged(GtkComboBox* comboBox, SelectionSetToolmenu* self);
	static void onEntryActivated(GtkEntry* entry, SelectionSetToolmenu* self);
	static void onDeleteAllSetsClicked(GtkToolButton* toolbutton, SelectionSetToolmenu* self);
};
typedef boost::shared_ptr<SelectionSetToolmenu> SelectionSetToolmenuPtr;

} // namespace selection

#endif /* _SELECTION_SET_TOOL_MENU_H_ */
