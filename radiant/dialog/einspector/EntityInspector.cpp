#include "EntityInspector.h"
#include "EntityKeyValueVisitor.h"
#include "PropertyEditor.h"
#include "PropertyEditorFactory.h"

#include "ientity.h"
#include "iselection.h"

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
    _widget = gtk_vbox_new(FALSE, 0);
    
    gtk_box_pack_end(GTK_BOX(_widget), createDialogPane(), FALSE, FALSE, 6);
    gtk_box_pack_start(GTK_BOX(_widget), createTreeViewPane(), TRUE, TRUE, 0);
    
    gtk_widget_show_all(_widget);
    
    // Stimulate initial redraw to get the correct status
    queueDraw();
    
    // Set the function to call when a keyval is changed. This is a requirement
    // of the EntityCreator interface.
    GlobalEntityCreator().setKeyValueChangedFunc(EntityInspector::redrawInstance);

    // Create callback object to redraw the dialog when the selected entity is
    // changed
    GlobalSelectionSystem().addSelectionChangeCallback(FreeCaller1<const Selectable&, EntityInspector::selectionChanged>());
}

// Create the dialog pane

GtkWidget* EntityInspector::createDialogPane() {
    GtkWidget* hbx = gtk_hbox_new(FALSE, 0);
    _editorFrame = gtk_frame_new("Edit property");
    gtk_box_pack_start(GTK_BOX(hbx), _editorFrame, TRUE, TRUE, 6);
    gtk_widget_set_size_request(hbx, 0, 200);
    return hbx;
}

// Create the TreeView pane

GtkWidget* EntityInspector::createTreeViewPane() {
    GtkWidget* vbx = gtk_vbox_new(FALSE, 0);

    // Initialise the instance TreeStore
    _treeStore = gtk_tree_store_new(N_COLUMNS, G_TYPE_STRING, G_TYPE_STRING);
    
    // Create the TreeView widget and link it to the model
    _treeView = gtk_tree_view_new_with_model(GTK_TREE_MODEL(_treeStore));

    // Add columns to the TreeView
    GtkCellRenderer* textRenderer;

    textRenderer = gtk_cell_renderer_text_new();
    GtkTreeViewColumn* nameCol = 
        gtk_tree_view_column_new_with_attributes("Property",
                                                 textRenderer,
                                                 "text",
                                                 PROPERTY_NAME_COLUMN,
                                                 NULL);
    gtk_tree_view_column_set_resizable(nameCol, TRUE);
    gtk_tree_view_column_set_sizing(nameCol, GTK_TREE_VIEW_COLUMN_GROW_ONLY);
    gtk_tree_view_column_set_fixed_width(nameCol, 200);
    gtk_tree_view_append_column(GTK_TREE_VIEW(_treeView), nameCol);                                                                        

    textRenderer = gtk_cell_renderer_text_new();
    GtkTreeViewColumn* valCol =
        gtk_tree_view_column_new_with_attributes("Value",
                                                 textRenderer,
                                                 "text",
                                                 PROPERTY_VALUE_COLUMN,
                                                 NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(_treeView), valCol);

    // Set up the signals
    g_signal_connect(G_OBJECT(_treeView), "cursor-changed", G_CALLBACK(callbackTreeSelectionChanged), this);
                                                                         
    // Embed the TreeView in a scrolled viewport
    GtkWidget* scrollWin = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrollWin), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_widget_set_size_request(scrollWin, 260, 180);
    gtk_container_add(GTK_CONTAINER(scrollWin), _treeView);    

    gtk_box_pack_start(GTK_BOX(vbx), scrollWin, TRUE, TRUE, 0);
    return vbx;    
}

// Redraw the GUI elements, such as in response to a key/val change on the
// selected entity. This is called from the IdleDraw member object when
// idle.

void EntityInspector::callbackRedraw() {

    // Entity Inspector can only be used on a single entity. Multiple selections
    // or nothing selected result in a grayed-out dialog, as does the selection
    // of something that is not an Entity (worldspawn).

    if (updateSelectedEntity()) {
        gtk_widget_set_sensitive(_widget, TRUE);
        populateTreeModel(); 
    }
    else {
		// Disable the dialog and clear the TreeView
        gtk_widget_set_sensitive(_widget, FALSE);
        gtk_tree_store_clear(_treeStore);
    }
}

// Static function to get the singleton instance and invoke its redraw function.
// This is necessary since the EntityCreator interface requires a pointer to 
// a non-member function to call when a keyval is changed.

inline void EntityInspector::redrawInstance() {
    getInstance()->queueDraw();
}

// Pass on a queueDraw request to the contained IdleDraw object.

inline void EntityInspector::queueDraw() {
    _idleDraw.queueDraw();
}

// Selection changed callback

inline void EntityInspector::selectionChanged(const Selectable& sel) {
    EntityInspector::redrawInstance();   
}

/* GTK CALLBACKS */

// Called when the TreeView selects a different property
void EntityInspector::callbackTreeSelectionChanged(GtkWidget* widget, EntityInspector* self) {
    GtkTreeSelection* selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(self->_treeView));

    GtkTreeIter tmpIter;
    gtk_tree_selection_get_selected(selection, NULL, &tmpIter);

	// Get the selected key in the tree view
    GValue selString = { 0, 0 };
    gtk_tree_model_get_value(GTK_TREE_MODEL(self->_treeStore), &tmpIter, 0, &selString);
    const char *key = g_value_get_string(&selString);
    g_value_unset(&selString);
    
    // Construct the PropertyEditor, destroying the current one if it exists
    if (self->_currentPropertyEditor) {
    	delete self->_currentPropertyEditor; // destructor takes care of GTK widgets
    }
    self->_currentPropertyEditor = PropertyEditorFactory::create("TextPropertyEditor",
    						 							   self->_selectedEntity,
    													   key);
    gtk_container_add(GTK_CONTAINER(self->_editorFrame), self->_currentPropertyEditor->getWidget());
}

// Populate TreeStore with current selections' keyvals

void EntityInspector::populateTreeModel() {
    
    // Create a visitor and use it to obtain the key/value map
    EntityKeyValueVisitor visitor;
    _selectedEntity->forEachKeyValue(visitor);
    KeyValueMap kvMap = visitor.getMap();
    
    // Clear the current TreeStore, and add the keys to it
    gtk_tree_store_clear(_treeStore);
    GtkTreeIter basicIter;
    gtk_tree_store_append(_treeStore, &basicIter, NULL);
    gtk_tree_store_set(_treeStore, &basicIter, PROPERTY_NAME_COLUMN, "Basic", -1);

    for (KeyValueMap::iterator i = kvMap.begin(); i != kvMap.end(); i++) {
        GtkTreeIter tempIter;
        gtk_tree_store_append(_treeStore, &tempIter, &basicIter);
        gtk_tree_store_set(_treeStore, &tempIter, PROPERTY_NAME_COLUMN, i->first, -1);
        gtk_tree_store_set(_treeStore, &tempIter, PROPERTY_VALUE_COLUMN, i->second, -1);
    }
}

// Update the selected Entity pointer

bool EntityInspector::updateSelectedEntity() {
	if (GlobalSelectionSystem().countSelected() != 1 ||
        (_selectedEntity = Node_getEntity(GlobalSelectionSystem().ultimateSelected().path().top())) == 0) {
        return false;
    } else {
		return true;
    }
}

} // namespace ui
