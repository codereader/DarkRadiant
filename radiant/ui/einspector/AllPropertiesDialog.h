#ifndef ALLPROPERTIESDIALOG_H_
#define ALLPROPERTIESDIALOG_H_

#include "EntityInspector.h"

#include "ientity.h"

#include <gtk/gtk.h>

#include <string>

namespace ui
{

/* CONSTANTS
 */
 
namespace {

    const int DIALOG_WIDTH = 512;
    const int DIALOG_HEIGHT = 450;

    const std::string ALL_PROPERTIES_TITLE = "All properties";
    const std::string RECOGNISED_PROPERTY_IMG = "recprop.png";
    const std::string UNRECOGNISED_PROPERTY_IMG = "unrecprop.png";
        
}

/* AllPropertiesDialog
 * 
 * This is a GTK dialog which allows the manual inspection and configuration
 * of all properties on an entity, no matter whether the properties are
 * identified as members of a group or not. All properties are edited
 * directly as text, similar to the D3Radiant entity inspector interface
 */

class AllPropertiesDialog
{
private:

    // Enumeration for TreeView columns
    enum {
        KEY_COLUMN,
        VALUE_COLUMN,
        TEXT_COLOUR_COLUMN,
        ICON_COLUMN,
        N_COLUMNS
    };

    // Main GTK widget (window)
    GtkWindow* _window;
 
    // The GtkListStore with keys and values
    GtkListStore* _listStore;
    
    // Gtk tree view
    GtkWidget* _treeView;
 
    // The Entity to edit
    Entity* _entity;
 
    // Reference to set of known properties
    KnownPropertySet& _knownProps;
    
private:

    // Construct the GtkTreeView
    GtkWidget* createTreeView();

    // Callback to cancel the dialog and hide the window
    static void callbackCancel(GtkWidget* widget, AllPropertiesDialog* self);

    // Callback when close box is clicked
    static void callbackDestroy(GtkWidget* widget, GdkEvent* event, AllPropertiesDialog* self);

    // Callback when OK button is clicked
    static void callbackOK(GtkWidget* widget, AllPropertiesDialog* self);

    // Callback on completion of cell editing
    static void callbackEditDone(GtkWidget* widget, const char* path, const char* newText, AllPropertiesDialog* self);

    // Callback when Add button clicked
    static void callbackAdd(GtkWidget* widget, AllPropertiesDialog* self);
    
    // callback when Delete button clicked
    static void callbackDelete(GtkWidget* widget, AllPropertiesDialog* self);
    
    // Destroy self and all owned widgets
    void destroy();
    

public:

    // Constructor. Create the GTK widgets, and accept a reference to the set
    // of known properties so they can be displayed differently in the list.
	AllPropertiesDialog(KnownPropertySet& set);
    
    // Show the GTK widgets and start receiving user input, for the given
    // entity.
    void showForEntity(Entity* ent);
    
    // Destructor. Hide and destroy the GTK widgets.
    ~AllPropertiesDialog();
};

}

#endif /*ALLPROPERTIESDIALOG_H_*/
