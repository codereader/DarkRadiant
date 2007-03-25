#ifndef ENTITYLIST_H_
#define ENTITYLIST_H_

#include "iselection.h"
#include "gtkutil/WindowPosition.h"

typedef struct _GtkTreeView GtkTreeView;
typedef struct _GtkTreeSelection GtkTreeSelection;
typedef struct _GtkTreeIter GtkTreeIter;
typedef struct _GtkTreePath GtkTreePath;
typedef struct _GtkTreeModel GtkTreeModel;

namespace ui {

class EntityList :
	public SelectionSystem::Observer
{
	// The main dialog window
	GtkWidget* _dialog;
	GtkTreeView* _treeView;
	GtkTreeSelection* _selection;
	
	// The treemodel (is hosted externally in scenegraph - legacy)
	GtkTreeModel* _treeModel;

	gtkutil::WindowPosition _windowPosition;

	bool _callbackActive;

public:
	// Constructor, creates all the widgets
	EntityList();
	
	/** greebo: Shuts down this dialog, safely disconnects it
	 * 			from the EventManager and the SelectionSystem.
	 * 			Saves the window information to the Registry.
	 */
	void shutdown();
	
	/** greebo: Toggles the window (command target).
	 */
	static void toggle();
	
	/** greebo: Contains the static instance. Use this
	 * 			to access the other members
	 */
	static EntityList& Instance();
	
private:
	/** greebo: Creates the widgets
	 */
	void populateWindow();

	/** greebo: Updates the treeview contents
	 */
	void update();

	/** greebo: SelectionSystem::Observer implementation.
	 * 			Gets notified as soon as the selection is changed.
	 */
	void selectionChanged(scene::Instance& instance);

	/** greebo: Toggles the visibility of this dialog
	 */
	void toggleWindow();

	// The callback to catch the delete event (toggles the window)
	static gboolean onDelete(GtkWidget* widget, GdkEvent* event, EntityList* self);
	static void onRowExpand(GtkTreeView* view, GtkTreeIter* iter, GtkTreePath* path, EntityList* self);
	
	static gboolean onSelection(GtkTreeSelection *selection, GtkTreeModel *model, 
								GtkTreePath *path, gboolean path_currently_selected, gpointer data);

	static gboolean modelUpdater(GtkTreeModel* model, GtkTreePath* path, 
								 GtkTreeIter* iter, gpointer data);
};

} // namespace ui

#endif /*ENTITYLIST_H_*/
