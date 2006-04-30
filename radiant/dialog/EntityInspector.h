#ifndef ENTITYINSPECTOR_H_
#define ENTITYINSPECTOR_H_

#include "gtkutil/idledraw.h"

#include "iselection.h"

#include <gtk/gtk.h>

namespace ui {

/* The EntityInspector class represents the GTK dialog for editing properties
 * on the selected game entity. The class is implemented as a singleton and
 * contains a method to return the current instance.
 */

class EntityInspector 
{
private:

    // The Gtk dialog widgets

    GtkWidget* _widget; 
    
    GtkWidget* _editorFrame;
    GtkWidget* _selectionTreeView;
    
    GtkTreeStore* _treeStore;
    GtkWidget* _treeView;

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

public:

    // Constructor
    EntityInspector():
        // Set the IdleDraw instance to call the doRedraw function when
        // required
        _idleDraw(MemberCaller<EntityInspector, &EntityInspector::doRedraw>(*this)) {}

    // Return or create the singleton instance
    static EntityInspector* getInstance();

    // Get the Gtk Widget for display in the main application
    GtkWidget* getWidget();

    // Inform the IdleDraw to invoke a redraw when idle
    void queueDraw();
    
    // Redraw the GUI elements. Called by the IdleDraw object when GTK is idle
    // and a queueDraw request has been passed.
    void doRedraw();
    
    // Static class function to instigate a redraw. This is passed as a pointer
    // to the GlobalEntityCreator's setKeyValueChangedFunc function.
    static void redraw();

    // Function to call when the current Selection is changed by the selection
    // system. Internally this function will just stimulate a redraw, but it
    // must take a reference to the Selectable object.
    static void selectionChanged(const Selectable&);

};

} // namespace ui

#endif /*ENTITYINSPECTOR_H_*/
