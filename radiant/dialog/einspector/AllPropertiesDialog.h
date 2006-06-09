#ifndef ALLPROPERTIESDIALOG_H_
#define ALLPROPERTIESDIALOG_H_

#include "ientity.h"

#include <gtk/gtk.h>

#include <string>

namespace ui
{

/* CONSTANTS
 */
 
namespace {

    const int DIALOG_WIDTH = 400;
    const int DIALOG_HEIGHT = 450;

    const std::string ALL_PROPERTIES_TITLE = "All properties";   
        
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
        N_COLUMNS
    };

    // Main GTK widget (window)
    GtkWindow* _window;
 
    // The GtkListStore with keys and values
    GtkListStore* _listStore;
 
    // The Entity to edit
    Entity* _entity;
    
private:

    // Construct the GtkTreeView
    GtkWidget* createTreeView();

    // Callback to cancel the dialog and hide the window
    static void callbackCancel(GtkWidget* widget, AllPropertiesDialog* self);

    // Callback when close box is clicked
    static void callbackDestroy(GtkWidget* widget, GdkEvent* event, AllPropertiesDialog* self);

    // Destroy self and all owned widgets
    void destroy();
    

public:

    // Constructor. Create the GTK widgets.
	AllPropertiesDialog();
    
    // Show the GTK widgets and start receiving user input, for the given
    // entity.
    void show(Entity* ent);
    
    // Destructor. Hide and destroy the GTK widgets.
    ~AllPropertiesDialog();
};

}

#endif /*ALLPROPERTIESDIALOG_H_*/
