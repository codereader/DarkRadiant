#ifndef CUSTOMSTIMEDITOR_H_
#define CUSTOMSTIMEDITOR_H_

#include "StimTypes.h"
#include "SREntity.h"

typedef struct _GtkWidget GtkWidget;
typedef struct _GtkTreeModel GtkTreeModel;
typedef struct _GtkTreeSelection GtkTreeSelection;
typedef struct _GtkTreeView GtkTreeView;
typedef struct _GtkEditable GtkEditable;
typedef struct _GdkEventButton GdkEventButton;

namespace ui {
	
class CustomStimEditor
{
	struct PropertyWidget {
		GtkWidget* vbox;
		GtkWidget* nameLabel;
		GtkWidget* nameEntry;
	} _propertyWidgets;
	
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
	
	// The filtered liststore (a GtkTreeModelFilter)
	GtkTreeModel* _customStimStore;
	
	// The treeview and its selection
	GtkWidget* _list;
	GtkTreeSelection* _selection;
	
	// Reference to the helper object (owned by StimResponseEditor)
	StimTypes& _stimTypes;
	
	// To avoid GTK callback loops
	bool _updatesDisabled;
	
	// The entity we're working on
	SREntityPtr _entity;
	
	GtkWidget* _parentWindow;

public:
	/** greebo: Constructor creates all the widgets
	 */
	CustomStimEditor(GtkWidget* parentWindow, StimTypes& stimTypes);

	operator GtkWidget*();
	
	/** greebo: Sets the new entity (is called by the subclasses)
	 */
	void setEntity(SREntityPtr entity);
	
private:
	/** greebo: Updates the property widgets on selection change
	 */
	void update();

	/** greebo: Gets called when an entry box changes, this can be
	 * 			overriden by the subclasses, if this is needed
	 */
	void entryChanged(GtkEditable* editable);

	/** greebo: Returns the ID of the currently selected stim type
	 * 		
	 * @returns: the id (number) of the selected stim or -1 on failure 
	 */
	int getIdFromSelection();

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
	static void onEntryChanged(GtkEditable* editable, CustomStimEditor* self);
	static void onSelectionChange(GtkTreeSelection* selection, CustomStimEditor* self);
	
	// Context menu
	// Release-event opens the context menu for right clicks
	static gboolean onTreeViewButtonRelease(GtkTreeView* view, GdkEventButton* ev, CustomStimEditor* self);
	static void onContextMenuAdd(GtkWidget* w, CustomStimEditor* self);
	static void onContextMenuDelete(GtkWidget* w, CustomStimEditor* self);
};

} // namespace ui

#endif /*CUSTOMSTIMEDITOR_H_*/
