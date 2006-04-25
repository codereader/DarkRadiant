#include "EntityInspector.h"
#include "ientity.h"

#include <iostream>

namespace ui {

// Return the singleton EntityInspector instance, creating it if it is not yet
// created. Single-threaded design.

EntityInspector* EntityInspector::getInstance() {
    static EntityInspector _instance;
    return &_instance;
}

// Return the Gtk widget for the EntityInspector dialog. The GtkWidget is
// constructed on demand, since the actual dialog may never be shown.

GtkWidget* EntityInspector::getWidget() {
    if (_widget == NULL) 
        constructUI();
    return _widget;
}

// Create the actual UI components for the EntityInspector dialog

void EntityInspector::constructUI() {
    _widget = gtk_vbox_new(FALSE, 6);
    
    gtk_box_pack_end(GTK_BOX(_widget), createDialogPane(), FALSE, FALSE, 6);
    
    gtk_widget_show_all(_widget);
    
    // Set the function to call when a keyval is changed. This is a requirement
    // of the EntityCreator interface.
    GlobalEntityCreator().setKeyValueChangedFunc(EntityInspector::redraw);
}

// Create the dialog pane

GtkWidget* EntityInspector::createDialogPane() {
    GtkWidget* vbx = gtk_vbox_new(FALSE, 0);
    _editorFrame = gtk_frame_new("Edit property");
    gtk_box_pack_start(GTK_BOX(vbx), _editorFrame, FALSE, FALSE, 6);
    gtk_widget_set_size_request(vbx, 0, 250);
    return vbx;
}

// Redraw the GUI elements, such as in response to a key/val change on the
// selected entity. Note that this function is static as it is accessed via
// a function pointer in the GlobalEntityCreator.

void EntityInspector::redraw() {}

} // namespace ui
