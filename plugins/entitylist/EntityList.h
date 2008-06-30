#ifndef ENTITYLIST_H_
#define ENTITYLIST_H_

#include "iselection.h"
#include "iscenegraph.h"
#include "iradiant.h"
#include "imodule.h"
#include "gtkutil/WindowPosition.h"
#include "gtkutil/window/PersistentTransientWindow.h"
#include "GraphTreeModel.h"

typedef struct _GtkTreeView GtkTreeView;
typedef struct _GtkTreeSelection GtkTreeSelection;
typedef struct _GtkTreeIter GtkTreeIter;
typedef struct _GtkTreePath GtkTreePath;
typedef struct _GtkTreeModel GtkTreeModel;
typedef struct _GtkToggleButton GtkToggleButton;

namespace ui {

class EntityList;
typedef boost::shared_ptr<EntityList> EntityListPtr;

class EntityList : 
	public gtkutil::PersistentTransientWindow,
	public SelectionSystem::Observer,
	public RadiantEventListener
{
	// The main tree view
	GtkTreeView* _treeView;
	GtkTreeSelection* _selection;
	
	// The GraphTreeModel instance
	GraphTreeModel _treeModel; 

	// The small checkbox in the lower half
	GtkWidget* _focusOnSelectedEntityToggle;

	gtkutil::WindowPosition _windowPosition;

	bool _callbackActive;

private:
	// This is where the static shared_ptr of the singleton instance is held.
	static EntityListPtr& InstancePtr();

	// TransientWindow callbacks
	virtual void _preHide();
	virtual void _preShow();
	
	/** greebo: Creates the widgets
	 */
	void populateWindow();

	/** greebo: Updates the treeview contents
	 */
	void update();

	/** greebo: SelectionSystem::Observer implementation.
	 * 			Gets notified as soon as the selection is changed.
	 */
	void selectionChanged(const scene::INodePtr& node, bool isComponent);

	/** greebo: Toggles the visibility of this dialog
	 */
	void toggleWindow();

	static void onRowExpand(GtkTreeView* view, GtkTreeIter* iter, GtkTreePath* path, EntityList* self);
	
	static gboolean onSelection(GtkTreeSelection *selection, GtkTreeModel *model, 
								GtkTreePath *path, gboolean path_currently_selected, gpointer data);

	static void onFocusSelectionToggle(GtkToggleButton* togglebutton, EntityList* self);

	// (private) Constructor, creates all the widgets
	EntityList();
		
public:
	/** greebo: Shuts down this dialog, safely disconnects it
	 * 			from the EventManager and the SelectionSystem.
	 * 			Saves the window information to the Registry.
	 */
	virtual void onRadiantShutdown();
	
	/** greebo: Toggles the window (command target).
	 */
	static void toggle();
	
	/** greebo: Contains the static instance. Use this
	 * 			to access the other members
	 */
	static EntityList& Instance();
	
	// Destroys the singleton instance
	static void destroyInstance();
};

} // namespace ui

#endif /*ENTITYLIST_H_*/
