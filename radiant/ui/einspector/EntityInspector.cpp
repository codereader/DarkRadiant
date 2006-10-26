#include "EntityInspector.h"
#include "PropertyEditorFactory.h"

#include "ientity.h"
#include "iselection.h"

#include "gtkutil/image.h"
#include "gtkutil/dialog.h"

#include "xmlutil/Document.h"
#include "xmlutil/AttributeNotFoundException.h"

#include "signal/signal.h"

#include "error.h"

#include <iostream>
#include <map>
#include <string>

namespace ui {

/* CONSTANTS */

namespace {
	
    const int TREEVIEW_MIN_WIDTH = 220;
    const int TREEVIEW_MIN_HEIGHT = 60;
    const int PROPERTYEDITORPANE_MIN_HEIGHT = 120;
    
	// TreeView column numbers
    enum {
        PROPERTY_NAME_COLUMN,
        PROPERTY_VALUE_COLUMN,
        TEXT_COLOUR_COLUMN,
        PROPERTY_ICON_COLUMN,
        N_COLUMNS
    };

}

// Constructor creates UI components for the EntityInspector dialog

EntityInspector::EntityInspector()
:_idleDraw(MemberCaller<EntityInspector, &EntityInspector::callbackRedraw>(*this)) // Set the IdleDraw
{
    _widget = gtk_vbox_new(FALSE, 0);
    
    gtk_box_pack_start(GTK_BOX(_widget), createTreeViewPane(), TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(_widget), createDialogPane(), FALSE, FALSE, 0);
    
    // Stimulate initial redraw to get the correct status
    queueDraw();
    
    // Set the function to call when a keyval is changed. This is a requirement
    // of the EntityCreator interface.
    GlobalEntityCreator().setKeyValueChangedFunc(EntityInspector::redrawInstance);

    // Create callback object to redraw the dialog when the selected entity is
    // changed
    GlobalSelectionSystem().addSelectionChangeCallback(FreeCaller1<const Selectable&, EntityInspector::selectionChanged>());
}

// Return the singleton EntityInspector instance, creating it if it is not yet
// created. Single-threaded design.

EntityInspector& EntityInspector::getInstance() {
    static EntityInspector _instance;
    return _instance;
}

// Return the Gtk widget for the EntityInspector dialog. 

GtkWidget* EntityInspector::getWidget() {
	gtk_widget_show_all(_widget);
    return _widget;
}

// Create the dialog pane

GtkWidget* EntityInspector::createDialogPane() {
	GtkWidget* hbx = gtk_hbox_new(FALSE, 0);
    _editorFrame = gtk_frame_new(NULL);
    gtk_box_pack_start(GTK_BOX(hbx), _editorFrame, TRUE, TRUE, 0);
    gtk_widget_set_size_request(hbx, 0, PROPERTYEDITORPANE_MIN_HEIGHT);
    return hbx;
}

// Create the TreeView pane

GtkWidget* EntityInspector::createTreeViewPane() {
    GtkWidget* vbx = gtk_vbox_new(FALSE, 0);

    // Initialise the instance TreeStore
    _listStore = gtk_list_store_new(N_COLUMNS, 
    							    G_TYPE_STRING, // property
    							    G_TYPE_STRING, // value
                                    G_TYPE_STRING, // text colour
    							    GDK_TYPE_PIXBUF); // value icon
    
    // Create the TreeView widget and link it to the model
    _treeView = gtk_tree_view_new_with_model(GTK_TREE_MODEL(_listStore));

    // Add columns to the TreeView
    GtkCellRenderer* textRenderer;

    textRenderer = gtk_cell_renderer_text_new();
    GtkTreeViewColumn* nameCol = 
        gtk_tree_view_column_new_with_attributes("Property",
                                                 textRenderer,
                                                 "text",
                                                 PROPERTY_NAME_COLUMN,
                                                 "foreground",
                                                 TEXT_COLOUR_COLUMN,
                                                 NULL);
    gtk_tree_view_column_set_resizable(nameCol, TRUE);
    gtk_tree_view_append_column(GTK_TREE_VIEW(_treeView), nameCol);                                                                        

	// Create the value column

    GtkTreeViewColumn* valCol = gtk_tree_view_column_new();
    gtk_tree_view_column_set_title(valCol, "Value");
    gtk_tree_view_column_set_spacing(valCol, 3);

	GtkCellRenderer* pixRenderer = gtk_cell_renderer_pixbuf_new();
	gtk_tree_view_column_pack_start(valCol, pixRenderer, FALSE);
    gtk_tree_view_column_set_attributes(valCol, pixRenderer, "pixbuf", PROPERTY_ICON_COLUMN, NULL);

    textRenderer = gtk_cell_renderer_text_new();
    gtk_tree_view_column_pack_start(valCol, textRenderer, TRUE);
    gtk_tree_view_column_set_attributes(valCol, textRenderer, "text", PROPERTY_VALUE_COLUMN, NULL);

    gtk_tree_view_append_column(GTK_TREE_VIEW(_treeView), valCol);

    // Set up the signals
    GtkTreeSelection* selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(_treeView));
    g_signal_connect(G_OBJECT(selection), "changed", G_CALLBACK(callbackTreeSelectionChanged), this);
                                                                         
    // Embed the TreeView in a scrolled viewport
    GtkWidget* scrollWin = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrollWin), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrollWin), GTK_SHADOW_ETCHED_IN);
    gtk_widget_set_size_request(scrollWin, TREEVIEW_MIN_WIDTH, TREEVIEW_MIN_HEIGHT);
    gtk_container_add(GTK_CONTAINER(scrollWin), _treeView);    

    gtk_box_pack_start(GTK_BOX(vbx), scrollWin, TRUE, TRUE, 0);

	// Pack in the key and value edit boxes
	_keyEntry = gtk_entry_new();
	_valEntry = gtk_entry_new();

	GtkWidget* setButton = gtk_button_new_with_label("Set");
	GtkWidget* setButtonBox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(setButtonBox), _valEntry, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(setButtonBox), setButton, FALSE, FALSE, 0);
	
	gtk_box_pack_start(GTK_BOX(vbx), _keyEntry, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbx), setButtonBox, FALSE, FALSE, 0);
    
    // Signals for entry boxes
    g_signal_connect(G_OBJECT(setButton), "clicked", G_CALLBACK(_onSetProperty), this);
    g_signal_connect(G_OBJECT(_keyEntry), "activate", G_CALLBACK(_onKeyEntryActivate), this);
    g_signal_connect(G_OBJECT(_valEntry), "activate", G_CALLBACK(_onValEntryActivate), this);
    
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
        refreshTreeModel(); // get values, already have category tree
    }
    else {
        // Remove the displayed PropertyEditor
        if (_currentPropertyEditor) {
            delete _currentPropertyEditor;
            _currentPropertyEditor = NULL;
        }
		// Disable the dialog and clear the TreeView
        gtk_widget_set_sensitive(_widget, FALSE);
        gtk_list_store_clear(_listStore);
        
    }
}

// Static function to get the singleton instance and invoke its redraw function.
// This is necessary since the EntityCreator interface requires a pointer to 
// a non-member function to call when a keyval is changed.

void EntityInspector::redrawInstance() {
    getInstance().queueDraw();
}

// Pass on a queueDraw request to the contained IdleDraw object.

inline void EntityInspector::queueDraw() {
    _idleDraw.queueDraw();
}

// Selection changed callback

void EntityInspector::selectionChanged(const Selectable& sel) {
    EntityInspector::redrawInstance();   
}

// Set entity property from entry boxes

void EntityInspector::setPropertyFromEntries() {
	std::string key = gtk_entry_get_text(GTK_ENTRY(_keyEntry));
	std::string val = gtk_entry_get_text(GTK_ENTRY(_valEntry));
	_selectedEntity->setKeyValue(key, val);
}

// Construct and return static PropertyMap instance

const StringMap& EntityInspector::getPropertyMap() {

	// Static instance of local class, which queries the XML Registry
	// upon construction and adds the property nodes to the map.
	
	struct PropertyMapConstructor
	{
		// String map to construct
		StringMap _map;
		
		// Constructor queries the XML registry
		PropertyMapConstructor() {
			xml::NodeList pNodes = GlobalRadiant().registry().findXPath("game/entityInspector//propertyCategory//property");	
			for (xml::NodeList::const_iterator iter = pNodes.begin();
				 iter != pNodes.end();
				 ++iter)
			{
				_map.insert(StringMap::value_type(iter->getAttributeValue("name"),
												  iter->getAttributeValue("type")));
			}
		}
		
		
	};
	static PropertyMapConstructor _propMap;
	
	// Return the constructed map
	return _propMap._map;
}

/* GTK CALLBACKS */

// Called when the TreeView selects a different property
void EntityInspector::callbackTreeSelectionChanged(GtkWidget* widget, EntityInspector* self) {
    self->treeSelectionChanged();
}

void EntityInspector::_onSetProperty(GtkWidget* button, EntityInspector* self) {
	self->setPropertyFromEntries();
}

void EntityInspector::_onKeyEntryActivate(GtkWidget* w, EntityInspector* self) {
	// Move to value entry
	gtk_widget_grab_focus(self->_valEntry);	
}

void EntityInspector::_onValEntryActivate(GtkWidget* w, EntityInspector* self) {
	// Set property and move back to key entry
	self->setPropertyFromEntries();
	gtk_widget_grab_focus(self->_keyEntry);
}
/* END GTK CALLBACKS */

// Update the PropertyEditor pane, displaying the PropertyEditor if necessary and
// making sure it refers to the currently-selected Entity.

void EntityInspector::treeSelectionChanged() {

    // Delete current property editor
    if (_currentPropertyEditor) {
        delete _currentPropertyEditor; // destructor takes care of GTK widgets
        _currentPropertyEditor = NULL;
    }

	// Key and value text for the entry boxes
	std::string key = "";
	std::string value = "";

    GtkTreeSelection* selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(_treeView));

    // Get the TreeView selection if it exists, and add the correct PropertyEditor. If
    // nothing is selected, the PropertyEditor pane will be left blank.
    GtkTreeIter tmpIter;
    if (gtk_tree_selection_get_selected(selection, NULL, &tmpIter)) {

        // Get the selected key and value in the tree view
        GValue selString = {0, 0};
        gtk_tree_model_get_value(GTK_TREE_MODEL(_listStore), &tmpIter, PROPERTY_NAME_COLUMN, &selString);
        key = g_value_get_string(&selString);
        g_value_unset(&selString);
        gtk_tree_model_get_value(GTK_TREE_MODEL(_listStore), &tmpIter, PROPERTY_VALUE_COLUMN, &selString);
        value = g_value_get_string(&selString);
        g_value_unset(&selString);
        
        // Get the type for this key
        std::string type = getPropertyMap().find(key)->second;
        
        _currentPropertyEditor = PropertyEditorFactory::create(type,
                                                               _selectedEntity,
                                                               key,
                                                               ""); // TODO: implement options
        if (_currentPropertyEditor != NULL) {
            gtk_container_add(GTK_CONTAINER(_editorFrame), _currentPropertyEditor->getWidget());
        }
    }
    
    // Update key and value entry boxes
	gtk_entry_set_text(GTK_ENTRY(_keyEntry), key.c_str());
	gtk_entry_set_text(GTK_ENTRY(_valEntry), value.c_str());

}

// Main refresh function. The TreeStore is populated with categories,
// but the values are not yet set.

void EntityInspector::refreshTreeModel() {

	// Clear the existing list
	gtk_list_store_clear(_listStore);

	// Local functor to enumerate keyvals on object and add them to the list
	// view.
	
	struct ListPopulateVisitor
	: public Entity::Visitor
	{
		// List store to populate
		GtkListStore* _store;
		
		// Property map to look up types
		const StringMap& _map;
		
		// Constructor
		ListPopulateVisitor(GtkListStore* store, const StringMap& map)
		: _store(store), _map(map)
		{}
		
		// Required visit function
		virtual void visit(const char* key, const char* value) {
			// Look up type for this key
			std::string type = _map.find(key)->second;

			GtkTreeIter iter;
			gtk_list_store_append(_store, &iter);
			gtk_list_store_set(_store, &iter,
             			       PROPERTY_NAME_COLUMN, key,
						       PROPERTY_VALUE_COLUMN, value,
						       TEXT_COLOUR_COLUMN, "black",
						       PROPERTY_ICON_COLUMN, PropertyEditorFactory::getPixbufFor(type),
						       -1);
							   	
		}
			
	};
	
	// Populate the list view
	ListPopulateVisitor visitor(_listStore, getPropertyMap());
	_selectedEntity->forEachKeyValue(visitor);

	// Force an update of widgets
	treeSelectionChanged();

}

// Update the selected Entity pointer

bool EntityInspector::updateSelectedEntity() {
	if (GlobalSelectionSystem().countSelected() != 1 ||
           (_selectedEntity = Node_getEntity(GlobalSelectionSystem().ultimateSelected().path().top())) == 0
        && (_selectedEntity = Node_getEntity(GlobalSelectionSystem().ultimateSelected().path().parent())) == 0)
    {
		return false;
    } else {
		return true;
    }
}

} // namespace ui
