#ifndef ENTITYINSPECTOR_H_
#define ENTITYINSPECTOR_H_

#include "gtkutil/idledraw.h"

#include "iselection.h"
#include "ientity.h"

#include "PropertyEditor.h"

#include <gtk/gtk.h>

namespace ui {

/* The EntityInspector class represents the GTK dialog for editing properties
 * on the selected game entity. The class is implemented as a singleton and
 * contains a method to return the current instance.
 */

class EntityInspector 
{
private:

    // TreeView column numbers
    
    enum {
        PROPERTY_NAME_COLUMN,
        PROPERTY_VALUE_COLUMN,
        PROPERTY_TYPE_COLUMN,
        PROPERTY_ICON_COLUMN,
        N_COLUMNS
    };

	// Currently selected entity
	Entity* _selectedEntity;

    // The Gtk dialog widgets

    GtkWidget* _widget; 
    
    GtkWidget* _editorFrame;
    GtkWidget* _selectionTreeView;
    
    GtkTreeStore* _treeStore;
    GtkWidget* _treeView;

	// Currently displayed PropertyEditor
	PropertyEditor* _currentPropertyEditor;

    // Utility functions to construct the Gtk components

    void constructUI();

    GtkWidget* createDialogPane(); // bottom widget pane 
    GtkWidget* createTreeViewPane(); // tree view for selecting attributes

    // GtkUtil IdleDraw class. This allows redraw calls to be scheduled for
    // when GTK is idle.
    IdleDraw _idleDraw;

    // GTK CALLBACKS
    // Must be static as they are called from a C-based API
    static void callbackTreeSelectionChanged(GtkWidget* widget, EntityInspector* self);

    // Routine to populate the TreeStore with the keyvals attached to the
    // currently-selected object. 
    void populateTreeModel();

	// Update the currently selected entity pointer. This function returns true
	// if a single Entity is selected, and false if either a non-Entity or more
	// than one object is selected.
	bool updateSelectedEntity();

public:

    // Constructor
    EntityInspector():
        // Set the IdleDraw instance to call the doRedraw function when
        // required
        _idleDraw(MemberCaller<EntityInspector, &EntityInspector::callbackRedraw>(*this)) {}

    // Return or create the singleton instance
    static EntityInspector* getInstance();

    // Get the Gtk Widget for display in the main application
    GtkWidget* getWidget();

    // Inform the IdleDraw to invoke a redraw when idle
    void queueDraw();
    
    // Redraw the GUI elements. Called by the IdleDraw object when GTK is idle
    // and a queueDraw request has been passed.
    void callbackRedraw();
    
    // Static class function to instigate a redraw. This is passed as a pointer
    // to the GlobalEntityCreator's setKeyValueChangedFunc function.
    static void redrawInstance();

    // Function to call when the current Selection is changed by the selection
    // system. Internally this function will just stimulate a redraw, but it
    // must take a reference to the Selectable object.
    static void selectionChanged(const Selectable&);

};

} // namespace ui

#endif /*ENTITYINSPECTOR_H_*/
