#include "EntityInspector.h"
#include "PropertyEditorFactory.h"
#include "AddPropertyDialog.h"

#include "ientity.h"
#include "ieclass.h"
#include "iselection.h"
#include "iregistry.h"

#include "scenelib.h"
#include "gtkutil/dialog.h"
#include "gtkutil/StockIconMenuItem.h"
#include "xmlutil/Document.h"
#include "signal/signal.h"
#include "map/Map.h"

#include <map>
#include <string>

#include <gtk/gtk.h>

#include <boost/bind.hpp>

namespace ui {

/* CONSTANTS */

namespace {
	
    const int TREEVIEW_MIN_WIDTH = 220;
    const int TREEVIEW_MIN_HEIGHT = 60;
    const int PROPERTYEDITORPANE_MIN_HEIGHT = 90;
    
    const char* PROPERTY_NODES_XPATH = "game/entityInspector//property";
    
	// TreeView column numbers
    enum {
        PROPERTY_NAME_COLUMN,
        PROPERTY_VALUE_COLUMN,
        TEXT_COLOUR_COLUMN,
        PROPERTY_ICON_COLUMN,
        INHERITED_FLAG_COLUMN,
        N_COLUMNS
    };

}

// Constructor creates UI components for the EntityInspector dialog

EntityInspector::EntityInspector()
: _listStore(gtk_list_store_new(N_COLUMNS, 
	    						G_TYPE_STRING, // property
	    						G_TYPE_STRING, // value
	    						G_TYPE_STRING, // text colour
	    						GDK_TYPE_PIXBUF, // value icon
	    						G_TYPE_STRING)),
  _treeView(gtk_tree_view_new_with_model(GTK_TREE_MODEL(_listStore))),
  _contextMenu(gtkutil::PopupMenu(_treeView)),
  _idleDraw(MemberCaller<EntityInspector, &EntityInspector::callbackRedraw>(*this)), // Set the IdleDraw
  _showInherited(false)
{
    _widget = gtk_vbox_new(FALSE, 0);
    
	// Pack in GUI components
	
	GtkWidget* showInherited = gtk_check_button_new_with_label("Show inherited properties");
	g_signal_connect(G_OBJECT(showInherited), "toggled", G_CALLBACK(_onToggleShowInherited), this);

	gtk_box_pack_start(GTK_BOX(_widget), showInherited, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(_widget), createTreeViewPane(), TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(_widget), createDialogPane(), FALSE, FALSE, 0);
    
    // Create the context menu
    createContextMenu();
    
    // Stimulate initial redraw to get the correct status
    queueDraw();
    
    // Set the function to call when a keyval is changed. This is a requirement
    // of the EntityCreator interface.
    GlobalEntityCreator().setKeyValueChangedFunc(
    	EntityInspector::keyValueChanged);

	// Register self to the SelectionSystem to get notified upon selection changes.
	GlobalSelectionSystem().addObserver(this);
}

// Create the context menu
void EntityInspector::createContextMenu() {
	_contextMenu.addItem(
		gtkutil::StockIconMenuItem(GTK_STOCK_ADD, "Add property..."), 
		boost::bind(&EntityInspector::_onAddKey, this)
	);
	_contextMenu.addItem(
		gtkutil::StockIconMenuItem(GTK_STOCK_DELETE, "Delete property"), 
		boost::bind(&EntityInspector::_onDeleteKey, this), 
		boost::bind(&EntityInspector::_testDeleteKey, this)
	);
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
    gtk_frame_set_shadow_type(GTK_FRAME(_editorFrame), GTK_SHADOW_NONE);
    gtk_box_pack_start(GTK_BOX(hbx), _editorFrame, TRUE, TRUE, 0);
    gtk_widget_set_size_request(hbx, 0, PROPERTYEDITORPANE_MIN_HEIGHT);
    return hbx;
}

// Create the TreeView pane

GtkWidget* EntityInspector::createTreeViewPane() {
    
    GtkWidget* vbx = gtk_vbox_new(FALSE, 3);

    // Create the Property column
    GtkTreeViewColumn* nameCol = gtk_tree_view_column_new();
    gtk_tree_view_column_set_title(nameCol, "Property");
	gtk_tree_view_column_set_sizing(nameCol, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
    gtk_tree_view_column_set_spacing(nameCol, 3);

	GtkCellRenderer* pixRenderer = gtk_cell_renderer_pixbuf_new();
	gtk_tree_view_column_pack_start(nameCol, pixRenderer, FALSE);
    gtk_tree_view_column_set_attributes(nameCol, pixRenderer, "pixbuf", PROPERTY_ICON_COLUMN, NULL);

    GtkCellRenderer* textRenderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(nameCol, textRenderer, FALSE);
    gtk_tree_view_column_set_attributes(nameCol, textRenderer,
                                        "text", PROPERTY_NAME_COLUMN,
                                        "foreground", TEXT_COLOUR_COLUMN,
                                        NULL);

	gtk_tree_view_column_set_sort_column_id(nameCol, PROPERTY_NAME_COLUMN);
    gtk_tree_view_append_column(GTK_TREE_VIEW(_treeView), nameCol);                                                                        

	// Create the value column
    GtkTreeViewColumn* valCol = gtk_tree_view_column_new();
    gtk_tree_view_column_set_title(valCol, "Value");
	gtk_tree_view_column_set_sizing(valCol, GTK_TREE_VIEW_COLUMN_AUTOSIZE);

    GtkCellRenderer* valRenderer = gtk_cell_renderer_text_new();
    gtk_tree_view_column_pack_start(valCol, valRenderer, TRUE);
    gtk_tree_view_column_set_attributes(valCol, valRenderer, 
    									"text", PROPERTY_VALUE_COLUMN, 
    									"foreground", TEXT_COLOUR_COLUMN,
    									NULL);

	gtk_tree_view_column_set_sort_column_id(valCol, PROPERTY_VALUE_COLUMN);
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

	GtkWidget* setButton = gtk_button_new();
	gtk_container_add(
		GTK_CONTAINER(setButton), 
		gtk_image_new_from_stock(GTK_STOCK_APPLY, GTK_ICON_SIZE_MENU)
	);
	GtkWidget* setButtonBox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(setButtonBox), _valEntry, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(setButtonBox), setButton, FALSE, FALSE, 0);
	
	gtk_box_pack_start(GTK_BOX(vbx), _keyEntry, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbx), setButtonBox, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbx), gtk_hseparator_new(), FALSE, FALSE, 0);
    
    // Signals for entry boxes
    g_signal_connect(
    	G_OBJECT(setButton), "clicked", G_CALLBACK(_onSetProperty), this);
    g_signal_connect(
    	G_OBJECT(_keyEntry), "activate", G_CALLBACK(_onEntryActivate), this);
    g_signal_connect(
    	G_OBJECT(_valEntry), "activate", G_CALLBACK(_onEntryActivate), this);
    
    return vbx;    
}

// Retrieve the selected string from the given property in the list store

std::string EntityInspector::getListSelection(int col) {
	// Prepare to get the selection
    GtkTreeSelection* selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(_treeView));
    GtkTreeIter tmpIter;

	// Return the selected string if available, else a blank string
    if (gtk_tree_selection_get_selected(selection, NULL, &tmpIter)) {
        GValue selString = {0, 0};
        gtk_tree_model_get_value(GTK_TREE_MODEL(_listStore), &tmpIter, col, &selString);
        std::string value = g_value_get_string(&selString);
        g_value_unset(&selString);
        
        return value;
    }
    else {
    	return "";
    }
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
            _currentPropertyEditor = PropertyEditorPtr();
        }
		
		// Disable the dialog and clear the TreeView
        gtk_widget_set_sensitive(_widget, FALSE);
        gtk_list_store_clear(_listStore);
    }
    
}

// Entity keyvalue changed callback
void EntityInspector::keyValueChanged() {
    
    // Redraw the entity inspector GUI
    getInstance().queueDraw();
    
    // Set the map modified flag
    if (getInstance()._selectedEntity != NULL)
    	GlobalMap().setModified(true);
}

// Pass on a queueDraw request to the contained IdleDraw object.

inline void EntityInspector::queueDraw() {
    _idleDraw.queueDraw();
}

// Selection changed callback
void EntityInspector::selectionChanged(scene::Instance& instance, bool isComponent) {
	getInstance().queueDraw();
}

// Set entity property from entry boxes

void EntityInspector::setPropertyFromEntries() {
	std::string key = gtk_entry_get_text(GTK_ENTRY(_keyEntry));
	if (key.size() > 0) {
		std::string name = _selectedEntity->getKeyValue("name");
		std::string model = _selectedEntity->getKeyValue("model");
		bool isFuncType = (!name.empty() && name == model);
		
		std::string val = gtk_entry_get_text(GTK_ENTRY(_valEntry));
		_selectedEntity->setKeyValue(key, val);
		
		// Check for name key changes of func_statics
		if (isFuncType && key == "name") {
			// Adapt the model key along with the name
			_selectedEntity->setKeyValue("model", val);
		}
	}
}

// Construct and return static PropertyMap instance

const PropertyParmMap& EntityInspector::getPropertyMap() {

	// Static instance of local class, which queries the XML Registry
	// upon construction and adds the property nodes to the map.
	
	struct PropertyMapConstructor
	{
		// Map to construct
		PropertyParmMap _map;
		
		// Constructor queries the XML registry
		PropertyMapConstructor() {
			xml::NodeList pNodes = GlobalRegistry().findXPath(PROPERTY_NODES_XPATH);	
			for (xml::NodeList::const_iterator iter = pNodes.begin();
				 iter != pNodes.end();
				 ++iter)
			{
				PropertyParms parms;
				parms.type = iter->getAttributeValue("type");
				parms.options = iter->getAttributeValue("options");
				_map.insert(PropertyParmMap::value_type(iter->getAttributeValue("name"),
												  		parms));
			}
		}
		
		
	};
	static PropertyMapConstructor _propMap;
	
	// Return the constructed map
	return _propMap._map;
}

/* Popup menu callbacks (see gtkutil::PopupMenu) */

void EntityInspector::_onAddKey() 
{
	// Obtain the entity class to provide to the AddPropertyDialog
	IEntityClassConstPtr ec = _selectedEntity->getEntityClass();
	
	// Choose a property, and add to entity with a default value
	std::string property = AddPropertyDialog::chooseProperty(ec);
    if (!property.empty()) {
        
        // Save last key, so that it will be automatically selected
        _lastKey = property;
        
        // Add the keyvalue on the entity (triggering the refresh)
		_selectedEntity->setKeyValue(property, "-");
    }
}

void EntityInspector::_onDeleteKey() {
	std::string property = getListSelection(PROPERTY_NAME_COLUMN);
	if (!property.empty())
		_selectedEntity->setKeyValue(property, "");
}

bool EntityInspector::_testDeleteKey() {
	// Make sure the Delete item is only available for explicit 
	// (non-inherited) properties
	if (getListSelection(INHERITED_FLAG_COLUMN) != "1")
		return true;
	else
		return false;
}

/* GTK CALLBACKS */

// Called when the TreeView selects a different property
void EntityInspector::callbackTreeSelectionChanged(GtkWidget* widget, EntityInspector* self) {
    self->treeSelectionChanged();
}

void EntityInspector::_onSetProperty(GtkWidget* button, EntityInspector* self) {
	self->setPropertyFromEntries();
}

// ENTER key in entry boxes
void EntityInspector::_onEntryActivate(GtkWidget* w, EntityInspector* self) {
	// Set property and move back to key entry
	self->setPropertyFromEntries();
	gtk_widget_grab_focus(self->_keyEntry);
}

void EntityInspector::_onToggleShowInherited(GtkToggleButton* b, EntityInspector* self) {
	if (gtk_toggle_button_get_active(b)) {
		self->_showInherited = true;
	}
	else {
		self->_showInherited = false;
	}
	// Refresh list display
	self->refreshTreeModel();
}


/* END GTK CALLBACKS */

// Update the PropertyEditor pane, displaying the PropertyEditor if necessary 
// and making sure it refers to the currently-selected Entity.
void EntityInspector::treeSelectionChanged() {

	// Abort if called without a valid entity selection (may happen during 
	// various cleanup operations).
	if (_selectedEntity == NULL)
		return;

    // Get the selected key and value in the tree view
    std::string key = getListSelection(PROPERTY_NAME_COLUMN);
    std::string value = getListSelection(PROPERTY_VALUE_COLUMN);
    if (!key.empty())
        _lastKey = key; // save last key
    
    // Get the type for this key if it exists, and the options
    PropertyParmMap::const_iterator tIter = getPropertyMap().find(key);
    std::string type = (tIter != getPropertyMap().end() 
    					? tIter->second.type 
    					: "");
    std::string options = (tIter != getPropertyMap().end() 
    					   ? tIter->second.options 
    					   : "");
    
    // If the type was not found, also try looking on the entity class
    if (type.empty()) {
    	IEntityClassConstPtr eclass = _selectedEntity->getEntityClass();
		type = eclass->getAttribute(key).type;
    }

	// Remove the existing PropertyEditor widget, if there is one
	GtkWidget* existingWidget = gtk_bin_get_child(GTK_BIN(_editorFrame));    
   	if (existingWidget != NULL)
   		gtk_widget_destroy(existingWidget);

    // Construct and add a new PropertyEditor
    _currentPropertyEditor = PropertyEditorFactory::create(type,
                                                           _selectedEntity,
                                                           key,
                                                           options);
                                                           
	// If the creation was successful (because the PropertyEditor type exists),
	// add its widget to the editor pane
    if (_currentPropertyEditor) {
        gtk_container_add(GTK_CONTAINER(_editorFrame), 
        				  _currentPropertyEditor->getWidget());
    }
    
    // Update key and value entry boxes, but only if there is a key value. If
    // there is no selection we do not clear the boxes, to allow keyval copying
    // between entities.
	if (!key.empty()) {
		gtk_entry_set_text(GTK_ENTRY(_keyEntry), key.c_str());
		gtk_entry_set_text(GTK_ENTRY(_valEntry), value.c_str());
	}

}

// Main refresh function.
void EntityInspector::refreshTreeModel() {

	// Clear the existing list
	gtk_list_store_clear(_listStore);

	// Local functor to enumerate keyvals on object and add them to the list
	// view.
	
	class ListPopulateVisitor
	: public Entity::Visitor
	{
		// List store to populate
		GtkListStore* _store;
		
		// Property map to look up types
		const PropertyParmMap& _map;
		
		// Entity class to check for types
		IEntityClassConstPtr _eclass;
        
        // Last selected key to highlight
        std::string _lastKey;
        
        // TreeIter to select, if we find the last-selected key
        GtkTreeIter* _lastIter;
	
	public:
	
		// Constructor
		ListPopulateVisitor(GtkListStore* store, 
							const PropertyParmMap& map,
							IEntityClassConstPtr cls,
                            std::string lastKey)
        : _store(store), _map(map), _eclass(cls), _lastKey(lastKey),
          _lastIter(NULL)
		{
        }
		
		// Required visit function
		virtual void visit(const std::string& key, const std::string& value) {

			// Look up type for this key. First check the property parm map,
			// then the entity class itself. If nothing is found, leave blank.
			PropertyParmMap::const_iterator typeIter = _map.find(key);
			std::string type;
			if (typeIter != _map.end()) {
				type = typeIter->second.type;
			}
			else {
				// Check the entityclass (which will return blank if not found)
				type = _eclass->getAttribute(key).type;
			}

			// Append the details to the treestore
			GtkTreeIter iter;
			gtk_list_store_append(_store, &iter);
			gtk_list_store_set(
				_store, &iter,
				PROPERTY_NAME_COLUMN, key.c_str(),
				PROPERTY_VALUE_COLUMN, value.c_str(),
				TEXT_COLOUR_COLUMN, "black",
				PROPERTY_ICON_COLUMN, PropertyEditorFactory::getPixbufFor(type),
				INHERITED_FLAG_COLUMN, "", // not inherited
				-1);
            
            // If this was the last selected key, save the Iter so we can
            // select it again
            if (key == _lastKey) {
                _lastIter = gtk_tree_iter_copy(&iter);
            }
							   	
		}
        
        // Get the iter pointing to the last-selected key
        GtkTreeIter* getLastIter() {
            return _lastIter;
        }
			
	};
	
	// Populate the list view
	ListPopulateVisitor visitor(_listStore, 
								getPropertyMap(),
								_selectedEntity->getEntityClass(),
                                _lastKey);
	_selectedEntity->forEachKeyValue(visitor);

	// Add the inherited properties if the toggle is set
	if (_showInherited) {
		appendClassProperties();
	}

    // If we found the last-selected key, select it
    GtkTreeIter* lastIter = visitor.getLastIter();
    if (lastIter != NULL) {
        gtk_tree_selection_select_iter(
            gtk_tree_view_get_selection(GTK_TREE_VIEW(_treeView)),
            lastIter
        );
    }
                                   
	// Force an update of widgets
	treeSelectionChanged();

}

// Append inherited (entityclass) properties
void EntityInspector::appendClassProperties() {

	// Get the entityclass for the current entity
	std::string className = _selectedEntity->getKeyValue("classname");
	IEntityClassPtr eclass = GlobalEntityClassManager().findOrInsert(className, 
																	 true);
	
	// Use a functor to walk the entityclass and add all of its attributes
	// to the tree

	struct ClassPropertyVisitor
	: public EntityClassAttributeVisitor
	{

		// List store to populate
		GtkListStore* _store;

		// Constructor
		ClassPropertyVisitor(GtkListStore* store)
		: _store(store) {}

		// Required visitor function
		void visit(const EntityClassAttribute& a) {
			
			// Only add properties with values, we don't want the optional
			// "editor_var xxx" properties here.
			if (!a.value.empty()) {
				GtkTreeIter iter;
				gtk_list_store_append(_store, &iter);
				gtk_list_store_set(_store, &iter,
					PROPERTY_NAME_COLUMN, a.name.c_str(),
					PROPERTY_VALUE_COLUMN, a.value.c_str(),
					TEXT_COLOUR_COLUMN, "#707070",
					PROPERTY_ICON_COLUMN, NULL,
					INHERITED_FLAG_COLUMN, "1", // inherited
					-1);
			}
		}
	};
	
	// Visit the entity class
	ClassPropertyVisitor visitor(_listStore);
	eclass->forEachClassAttribute(visitor);
}

// Update the selected Entity pointer

bool EntityInspector::updateSelectedEntity() {

	_selectedEntity = NULL;

	// A single entity must be selected
	if (GlobalSelectionSystem().countSelected() != 1)
		return false;

	// The root node must not be selected (this can happen if Invert Selection is activated
	// with an empty scene, or by direct selection in the entity list).
	if (GlobalSelectionSystem().ultimateSelected().path().top()->isRoot())
		return false;
	
	// Try both the selected node (if an entity is selected) or the parent node (if a brush is 
	// selected. If neither of them convert to entities, return false.
	if ((_selectedEntity = Node_getEntity(GlobalSelectionSystem().ultimateSelected().path().top())) == 0
		 && (_selectedEntity = Node_getEntity(GlobalSelectionSystem().ultimateSelected().path().parent())) == 0)
	{
		return false;
	}
	else 
	{
		return true;
	}
}

} // namespace ui
