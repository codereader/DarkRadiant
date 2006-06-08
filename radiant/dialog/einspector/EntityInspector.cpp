#include "EntityInspector.h"
#include "EntityKeyValueVisitor.h"
#include "PropertyEditor.h"
#include "PropertyEditorFactory.h"
#include "AllPropertiesDialog.h"

#include "ientity.h"
#include "iselection.h"
#include "igamedescriptor.h"

#include "gtkutil/image.h"
#include "gtkutil/dialog.h"

#include "xmlutil/Document.h"

#include "signal/signal.h"

#include "error.h"

#include <iostream>
#include <sstream>

namespace ui {

// INITIALISATION

EntityInspector::PropertyCategoryMap EntityInspector::_categoryMap;

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

// Static function to obtain the PropertyCategories from the XML file.

void EntityInspector::parseXml(xml::Document doc) {

    xml::NodeList categories = doc.findXPath("/game/entityInspector//propertyCategory");

    for (unsigned int i = 0; i < categories.size(); i++)
    	makePropertyCategory(categories[i]);
}

void EntityInspector::makePropertyCategory(xml::Node& node) {

	// Get the category name
	std::string categoryName = node.getAttributeValue("name");
	PropertyCategory* cat = new PropertyCategory(); 

	// Find all <property> elements underneath <propertyCategory>
    xml::NodeList propertyList = node.getNamedChildren("property");

	for (unsigned int i = 0; i < propertyList.size(); i++) {

		// Look up the name and type attributes of this property node
		std::string keyName = propertyList[i].getAttributeValue("name");
		std::string keyType = propertyList[i].getAttributeValue("type");

		// Add the name and type to the PropertyCategory object
		cat->insert(PropertyCategory::value_type(keyName, keyType));

	}
	
	// Add the PropertyCategory to the category map, as long as it is not
	// empty.
	if (cat->size() > 0) {
		_categoryMap[categoryName] = cat;
	} else {
		std::cerr << "ERROR: EntityInspector: failed to create PropertyCategory "
				  << "\"" << categoryName << "\". Category contains no properties." << std::endl;
	}
}

// Create the actual UI components for the EntityInspector dialog

void EntityInspector::constructUI() {
    _widget = gtk_vpaned_new();
    
    gtk_paned_add1(GTK_PANED(_widget), createTreeViewPane());
    gtk_paned_add2(GTK_PANED(_widget), createDialogPane());
    
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
    _editorFrame = gtk_frame_new(NULL);
    gtk_box_pack_start(GTK_BOX(hbx), _editorFrame, TRUE, TRUE, 0);
    gtk_widget_set_size_request(hbx, 0, PROPERTYEDITORPANE_MIN_HEIGHT);
    return hbx;
}

// Create the TreeView pane

GtkWidget* EntityInspector::createTreeViewPane() {
    GtkWidget* vbx = gtk_vbox_new(FALSE, 0);

    // Initialise the instance TreeStore
    _treeStore = gtk_tree_store_new(N_COLUMNS, 
    							    G_TYPE_STRING, // property
    							    G_TYPE_STRING, // value
    							    G_TYPE_STRING, // value type
    							    GDK_TYPE_PIXBUF); // value icon
    
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
    g_signal_connect(G_OBJECT(_treeView), "cursor-changed", G_CALLBACK(callbackTreeSelectionChanged), this);
                                                                         
    // Embed the TreeView in a scrolled viewport
    GtkWidget* scrollWin = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrollWin), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(scrollWin), GTK_SHADOW_ETCHED_IN);
    gtk_widget_set_size_request(scrollWin, TREEVIEW_MIN_WIDTH, TREEVIEW_MIN_HEIGHT);
    gtk_container_add(GTK_CONTAINER(scrollWin), _treeView);    

    gtk_box_pack_start(GTK_BOX(vbx), scrollWin, TRUE, TRUE, 0);
    
    // Add the Unrecognised Properties text bar and advanced button.
    
    GtkWidget* advancedBar = gtk_hbox_new(FALSE, 0);
    gtk_container_set_border_width(GTK_CONTAINER(advancedBar), 3);

    _unrecognisedPropertiesMessage = gtk_label_new(NULL);
    GtkWidget* advancedButton = gtk_button_new_with_label("Advanced...");
    gtk_box_pack_start(GTK_BOX(advancedBar), _unrecognisedPropertiesMessage, FALSE, FALSE, 0);
    gtk_box_pack_end(GTK_BOX(advancedBar), advancedButton, FALSE, FALSE, 0);

    g_signal_connect(G_OBJECT(advancedButton), "clicked", G_CALLBACK(callbackAdvancedButtonClicked), this);

    gtk_box_pack_start(GTK_BOX(vbx), advancedBar, FALSE, FALSE, 0);
    
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
        
        // If the TreeStore is empty, populate it, otherwise refresh it with
        // values from the current entity.
        GtkTreeIter throwAwayIter;
        if (!gtk_tree_model_get_iter_first(GTK_TREE_MODEL(_treeStore), 
                                           &throwAwayIter)) { // returns FALSE if tree is empty
            populateTreeModel(); // add category tree
            refreshTreeModel(); // get values
        } else {
            refreshTreeModel(); // get values, already have category tree
        }
    }
    else {
        // Remove the displayed PropertyEditor
        if (_currentPropertyEditor) {
            delete _currentPropertyEditor;
            _currentPropertyEditor = NULL;
        }
		// Disable the dialog and clear the TreeView
        gtk_widget_set_sensitive(_widget, FALSE);
        gtk_tree_store_clear(_treeStore);
        gtk_label_set_text(GTK_LABEL(_unrecognisedPropertiesMessage), "");
        
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

void EntityInspector::selectionChanged(const Selectable& sel) {
    EntityInspector::redrawInstance();   
}

/* GTK CALLBACKS */

// Called when the TreeView selects a different property
void EntityInspector::callbackTreeSelectionChanged(GtkWidget* widget, EntityInspector* self) {
    self->updatePropertyEditor();
}

// Called when the Advanced button is clicked
void EntityInspector::callbackAdvancedButtonClicked(GtkWidget* widget, EntityInspector* self) {
    if (self->_allPropsDialog == NULL)
        self->_allPropsDialog = new AllPropertiesDialog();
    self->_allPropsDialog->show(self->_selectedEntity);
}

/* END GTK CALLBACKS */

// Update the PropertyEditor pane, displaying the PropertyEditor if necessary and
// making sure it refers to the currently-selected Entity.

void EntityInspector::updatePropertyEditor() {
    GtkTreeSelection* selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(_treeView));

    GtkTreeIter tmpIter;
    gtk_tree_selection_get_selected(selection, NULL, &tmpIter);

    // Get the selected key in the tree view
    GValue selString = {0, 0};
    gtk_tree_model_get_value(GTK_TREE_MODEL(_treeStore), &tmpIter, PROPERTY_NAME_COLUMN, &selString);
    const std::string key(g_value_get_string(&selString));
    g_value_unset(&selString);
    
    // Construct the PropertyEditor, destroying the current one if it exists. We
    // need the PROPERTY_TYPE_COLUMN string to find out what type of PropertyEditor
    // is needed.

    if (_currentPropertyEditor) {
        delete _currentPropertyEditor; // destructor takes care of GTK widgets
        _currentPropertyEditor = NULL;
    }
    
    GValue selType = {0, 0};
    gtk_tree_model_get_value(GTK_TREE_MODEL(_treeStore), &tmpIter, PROPERTY_TYPE_COLUMN, &selType);
    const std::string keyType(g_value_get_string(&selType));
    g_value_unset(&selType);
    
    _currentPropertyEditor = PropertyEditorFactory::create(keyType,
                                                           _selectedEntity,
                                                           key);
    if (_currentPropertyEditor != NULL) {
        gtk_container_add(GTK_CONTAINER(_editorFrame), _currentPropertyEditor->getWidget());
    }
}

// Populate TreeStore with the recognised properties, setting all values to
// empty for now.

void EntityInspector::populateTreeModel() {
    
    for (PropertyCategoryMap::iterator i = _categoryMap.begin();
			i != _categoryMap.end();
				i++) {

		// Create the top-level category node		
		GtkTreeIter categoryIter;
		gtk_tree_store_append(_treeStore, &categoryIter, NULL);
		gtk_tree_store_set(_treeStore, &categoryIter, PROPERTY_NAME_COLUMN, i->first.c_str(), PROPERTY_TYPE_COLUMN, "Category", -1);
		
		// Create the individual property sub-nodes.
		for (PropertyCategory::iterator j = i->second->begin(); j != i->second->end(); j++) {

			std::string keyName = j->first;
			std::string keyType = j->second;

	        GtkTreeIter tempIter;
	        gtk_tree_store_append(_treeStore, &tempIter, &categoryIter);
	        gtk_tree_store_set(_treeStore, &tempIter, PROPERTY_NAME_COLUMN, keyName.c_str(), -1);
	        gtk_tree_store_set(_treeStore, &tempIter, PROPERTY_VALUE_COLUMN, NO_VALUE_STRING.c_str(), -1); // no value yet
	        gtk_tree_store_set(_treeStore, &tempIter, PROPERTY_TYPE_COLUMN, keyType.c_str(), -1);

			std::string typeIcon = std::string("icon_") + keyType + ".png";
	        gtk_tree_store_set(_treeStore, &tempIter, PROPERTY_ICON_COLUMN, 
							   PropertyEditorFactory::getPixbufFor(keyType),
	        				   -1);
                               
            // Add the property to the set of known properties
            _knownProperties.insert(keyName);
				
		}
	}
}

// Refresh the values in the existing TreeModel. This avoids clearing the TreeStore
// and upsetting the current displayed position.

// Callback helper function to walk the GtkTreeModel
gboolean EntityInspector::treeWalkFunc(GtkTreeModel* model, GtkTreePath* path, GtkTreeIter* iter, gpointer data) {
    Entity* selectedEntity = reinterpret_cast<Entity*>(data);
    GValue gPropertyName = {0, 0};

    gtk_tree_model_get_value(model, iter, PROPERTY_NAME_COLUMN, &gPropertyName);
    const std::string property = g_value_get_string(&gPropertyName);
    g_value_unset(&gPropertyName);

    // If the property is not a top-level category, look it up in the map and
    // update its value
    if (gtk_tree_path_get_depth(path) == 2) {
        std::string newValue = selectedEntity->getKeyValue(property.c_str());
        if (newValue.size() > 0) { // property found on Entity
            gtk_tree_store_set(GTK_TREE_STORE(model), iter, PROPERTY_VALUE_COLUMN, newValue.c_str(), -1);
        } else {
            gtk_tree_store_set(GTK_TREE_STORE(model), iter, PROPERTY_VALUE_COLUMN, NO_VALUE_STRING.c_str(), -1);
        }            
    }
    return FALSE;  
}

// Main refresh function
void EntityInspector::refreshTreeModel() {

    // Iterate over the rows in the TreeModel, setting the appropriate column
    // to the value now on the _selectedEntity.
    gtk_tree_model_foreach(GTK_TREE_MODEL(_treeStore), treeWalkFunc, _selectedEntity);

    // Update the PropertyEditor pane, if a PropertyEditor is currently visible
    if (_currentPropertyEditor)
        updatePropertyEditor();
        
    // Update the unrecognised properties status message. Use a local visitor
    // class to count the number of properties on the entity, and subtract 
    // the number of recognised properties.
    
    struct PropertyCounter: public Entity::Visitor {

        // Number of unrecognised properties counted
        int count;
        
        // Set of recognised properties
        KnownPropertySet& knownSet;
        
        // Ctor
        PropertyCounter(KnownPropertySet& set): 
            count(0),
            knownSet(set) {
        }   
        
        // Required visit function
        virtual void visit(const char* key, const char* value) {
            if (knownSet.find(std::string(key)) == knownSet.end()) {
                ++count;
            }
        }
        
        // Format the output
        const char* getLabel() {
            std::stringstream str;
            if (count > 0)
                str << UNRECOGNISED_PROPERTIES_PREFIX << count << UNRECOGNISED_PROPERTIES_SUFFIX;
            else
                str << "";
            return str.str().c_str(); 
        }
    };
    
    PropertyCounter ctr(_knownProperties);
    _selectedEntity->forEachKeyValue(ctr);
    gtk_label_set_markup(GTK_LABEL(_unrecognisedPropertiesMessage), ctr.getLabel());

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
