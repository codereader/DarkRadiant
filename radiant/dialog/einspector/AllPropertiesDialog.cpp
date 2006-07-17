#include "AllPropertiesDialog.h"

#include "mainframe.h"

#include "gtkutil/image.h"
#include "gtkutil/dialog.h"
#include "gtkutil/EntryAbortedException.h"

#include "iundo.h"

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
    g_signal_connect(G_OBJECT(okButton), "clicked", G_CALLBACK(callbackOK), this);
    g_signal_connect(G_OBJECT(addButton), "clicked", G_CALLBACK(callbackAdd), this);
    g_signal_connect(G_OBJECT(removeButton), "clicked", G_CALLBACK(callbackDelete), this);
    
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
    _treeView = gtk_tree_view_new_with_model(GTK_TREE_MODEL(_listStore));
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

// OK callback function

void AllPropertiesDialog::callbackOK(GtkWidget* widget, AllPropertiesDialog* self) {

    // Construct a Set of properties on the Entity so we can compare this with
    // the set of properties in the ListStore.

    struct KeyValSetMaker: public Entity::Visitor {
        
        // Set of discovered properties
        std::set<std::string> _propSet;
        
        // Entity to operate on
        Entity* _ent;        
        
        // Constructor
        KeyValSetMaker(Entity* ent): _ent(ent) {}
        
        virtual void visit(const char* key, const char* value) {
            _propSet.insert(key);
        }
        
    } kvSet(self->_entity);
    
    self->_entity->forEachKeyValue(kvSet);
    
    // Now set keyvals based on the ListStore contents, removing them from the
    // Entity's set as we go to yield a final set of properties to be deleted
    // from the Entity.

	GlobalUndoSystem().start();
    
    GtkTreeIter iter;
    if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(self->_listStore), &iter)) {
        do {

            GValue key = { 0, 0 };
            GValue value = { 0, 0 };

            gtk_tree_model_get_value(GTK_TREE_MODEL(self->_listStore), &iter, KEY_COLUMN, &key);
            gtk_tree_model_get_value(GTK_TREE_MODEL(self->_listStore), &iter, VALUE_COLUMN, &value);

            const char* strKey = g_value_get_string(&key);
            const char* strVal = g_value_get_string(&value);

            self->_entity->setKeyValue(strKey, strVal);
            kvSet._propSet.erase(std::string(strKey));
            
        } while (gtk_tree_model_iter_next(GTK_TREE_MODEL(self->_listStore), &iter));
    }

	// Delete all Entity keys that are left (i.e. not in the ListStore).
	
	for (std::set<std::string>::iterator i = kvSet._propSet.begin();
		 	i != kvSet._propSet.end();
		 		i++) {
		self->_entity->setKeyValue(i->c_str(), "");
	}

	GlobalUndoSystem().finish("entitySetKeyValues -allpropertiesdialog");

    // Close and destroy the dialog
    self->destroy();
    
}

// Callback for Add button

void AllPropertiesDialog::callbackAdd(GtkWidget* widget, AllPropertiesDialog* self) {
    try {
        const std::string newKey = gtkutil::textEntryDialog("Add property", "New key name: ");   
        if (newKey.size() > 0) {
            GtkTreeIter iter;
            gtk_list_store_append(GTK_LIST_STORE(self->_listStore), &iter);
            gtk_list_store_set(GTK_LIST_STORE(self->_listStore), &iter,
                               KEY_COLUMN, newKey.c_str(),
                               VALUE_COLUMN, "", // setting to "" causes the key to be removed if not filled in before OK is pressed
                               TEXT_COLOUR_COLUMN, "blue",
                               -1);
            // Start editing the value cell immediately
            gtk_tree_view_set_cursor(GTK_TREE_VIEW(self->_treeView), 
                                     gtk_tree_model_get_path(GTK_TREE_MODEL(self->_listStore), &iter), 
                                     gtk_tree_view_get_column(GTK_TREE_VIEW(self->_treeView), VALUE_COLUMN),
                                     TRUE); // start editing
        }        
    } catch (gtkutil::EntryAbortedException e) {
        // Do nothing
    }
}

// Callback for Delete button

void AllPropertiesDialog::callbackDelete(GtkWidget* widget, AllPropertiesDialog* self) {
    GtkTreeSelection* selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(self->_treeView));
    GtkTreeIter iter;
    if (gtk_tree_selection_get_selected(selection, NULL, &iter)) {
        gtk_list_store_remove(GTK_LIST_STORE(self->_listStore), &iter);
    }
}

// Destroy callback

void AllPropertiesDialog::callbackDestroy(GtkWidget* widget, GdkEvent* event, AllPropertiesDialog* self) {
    self->destroy();
}

// Cell editing completion callback

void AllPropertiesDialog::callbackEditDone(GtkWidget* widget, const char* path, const char* newText, AllPropertiesDialog* self) {
    GtkTreeIter iter;
    gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(self->_listStore), &iter, path);
    gtk_list_store_set(GTK_LIST_STORE(self->_listStore), &iter, VALUE_COLUMN, newText, -1);
}

// Destroy self and all Gtk widgets

void AllPropertiesDialog::destroy() {
    delete this;
}


} // namespace ui
