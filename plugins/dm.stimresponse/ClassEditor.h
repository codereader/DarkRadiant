#ifndef CLASSEDITOR_H_
#define CLASSEDITOR_H_

#include "SREntity.h"

typedef struct _GtkWidget GtkWidget;
typedef struct _GtkTreeView GtkTreeView;
typedef struct _GtkTreeSelection GtkTreeSelection;
typedef struct _GdkEventKey GdkEventKey;
typedef struct _GdkEventButton GdkEventButton;

namespace ui {

class ClassEditor
{
protected:
	GtkWidget* _pageVBox;
	
	GtkWidget* _list;
	GtkTreeSelection* _selection;
	
	// The entity object we're editing
	SREntityPtr _entity;

public:
	/** greebo: Constructs the shared widgets, but does not pack them
	 */
	ClassEditor();
	
	/** greebo: Operator cast to widget to pack this page into
	 * 			a notebook tab or other parent widget.
	 */
	virtual operator GtkWidget*() = 0;
	
	/** greebo: Sets the new entity (is called by the StimResponseEditor class)
	 */
	virtual void setEntity(SREntityPtr entity);

protected:
	/** greebo: Gets called when the list selection changes
	 */
	virtual void selectionChanged() = 0;

	/** greebo: Opens the context menu. The treeview widget this event
	 * 			has been happening on gets passed so that the correct
	 * 			menu can be displayed (in the case of multiple possible treeviews).
	 */
	virtual void openContextMenu(GtkTreeView* view) = 0;
	
	/** greebo: Attempts to delete the item from the passed treeview.
	 */
	virtual void removeItem(GtkTreeView* view) = 0;

	// GTK Callback for Stim/Response selection changes
	static void onSRSelectionChange(GtkTreeSelection* treeView, ClassEditor* self);
	// The keypress handler for catching the keys in the treeview
	static gboolean onTreeViewKeyPress(GtkTreeView* view,GdkEventKey* event, ClassEditor* self);
	// Release-event opens the context menu for right clicks
	static gboolean onTreeViewButtonRelease(GtkTreeView* view, GdkEventButton* ev, ClassEditor* self);
};

} // namespace ui

#endif /*CLASSEDITOR_H_*/
