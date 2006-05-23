#include "EntityInspector.h"
#include "EntityKeyValueVisitor.h"
#include "PropertyEditor.h"
#include "PropertyEditorFactory.h"

#include "ientity.h"
#include "iselection.h"

#include "gtkutil/image.h"
#include "gtkutil/dialog.h"

#include "error.h"

#include <libxml/parser.h>

#include <iostream>

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

// Static function to parse the <entityInspector> node in the .game file.

void EntityInspector::parseXmlNode(xmlNodePtr node) {
//	gtkutil::errorDialog("Unable to parse the <entityInspector> node in the .game file");
	node = node->children;

	// Search for <propertyCategory> nodes and create a PropertyCategory for each
	// one.
	for ( ; node != NULL; node = node->next) {
		if (node->type == XML_ELEMENT_NODE && 
			    xmlStrcmp(node->name, (const xmlChar*) "propertyCategory") == 0 ) {
			makePropertyCategory(node);
		}
	}
}

void EntityInspector::makePropertyCategory(xmlNodePtr node) {
	if (node->properties != NULL && node->properties->children != NULL) {

		// Find the "name" attribute and construct a PropertyCategory with its value
		if (xmlStrcmp(node->properties->name, (const xmlChar*) "name") == 0) { // name=

			// Get the category name
			std::string categoryName((const char*) node->properties->children->content);
			PropertyCategory* cat = new PropertyCategory(); 

			// Now search for <property> elements underneath <propertyCategory>
			for (xmlNodePtr child = node->children; child != NULL; child = child->next) {

				if (xmlStrcmp(child->name, (const xmlChar*) "property") == 0) {

					// Got a property node. Iterate over its attributes and search
					// for the name= and type= attributes.
					std::string keyName("");
					std::string keyType("");

					for (xmlAttrPtr props = child->properties; props != NULL; props = props->next) {

						xmlChar *content = props->children->content;

						const xmlChar *name = props->name;
						if (xmlStrcmp(name, (const xmlChar*) "name") == 0)
							keyName = std::string((char*) content);
						else if (xmlStrcmp(name, (const xmlChar*) "type") == 0)
							keyType = std::string((char*) content);
					}

					// Add the name and type to the PropertyCategory object, but
					// only if we found both attributes
					if (keyName != "" && keyType != "") {
						cat->insert(PropertyCategory::value_type(keyName, keyType));
					} else {
						gtkutil::errorDialog(std::string("EntityInspector: failed to parse XML configuration file")
											+ "\n\nEither the \"name\" or the \"type\" field was not found in a property"
											+ " of category \"" + categoryName + "\".");
					}
				}			
			}
			
			// Add the PropertyCategory to the category map, as long as it is not
			// empty.
			if (cat->size() > 0) {
				_categoryMap[categoryName] = cat;
			} else {
				gtkutil::errorDialog(std::string("EntityInspector: failed to create PropertyCategory ")
									+ "\"" + categoryName + "\".\n\nCategory contains no properties.");
			}
		}
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
    gtk_widget_set_size_request(hbx, 0, 200);
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
    GValue selString = {0, 0};
    gtk_tree_model_get_value(GTK_TREE_MODEL(self->_treeStore), &tmpIter, PROPERTY_NAME_COLUMN, &selString);
    const std::string key(g_value_get_string(&selString));
    g_value_unset(&selString);
    
    // Construct the PropertyEditor, destroying the current one if it exists. We
    // need the PROPERTY_TYPE_COLUMN string to find out what type of PropertyEditor
    // is needed.

    if (self->_currentPropertyEditor) {
    	delete self->_currentPropertyEditor; // destructor takes care of GTK widgets
    }
    
    GValue selType = {0, 0};
    gtk_tree_model_get_value(GTK_TREE_MODEL(self->_treeStore), &tmpIter, PROPERTY_TYPE_COLUMN, &selType);
    const std::string keyType(g_value_get_string(&selType));
    g_value_unset(&selType);
	
    self->_currentPropertyEditor = PropertyEditorFactory::create(keyType,
    						 							   self->_selectedEntity,
    													   key);
	if (self->_currentPropertyEditor != NULL) {
	    gtk_container_add(GTK_CONTAINER(self->_editorFrame), self->_currentPropertyEditor->getWidget());
	}
}

// Populate TreeStore with current selections' keyvals

void EntityInspector::populateTreeModel() {
    
    // Create a visitor and use it to obtain the key/value map
    EntityKeyValueVisitor visitor;
    _selectedEntity->forEachKeyValue(visitor);
    KeyValueMap kvMap = visitor.getMap();
    
    // Clear the current TreeStore, and add the keys to it
    gtk_tree_store_clear(_treeStore);

	for (PropertyCategoryMap::iterator i = _categoryMap.begin();
			i != _categoryMap.end();
				i++) {

		// Create the top-level category node		
		GtkTreeIter categoryIter;
		gtk_tree_store_append(_treeStore, &categoryIter, NULL);
		gtk_tree_store_set(_treeStore, &categoryIter, PROPERTY_NAME_COLUMN, i->first.c_str(), PROPERTY_TYPE_COLUMN, "Category", -1);
		
		// Create the individual property sub-nodes.
		for (PropertyCategory::iterator j = i->second->begin(); j != i->second->end(); j++) {

			std::string keyName(j->first);
			std::string keyType(j->second);

			// Find out whether this key is set on the entity (from the kvMap
			// produced earlier. If it is, store it otherwise we use a "not set"
			// signal.
			
			KeyValueMap::iterator tmp(kvMap.find(keyName.c_str()));
			std::string keyValue;
			
			if (tmp == kvMap.end())
				keyValue = "--";
			else
				keyValue = tmp->second;
			
	        GtkTreeIter tempIter;
	        gtk_tree_store_append(_treeStore, &tempIter, &categoryIter);
	        gtk_tree_store_set(_treeStore, &tempIter, PROPERTY_NAME_COLUMN, keyName.c_str(), -1);
	        gtk_tree_store_set(_treeStore, &tempIter, PROPERTY_VALUE_COLUMN, keyValue.c_str(), -1);
	        gtk_tree_store_set(_treeStore, &tempIter, PROPERTY_TYPE_COLUMN, keyType.c_str(), -1);

			std::string typeIcon = std::string("icon_") + keyType + ".png";
	        gtk_tree_store_set(_treeStore, &tempIter, PROPERTY_ICON_COLUMN, 
							   PropertyEditorFactory::getPixbufFor(keyType),
	        				   -1);
				
		}
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
