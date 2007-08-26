#ifndef ENTITYCLASSCHOOSER_H_
#define ENTITYCLASSCHOOSER_H_

#include <gtk/gtk.h>

#include "math/Vector3.h"

namespace {

    /* CONSTANTS */
    
    const char* ECLASS_CHOOSER_TITLE = "Create entity";
    const char* FOLDER_ICON = "folder16.png";
    const char* ENTITY_ICON = "cmenu_add_entity.png";
    
    // Registry XPath to lookup key that specifies the display folder
    const char* FOLDER_KEY_PATH = "game/entityChooser/displayFolderKey";

    // Tree column enum
    enum {
        NAME_COLUMN,
        ICON_COLUMN,
        DIR_FLAG_COLUMN,
        N_COLUMNS
    };

}

namespace ui
{

/** Dialog window displaying a tree of Entity Classes, allowing the selection
 * of a class to create at the current location. This widget is displayed
 * by the OrthoContextMenu.
 */

class EntityClassChooser
{
private:

	// Main dialog window
	GtkWidget* _widget;
	
	// Tree model holding the classnames, and the corresponding treeview
	GtkTreeStore* _treeStore;
	GtkWidget* _treeView;
	
	// GtkTreeSelection holding the currently-selected classname
	GtkTreeSelection* _selection;

	// Usage information textview
	GtkWidget* _usageTextView;

	// Add button. Needs to be a member since we enable/disable it in the
	// selectionchanged callback.
	GtkWidget* _addButton;

	// The 3D coordinates of the point where the entity must be created.
	Vector3 _lastPoint;

private:

	/* Widget construction helpers */
	
	GtkWidget* createTreeView();
	GtkWidget* createUsagePanel();
	GtkWidget* createButtonPanel();

	// Update the usage panel with information from the provided entityclass
	void updateUsageInfo(const std::string& eclass);

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
