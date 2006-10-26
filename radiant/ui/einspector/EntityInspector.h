#ifndef ENTITYINSPECTOR_H_
#define ENTITYINSPECTOR_H_

#include "PropertyEditor.h"

#include <gtk/gtkliststore.h>
#include <gtk/gtkwidget.h>

#include "gtkutil/idledraw.h"

/* FORWARD DECLS */

class Entity;
class Selectable;

namespace ui {

namespace {
	
	// Types
	typedef std::map<std::string, std::string> StringMap;
	
}
	

/* The EntityInspector class represents the GTK dialog for editing properties
 * on the selected game entity. The class is implemented as a singleton and
 * contains a method to return the current instance.
 */

class EntityInspector
{
private:

	// Currently selected entity
	Entity* _selectedEntity;

    // The Gtk dialog widgets

    GtkWidget* _widget; 
    
    GtkWidget* _editorFrame;
    GtkWidget* _selectionTreeView;
    
    GtkListStore* _listStore;
    GtkWidget* _treeView;

	// Key and value edit boxes
	GtkWidget* _keyEntry;
	GtkWidget* _valEntry;

	// Currently displayed PropertyEditor
	PropertyEditor* _currentPropertyEditor;

    // GtkUtil IdleDraw class. This allows redraw calls to be scheduled for
    // when GTK is idle.
    IdleDraw _idleDraw;

private:

    // Utility functions to construct the Gtk components

    GtkWidget* createDialogPane(); // bottom widget pane 
    GtkWidget* createTreeViewPane(); // tree view for selecting attributes

    /* GTK CALLBACKS */
    static void callbackTreeSelectionChanged(GtkWidget* widget, EntityInspector* self);
	static void _onSetProperty(GtkWidget*, EntityInspector*);    
	static void _onKeyEntryActivate(GtkWidget*, EntityInspector*);
	static void _onValEntryActivate(GtkWidget*, EntityInspector*);

    // Routines to populate the TreeStore with the keyvals attached to the
    // currently-selected object. 
    void refreshTreeModel(); 

	// Update the GTK components when a new selection is made in the tree view
    void treeSelectionChanged();
    
	// Update the currently selected entity pointer. This function returns true
	// if a single Entity is selected, and false if either a non-Entity or more
	// than one object is selected.
	bool updateSelectedEntity();

	// Set the keyval on the object from the entry and value textboxes
	void setPropertyFromEntries();

public:

    // Constructor
    EntityInspector();

	// Static map of property names to types
	const StringMap& getPropertyMap();

    // Return or create the singleton instance
    static EntityInspector& getInstance();

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
