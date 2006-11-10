#ifndef ENTITYCLASSCHOOSER_H_
#define ENTITYCLASSCHOOSER_H_

#include <gtk/gtk.h>

#include "math/Vector3.h"

namespace ui
{

	namespace {
		
		/* CONSTANTS */
		
		const char* ECLASS_CHOOSER_TITLE = "Create entity";
		
	}

/** Dialog window displaying a tree of Entity Classes, allowing the selection
 * of a class to create at the current location. This widget is displayed
 * by the OrthoContextMenu.
 */

class EntityClassChooser
{
private:

	// Main dialog window
	GtkWidget* _widget;
	
	// Tree model holding the classnames
	GtkTreeStore* _treeStore;
	
	// GtkTreeSelection holding the currently-selected classname
	GtkTreeSelection* _selection;

	// Add button. Needs to be a member since we enable/disable it in the
	// selectionchanged callback.
	GtkWidget* _addButton;

	// The 3D coordinates of the point where the entity must be created.
	Vector3 _lastPoint;

private:

	/* Widget construction helpers */
	
	GtkWidget* createTreeView();
	GtkWidget* createButtonPanel();

	/* GTK callbacks */
	
	// Called when close button is clicked, ensure that window is hidden
	// not destroyed.
	static void callbackHide(GtkWidget*, GdkEvent*, EntityClassChooser*);

	// Button callbacks
	static void callbackCancel(GtkWidget*, EntityClassChooser*);
	static void callbackAdd(GtkWidget*, EntityClassChooser*);

	// Check when the selection changes, disable the add button if there
	// is nothing selected.
	static void callbackSelectionChanged(GtkWidget*, EntityClassChooser*);

public:

	/// Constructor. Creates the GTK widgets.
	EntityClassChooser();
	
	/* Show the dialog and choose an entity class.
	 * 
	 * @param point
	 * The point at which the new entity should be created
	 */
	
	void show(const Vector3& point);
	
	/** Obtain the singleton instance and show it, passing in the
	 * required entity creation coordinates.
	 * 
	 * @param point
	 * The point at which the new entity should be created
	 */
	
	static void displayInstance(const Vector3& point);
	
};

}

#endif /*ENTITYCLASSCHOOSER_H_*/
