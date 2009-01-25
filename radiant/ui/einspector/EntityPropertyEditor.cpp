#include "EntityPropertyEditor.h"

#include "iscenegraph.h"
#include "ientity.h"
#include "scenelib.h"

#include <gtk/gtk.h>

namespace ui {

// Blank ctor
EntityPropertyEditor::EntityPropertyEditor() :
	ComboBoxPropertyEditor()
{}

// Constructor. Create the GTK widgets here

EntityPropertyEditor::EntityPropertyEditor(Entity* entity, const std::string& name) :
   ComboBoxPropertyEditor(entity, name)
{
	populateComboBox();
}

// Traverse the scenegraph to populate the combo box
void EntityPropertyEditor::populateComboBox() {

    // Create a scenegraph walker to traverse the graph
  
    struct EntityFinder: 
		public scene::NodeVisitor
	{
        // List store to add to
        GtkTreeModel* _store;
        
        // Constructor
		EntityFinder(GtkWidget* box) :
			_store(gtk_combo_box_get_model(GTK_COMBO_BOX(box)))
		{}
            
        // Visit function
        bool pre(const scene::INodePtr& node) {
			// Check for an entity
            Entity* entity = Node_getEntity(node);

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

	Node_traverseSubgraph(GlobalSceneGraph().root(), finder);
}

} // namespace ui
