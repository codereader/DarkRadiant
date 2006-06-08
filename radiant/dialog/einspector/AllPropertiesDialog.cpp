#include "AllPropertiesDialog.h"

#include "mainframe.h"

#include <iostream>

namespace ui
{
    
// Ctor

AllPropertiesDialog::AllPropertiesDialog():
    _window(GTK_WINDOW(gtk_window_new(GTK_WINDOW_TOPLEVEL)))
{
    std::cout << "AllPropertiesDialog constructing" << std::endl;
    gtk_window_set_transient_for(_window, MainFrame_getWindow());
    gtk_window_set_modal(_window, TRUE);
    gtk_window_set_position(_window, GTK_WIN_POS_CENTER_ON_PARENT);
    gtk_window_set_title(_window, ALL_PROPERTIES_TITLE.c_str());
    gtk_window_set_default_size(_window, DIALOG_WIDTH, DIALOG_HEIGHT);

    GtkWidget* vbox = gtk_vbox_new(FALSE, 0);

    // Add the TreeView to the top of the dialog
    
    GtkWidget* _treeView = gtk_tree_view_new();
    GtkWidget* scrollWin = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrollWin), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrollWin), GTK_SHADOW_ETCHED_IN);
    gtk_container_add(GTK_CONTAINER(scrollWin), _treeView);    
    
    gtk_box_pack_start(GTK_BOX(vbox), scrollWin, TRUE, TRUE, 0);

    // Buttons box
    
    GtkWidget* butbox = gtk_hbox_new(FALSE, 0);
    
    GtkWidget* okButton = gtk_button_new_from_stock(GTK_STOCK_OK);
    GtkWidget* cancelButton = gtk_button_new_from_stock(GTK_STOCK_CANCEL);
    GtkWidget* addButton = gtk_button_new_from_stock(GTK_STOCK_ADD);
    GtkWidget* removeButton = gtk_button_new_from_stock(GTK_STOCK_DELETE);
    
    gtk_box_pack_end(GTK_BOX(butbox), okButton, FALSE, FALSE, 0);
    gtk_box_pack_end(GTK_BOX(butbox), cancelButton, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(butbox), addButton, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(butbox), removeButton, FALSE, FALSE, 0);

    g_signal_connect(G_OBJECT(cancelButton), "clicked", G_CALLBACK(callbackCancel), this);
    
    gtk_box_pack_end(GTK_BOX(vbox), butbox, FALSE, FALSE, 0);
    
    gtk_container_add(GTK_CONTAINER(_window), vbox);

}

// Dtor

AllPropertiesDialog::~AllPropertiesDialog() {
    std::cout << "AllPropertiesDialog destroying" << std::endl;
    gtk_widget_destroy(GTK_WIDGET(_window));
}

// Show GTK widgets

void AllPropertiesDialog::show() {
    g_signal_connect(G_OBJECT(_window), "delete_event", G_CALLBACK(callbackDestroy), this);
    gtk_widget_show_all(GTK_WIDGET(_window));
}

// Cancel callback function

void AllPropertiesDialog::callbackCancel(GtkWidget* widget, AllPropertiesDialog* self) {
    gtk_widget_hide(GTK_WIDGET(self->_window));
}

// Destroy callback

void AllPropertiesDialog::callbackDestroy(GtkWidget* widget, GdkEvent* event, AllPropertiesDialog* self) {
    std::cout << "callbackDestroy()" << std::endl;
    self->callbackCancel(widget, self);
}


}
