#ifndef ENTITYINSPECTOR_H_
#define ENTITYINSPECTOR_H_

#include "PropertyEditor.h"

#include <gtk/gtkliststore.h>
#include <gtk/gtkwidget.h>
#include <gtk/gtkmenuitem.h>
#include <gtk/gtktogglebutton.h>

#include "iselection.h"

#include "gtkutil/idledraw.h"

/* FORWARD DECLS */

class Entity;
class Selectable;

namespace ui {

namespace {
	
	// Data structure to store the type (vector3, text etc) and the options
	// string for a single property.
	struct PropertyParms {
		std::string type;
		std::string options;
	};

	// Map of property names to PropertyParms
	typedef std::map<std::string, PropertyParms> PropertyParmMap;
	
}
	

/* The EntityInspector class represents the GTK dialog for editing properties
 * on the selected game entity. The class is implemented as a singleton and
 * contains a method to return the current instance.
 */

class EntityInspector :
 	public SelectionSystem::Observer
{
private:

	// Currently selected entity
	Entity* _selectedEntity;

	// Main EntityInspector widget
    GtkWidget* _widget; 
    
	// Frame to contain the Property Editor    
    GtkWidget* _editorFrame;
    
    // Key list store and view
    GtkListStore* _listStore;
    GtkWidget* _treeView;

	// Key and value edit boxes
	GtkWidget* _keyEntry;
	GtkWidget* _valEntry;

	// Context menu main widget and items
	GtkWidget* _contextMenu;
	GtkWidget* _addKeyMenuItem;
	GtkWidget* _delKeyMenuItem;
	
	// Currently displayed PropertyEditor
	PropertyEditor* _currentPropertyEditor;

    // GtkUtil IdleDraw class. This allows redraw calls to be scheduled for
    // when GTK is idle.
    IdleDraw _idleDraw;
    
    // Whether to show inherited properties or not
    bool _showInherited;

private:

    // Utility functions to construct the Gtk components

    GtkWidget* createDialogPane(); // bottom widget pane 
    GtkWidget* createTreeViewPane(); // tree view for selecting attributes
    void createContextMenu();

	// Utility function to retrieve the string selection from the given column in the
	// list store
	std::string getListSelection(int col);

    /* GTK CALLBACKS */

    static void callbackTreeSelectionChanged(GtkWidget* widget, EntityInspector* self);

	static void _onKeyEntryActivate(GtkWidget*, EntityInspector*);
	static void _onValEntryActivate(GtkWidget*, EntityInspector*);
	static void _onSetProperty(GtkWidget*, EntityInspector*);    

	static bool _onPopupMenu(GtkWidget*, GdkEventButton*, EntityInspector*);
	static void _onDeleteProperty(GtkMenuItem*, EntityInspector*);
	static void _onAddProperty(GtkMenuItem*, EntityInspector*);

	static void _onToggleShowInherited(GtkToggleButton*, EntityInspector*);

    // Routines to populate the TreeStore with the keyvals attached to the
    // currently-selected object. 
    void refreshTreeModel(); 
    void appendClassProperties();

	// Update the GTK components when a new selection is made in the tree view
    void treeSelectionChanged();
    
	// Update the currently selected entity pointer. This function returns true
	// if a single Entity is selected, and false if either a non-Entity or more
	// than one object is selected.
	bool updateSelectedEntity();

	// Set the keyval on the object from the entry and value textboxes
	void setPropertyFromEntries();

	// Static map of property names to PropertyParms objects
	const PropertyParmMap& getPropertyMap();

public:

    // Constructor
    EntityInspector();

    // Return or create the singleton instance
    static EntityInspector& getInstance();

    // Get the Gtk Widget for display in the main application
    GtkWidget* getWidget();

    // Inform the IdleDraw to invoke a redraw when idle
    void queueDraw();
    
    // Redraw the GUI elements. Called by the IdleDraw object when GTK is idle
    // and a queueDraw request has been passed.
    void callbackRedraw();
    
	// Callback used by the EntityCreator when a key value changes on an entity
    static void keyValueChanged();

	/** greebo: Gets called by the RadiantSelectionSystem upon selection change.
	 */
	void selectionChanged(scene::Instance& instance);

};

} // namespace ui

#endif /*ENTITYINSPECTOR_H_*/
