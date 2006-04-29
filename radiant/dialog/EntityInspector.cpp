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
    gtk_box_pack_start(GTK_BOX(_widget), createTreeViewPane(), FALSE, FALSE, 6);
    
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

// Create the TreeView pane

GtkWidget* EntityInspector::createTreeViewPane() {
    GtkWidget* vbx = gtk_vbox_new(FALSE, 0);

    // Initialise the instance TreeStore
    _treeStore = gtk_tree_store_new(1, G_TYPE_STRING);
    
    // Add some test values. TODO: test code
    GtkTreeIter tempIter;
    gtk_tree_store_append(_treeStore, &tempIter, NULL);
    gtk_tree_store_set(_treeStore, &tempIter, 0, "Basic", -1);
    gtk_tree_store_append(_treeStore, &tempIter, NULL);
    gtk_tree_store_set(_treeStore, &tempIter, 0, "Light", -1);

    // Create the TreeView widget and link it to the model
    _treeView = gtk_tree_view_new_with_model(GTK_TREE_MODEL(_treeStore));

    // Add columns to the TreeView
    GtkCellRenderer* textRenderer = gtk_cell_renderer_text_new();
    GtkTreeViewColumn* nameCol = 
        gtk_tree_view_column_new_with_attributes("Properties",
                                                 textRenderer,
                                                 "text",
                                                 0,
                                                 NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(_treeView), nameCol);                                                                        

    // Set up the signals
    g_signal_connect(G_OBJECT(_treeView), "cursor-changed", G_CALLBACK(callbackTreeSelectionChanged), this);
                                                                         
    // Embed the TreeView in a scrolled viewport
    GtkWidget* scrollWin = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrollWin), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_size_request(scrollWin, 260, 180);
    gtk_container_add(GTK_CONTAINER(scrollWin), _treeView);    

    gtk_box_pack_start(GTK_BOX(vbx), scrollWin, FALSE, FALSE, 6);
    return vbx;    
}

// Redraw the GUI elements, such as in response to a key/val change on the
// selected entity. 

void EntityInspector::doRedraw() {
    std::cout << "EntityInspector::doRedraw()" << std::endl;
}

// Static function to get the singleton instance and invoke its redraw function.
// This is necessary since the EntityCreator interface requires a pointer to 
// a non-member function to call when a keyval is changed.

void EntityInspector::redraw() {
    getInstance()->queueDraw();
}

// Pass on a queueDraw request to the contained IdleDraw object.

void EntityInspector::queueDraw() {
    _idleDraw.queueDraw();
}

/* GTK CALLBACKS */

void EntityInspector::callbackTreeSelectionChanged(GtkWidget* widget, EntityInspector* self) {
    std::cout << "Selection changed" << std::endl;   
}

} // namespace ui
