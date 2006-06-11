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

    _comboBox = gtk_combo_box_entry_new_text();
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
        
        // Combo box to add to
        GtkWidget* _box;
        
        // Constructor
        EntityFinder(GtkWidget* box): _box(box) {}
            
        // Visit function
        virtual bool pre(const scene::Path& path, scene::Instance& instance) const {
            Entity* entity = Node_getEntity(path.top());
            if (entity != NULL) {
                const char* entName = entity->getKeyValue("name");
                gtk_combo_box_append_text(GTK_COMBO_BOX(_box), entName);
                return false; // don't traverse children
            }
            return true;
        }
            
    } finder(_comboBox);

    GlobalSceneGraph().traverse(finder);
    
}

// Get and set value

void EntityPropertyEditor::setValue(const std::string& val) {
    gtk_entry_set_text(GTK_ENTRY(GTK_BIN(_comboBox)->child), val.c_str());
}

const std::string EntityPropertyEditor::getValue() {
    return std::string(gtk_combo_box_get_active_text(GTK_COMBO_BOX(_comboBox)));
}


}
