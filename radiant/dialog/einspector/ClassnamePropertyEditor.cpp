#include "ClassnamePropertyEditor.h"

#include "ieclass.h"
#include "eclasslib.h"

#include <iostream>

namespace ui
{

// Blank ctor

ClassnamePropertyEditor::ClassnamePropertyEditor() {}

// Constructor. Create the GTK widgets here

ClassnamePropertyEditor::ClassnamePropertyEditor(Entity* entity, const std::string& name):
    PropertyEditor(entity, name, "classname")
{
    GtkWidget* editBox = gtk_hbox_new(FALSE, 3);
    gtk_container_set_border_width(GTK_CONTAINER(editBox), 3);

    // Set up the sorted TreeModel
    
    GtkTreeModel* sortedModel = gtk_tree_model_sort_new_with_model(
                                    GTK_TREE_MODEL(
                                        gtk_list_store_new(1, G_TYPE_STRING)));
    gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(sortedModel), 0, GTK_SORT_ASCENDING);
    
    _comboBox = gtk_combo_box_entry_new_with_model(sortedModel, 0); 
    gtk_editable_set_editable(GTK_EDITABLE(GTK_BIN(_comboBox)->child), false);

    populateComboBox();

    // Pack widgets into the edit area
    
    std::string caption = getKey();
    caption.append(": ");
    gtk_box_pack_start(GTK_BOX(editBox), gtk_label_new(caption.c_str()), FALSE, FALSE, 0);
    gtk_box_pack_end(GTK_BOX(editBox), _comboBox, TRUE, TRUE, 0);
    
    GtkWidget* vbox = gtk_vbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), editBox, TRUE, FALSE, 0);
    
    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(getEditWindow()),
                                          vbox);
}

// Visit the set of Entity Classes to populate the combobox.

void ClassnamePropertyEditor::populateComboBox() {

    // Create an EntityClassVisitor to populate the combo box with entity
    // classnames.
    
    struct EClassVisitor: public EntityClassVisitor {
        
        // Combobox to populate
        GtkTreeModel* _store;
        
        // Constructor
        EClassVisitor(GtkWidget* box) {
            _store = gtk_tree_model_sort_get_model(
                        GTK_TREE_MODEL_SORT(gtk_combo_box_get_model(GTK_COMBO_BOX(box))));
        }
        
        // Visit function
        virtual void visit(EntityClass* eclass) {
            GtkTreeIter iter;
            gtk_list_store_append(GTK_LIST_STORE(_store), &iter);
            gtk_list_store_set(GTK_LIST_STORE(_store), &iter, 0, eclass->name(), -1);
        }
        
    } visitor(_comboBox);
    
    GlobalEntityClassManager().forEach(visitor);
}

// Get and set value

void ClassnamePropertyEditor::setValue(const std::string& val) {
    gtk_entry_set_text(GTK_ENTRY(GTK_BIN(_comboBox)->child), val.c_str());
}

const std::string ClassnamePropertyEditor::getValue() {
    return std::string(gtk_combo_box_get_active_text(GTK_COMBO_BOX(_comboBox)));
}


}
