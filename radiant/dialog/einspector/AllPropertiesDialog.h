#ifndef ALLPROPERTIESDIALOG_H_
#define ALLPROPERTIESDIALOG_H_

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

    // Main GTK widget (window)
    GtkWindow* _window;
    
private:

    // Callback to cancel the dialog and hide the window
    static void callbackCancel(GtkWidget* widget, AllPropertiesDialog* self);

    // Callback when close box is clicked
    static void callbackDestroy(GtkWidget* widget, GdkEvent* event, AllPropertiesDialog* self);

public:

    // Constructor. Create the GTK widgets.
	AllPropertiesDialog();
    
    // Show the GTK widgets and start receiving user input.
    void show();
    
    // Destructor. Hide and destroy the GTK widgets.
    ~AllPropertiesDialog();
};

}

#endif /*ALLPROPERTIESDIALOG_H_*/
