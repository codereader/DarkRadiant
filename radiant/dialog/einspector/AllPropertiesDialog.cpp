#include "AllPropertiesDialog.h"

#include "mainframe.h"

#include "gtkutil/image.h"

#include <iostream>

namespace ui
{
    
// Ctor

AllPropertiesDialog::AllPropertiesDialog(KnownPropertySet& set):
    _window(GTK_WINDOW(gtk_window_new(GTK_WINDOW_TOPLEVEL))),
    _entity(NULL),
    _knownProps(set)
{
    gtk_window_set_transient_for(_window, MainFrame_getWindow());
    gtk_window_set_modal(_window, TRUE);
    gtk_window_set_position(_window, GTK_WIN_POS_CENTER_ON_PARENT);
    gtk_window_set_title(_window, ALL_PROPERTIES_TITLE.c_str());
    gtk_window_set_default_size(_window, DIALOG_WIDTH, DIALOG_HEIGHT);
    g_signal_connect(G_OBJECT(_window), "delete_event", G_CALLBACK(callbackDestroy), this);

    GtkWidget* vbox = gtk_vbox_new(FALSE, 0);

    // Create and add the TreeView
    
    gtk_box_pack_start(GTK_BOX(vbox), createTreeView(), TRUE, TRUE, 0);

    // Buttons box
    
    GtkWidget* butbox = gtk_hbox_new(FALSE, 0);
    gtk_container_set_border_width(GTK_CONTAINER(butbox), 3);
    
    GtkWidget* okButton = gtk_button_new_from_stock(GTK_STOCK_OK);
    GtkWidget* cancelButton = gtk_button_new_from_stock(GTK_STOCK_CANCEL);
    GtkWidget* addButton = gtk_button_new_from_stock(GTK_STOCK_ADD);
    GtkWidget* removeButton = gtk_button_new_from_stock(GTK_STOCK_DELETE);
    
    gtk_box_pack_end(GTK_BOX(butbox), okButton, FALSE, FALSE, 0);
    gtk_box_pack_end(GTK_BOX(butbox), cancelButton, FALSE, FALSE, 3);
    gtk_box_pack_start(GTK_BOX(butbox), addButton, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(butbox), removeButton, FALSE, FALSE, 3);

    g_signal_connect(G_OBJECT(cancelButton), "clicked", G_CALLBACK(callbackCancel), this);
    
    gtk_box_pack_end(GTK_BOX(vbox), butbox, FALSE, FALSE, 0);
    
    gtk_container_add(GTK_CONTAINER(_window), vbox);

}

// Dtor

AllPropertiesDialog::~AllPropertiesDialog() {
    gtk_widget_hide(GTK_WIDGET(_window));
    gtk_widget_destroy(GTK_WIDGET(_window));
}

// Create and return the TreeView in a scrolled window

GtkWidget* AllPropertiesDialog::createTreeView() {

    _listStore = gtk_list_store_new(N_COLUMNS, 
                                    G_TYPE_STRING, 
                                    G_TYPE_STRING, 
                                    G_TYPE_STRING, 
                                    GDK_TYPE_PIXBUF);
    GtkWidget* _treeView = gtk_tree_view_new_with_model(GTK_TREE_MODEL(_listStore));
    g_object_unref(G_OBJECT(_listStore)); // Treeview owns reference to model

    // Key column
    GtkTreeViewColumn* keyCol = gtk_tree_view_column_new();
    gtk_tree_view_column_set_title(keyCol, "Property");
    gtk_tree_view_column_set_spacing(keyCol, 3);

    GtkCellRenderer* pixRenderer = gtk_cell_renderer_pixbuf_new();
    gtk_tree_view_column_pack_start(keyCol, pixRenderer, FALSE);
    gtk_tree_view_column_set_attributes(keyCol, pixRenderer, "pixbuf", ICON_COLUMN, NULL);

    GtkCellRenderer* keyRenderer = gtk_cell_renderer_text_new();
    gtk_tree_view_column_pack_start(keyCol, keyRenderer, TRUE);
    gtk_tree_view_column_set_attributes (keyCol, keyRenderer,
                                         "text", KEY_COLUMN,
                                         "foreground", TEXT_COLOUR_COLUMN,
                                         NULL);

    gtk_tree_view_append_column(GTK_TREE_VIEW(_treeView), keyCol);
    
    // Value column
    GtkTreeViewColumn* valCol = gtk_tree_view_column_new();
    
    gtk_tree_view_column_set_title(valCol, "Value");
    gtk_tree_view_column_set_spacing(valCol, 3);

    GtkCellRenderer* textRenderer = gtk_cell_renderer_text_new();
    gtk_tree_view_column_pack_start(valCol, textRenderer, TRUE);
    gtk_tree_view_column_set_attributes(valCol, textRenderer, 
                                        "text", VALUE_COLUMN, 
                                        "foreground", TEXT_COLOUR_COLUMN,
                                        NULL);
    g_object_set(G_OBJECT(textRenderer), "editable", TRUE, NULL);
    g_signal_connect(textRenderer, "edited", G_CALLBACK(callbackEditDone), this);

    gtk_tree_view_append_column(GTK_TREE_VIEW(_treeView), valCol);
    

    // Create the scrolled window
    GtkWidget* scrollWin = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrollWin), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrollWin), GTK_SHADOW_ETCHED_IN);
    gtk_container_add(GTK_CONTAINER(scrollWin), _treeView);    
    return scrollWin;

}

// Show GTK widgets

void AllPropertiesDialog::showForEntity(Entity* ent) {
    
    _entity = ent;
    
    // Clear the list store and re-populate it with the Entity's attributes

    gtk_list_store_clear(GTK_LIST_STORE(_listStore));

    struct ListStorePopulator: public Entity::Visitor {

        // The list store to populate
        GtkListStore* _store;
        
        // Set of known properties
        KnownPropertySet& _set;
        
        // Constructor
        ListStorePopulator(GtkListStore* store, KnownPropertySet& set): 
            _store(store),
            _set(set) {}

        // Required visit function
        virtual void visit(const char* key, const char* value) {
            
            // Determine if this property is in the known set
            std::string colour = "black";
            GdkPixbuf* buf;
            if (_set.find(std::string(key)) == _set.end()) {
                colour = "red";
                buf = gtkutil::getLocalPixbuf(UNRECOGNISED_PROPERTY_IMG);
            } else {
                buf = gtkutil::getLocalPixbuf(RECOGNISED_PROPERTY_IMG);
            }
            

            GtkTreeIter iter;
            gtk_list_store_append(_store, &iter);
            gtk_list_store_set(_store, &iter,
                                KEY_COLUMN, key,
                                VALUE_COLUMN, value,
                                TEXT_COLOUR_COLUMN, colour.c_str(),
                                ICON_COLUMN, buf,
                                -1);
        }
                
    } populator(_listStore, _knownProps);
    
    _entity->forEachKeyValue(populator);
    
    gtk_widget_show_all(GTK_WIDGET(_window));
}

// Cancel callback function

void AllPropertiesDialog::callbackCancel(GtkWidget* widget, AllPropertiesDialog* self) {
    self->destroy();
}

// Destroy callback

void AllPropertiesDialog::callbackDestroy(GtkWidget* widget, GdkEvent* event, AllPropertiesDialog* self) {
    self->destroy();
}

// Destroy self and all Gtk widgets

void AllPropertiesDialog::destroy() {
    delete this;
}

// Cell editing completion callback

void AllPropertiesDialog::callbackEditDone(GtkWidget* widget, const char* path, const char* newText, AllPropertiesDialog* self) {
    std::cout << "Editing finished, text is " << newText << std::endl;   
    
}


}
