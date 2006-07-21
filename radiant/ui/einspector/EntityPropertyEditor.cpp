#include "EntityPropertyEditor.h"

#include "iscenegraph.h"
#include "ientity.h"
#include "scenelib.h"

#include <iostream>

namespace ui
{

// Blank ctor

EntityPropertyEditor::EntityPropertyEditor() {}

// Constructor. Create the GTK widgets here

EntityPropertyEditor::EntityPropertyEditor(Entity* entity, const std::string& name):
    PropertyEditor(entity, name, "entity")
{
    GtkWidget* editBox = gtk_hbox_new(FALSE, 3);
    gtk_container_set_border_width(GTK_CONTAINER(editBox), 3);

    // Set up the sorted TreeModel
    
    GtkTreeModel* sortedModel = gtk_tree_model_sort_new_with_model(
                                    GTK_TREE_MODEL(
                                        gtk_list_store_new(1, G_TYPE_STRING)));
    gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(sortedModel), 0, GTK_SORT_ASCENDING);
    
    _comboBox = gtk_combo_box_entry_new_with_model(sortedModel, 0); // number of the "text" column
    populateComboBox();

    std::string caption = getKey();
    caption.append(": ");
    gtk_box_pack_start(GTK_BOX(editBox), gtk_label_new(caption.c_str()), FALSE, FALSE, 0);
    gtk_box_pack_end(GTK_BOX(editBox), _comboBox, TRUE, TRUE, 0);
    
    GtkWidget* vbox = gtk_vbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), editBox, TRUE, FALSE, 0);
    
    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(getEditWindow()),
                                          vbox);
}

// Traverse the scenegraph to populate the combo box

void EntityPropertyEditor::populateComboBox() {

    // Create a scenegraph walker to traverse the graph
    
    struct EntityFinder: public scene::Graph::Walker {
        
        // List store to add to
        GtkTreeModel* _store;
        
        // Constructor
        EntityFinder(GtkWidget* box) {
            _store = gtk_tree_model_sort_get_model(
                        GTK_TREE_MODEL_SORT(gtk_combo_box_get_model(GTK_COMBO_BOX(box))));
        }
            
        // Visit function
        virtual bool pre(const scene::Path& path, scene::Instance& instance) const {
            Entity* entity = Node_getEntity(path.top());
            if (entity != NULL) {

                const char* entName = entity->getKeyValue("name");

                GtkTreeIter iter;
                gtk_list_store_append(GTK_LIST_STORE(_store), &iter);
                gtk_list_store_set(GTK_LIST_STORE(_store), &iter, 0, entName, -1);

                return false; // don't traverse children if entity found
                
            }
            
            return true; // traverse children otherwise
        }
            
    } finder(_comboBox);

    GlobalSceneGraph().traverse(finder);
    
}

// Get and set value

void EntityPropertyEditor::setValue(const std::string& val) {
    gtk_entry_set_text(GTK_ENTRY(GTK_BIN(_comboBox)->child), val.c_str());
}

const std::string EntityPropertyEditor::getValue() {
    return gtk_entry_get_text(GTK_ENTRY(GTK_BIN(_comboBox)->child));
}


} // namespace ui
