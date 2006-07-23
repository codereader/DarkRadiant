#ifndef ENTITYCLASSCHOOSER_H_
#define ENTITYCLASSCHOOSER_H_

#include <gtk/gtk.h>

#include "generic/vector.h"

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

private:

	/* Widget construction helpers */
	
	GtkWidget* createTreeView();
	GtkWidget* createButtonPanel();

	/* GTK callbacks */
	
	// Called when close button is clicked, ensure that window is hidden
	// not destroyed.
	static void callbackHide(GtkWidget*, GdkEvent*, EntityClassChooser*);

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
