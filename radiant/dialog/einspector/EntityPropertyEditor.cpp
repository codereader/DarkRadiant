#include "EntityPropertyEditor.h"

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
    _comboBox = gtk_combo_box_new_text();
    
    gtk_combo_box_append_text(GTK_COMBO_BOX(_comboBox), "Select entity...");

    std::string caption = getKey();
    caption.append(": ");
    gtk_box_pack_start(GTK_BOX(editBox), gtk_label_new(caption.c_str()), FALSE, FALSE, 0);
    gtk_box_pack_end(GTK_BOX(editBox), _comboBox, TRUE, TRUE, 0);
    
    GtkWidget* vbox = gtk_vbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), editBox, TRUE, FALSE, 0);
    
    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(getEditWindow()),
                                          vbox);
}

// Get and set value

void EntityPropertyEditor::setValue(const std::string& val) {}

const std::string EntityPropertyEditor::getValue() {}


}
