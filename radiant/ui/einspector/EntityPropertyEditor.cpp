#include "EntityPropertyEditor.h"

#include "iscenegraph.h"
#include "ientity.h"
#include "scenelib.h"

#include <gtk/gtk.h>

namespace ui
{

// Blank ctor

EntityPropertyEditor::EntityPropertyEditor() {}

// Constructor. Create the GTK widgets here

EntityPropertyEditor::EntityPropertyEditor(Entity* entity, 
										   const std::string& name)
{
	_widget = gtk_vbox_new(FALSE, 6);
	
    GtkWidget* editBox = gtk_hbox_new(FALSE, 3);
    gtk_container_set_border_width(GTK_CONTAINER(editBox), 3);

    // Set up the combobox TreeModel
    _comboBox = gtk_combo_box_entry_new_with_model(
    				GTK_TREE_MODEL(gtk_list_store_new(1, G_TYPE_STRING)),
    				0); // number of the "text" column
    populateComboBox();
    
    // Add completion functionality to the combobox entry
    GtkEntryCompletion* completion = gtk_entry_completion_new();
    gtk_entry_completion_set_model(
    	completion, 
    	GTK_TREE_MODEL(gtk_combo_box_get_model(GTK_COMBO_BOX(_comboBox)))
    );
	gtk_entry_completion_set_text_column(completion, 0);
    gtk_entry_set_completion(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(_comboBox))), 
    						 completion);

    std::string caption = name + ": ";
    gtk_box_pack_start(GTK_BOX(editBox), gtk_label_new(caption.c_str()), 
    				   FALSE, FALSE, 0);
    gtk_box_pack_end(GTK_BOX(editBox), _comboBox, TRUE, TRUE, 0);
    
    GtkWidget* vbox = gtk_vbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), editBox, TRUE, FALSE, 0);
    
    gtk_box_pack_start(GTK_BOX(_widget), vbox, TRUE, TRUE, 0);
}

// Traverse the scenegraph to populate the combo box

void EntityPropertyEditor::populateComboBox() {

    // Create a scenegraph walker to traverse the graph
    
    struct EntityFinder: public scene::Graph::Walker {
        
        // List store to add to
        GtkTreeModel* _store;
        
        // Constructor
        EntityFinder(GtkWidget* box) {
            _store = gtk_combo_box_get_model(GTK_COMBO_BOX(box));
        }
            
        // Visit function
        virtual bool pre(const scene::Path& path, 
        				 scene::Instance& instance) const 
		{
            Entity* entity = Node_getEntity(path.top());
            if (entity != NULL) {

				// Get the entity name
                std::string entName = entity->getKeyValue("name");

				// Append the name to the list store
                GtkTreeIter iter;
                gtk_list_store_append(GTK_LIST_STORE(_store), &iter);
                gtk_list_store_set(GTK_LIST_STORE(_store), &iter, 
                				   0, entName.c_str(), 
                				   -1);

                return false; // don't traverse children if entity found
                
            }
            
            return true; // traverse children otherwise
        }
            
    } finder(_comboBox);

    GlobalSceneGraph().traverse(finder);
    
}

} // namespace ui
