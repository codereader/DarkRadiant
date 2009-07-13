#include "ComboBoxPropertyEditor.h"

#include "iscenegraph.h"
#include "ientity.h"
#include "scenelib.h"

#include "gtkutil/RightAlignment.h"

#include <gtk/gtk.h>

namespace ui
{

// Blank ctor
ComboBoxPropertyEditor::ComboBoxPropertyEditor() :
	_comboBox(NULL),
	_key("")
{}

// Constructor. Create the GTK widgets here
ComboBoxPropertyEditor::ComboBoxPropertyEditor(Entity* entity, const std::string& name) :
	PropertyEditor(entity),
	_key(name)
{
	_widget = gtk_vbox_new(FALSE, 6);
	
    GtkWidget* editBox = gtk_hbox_new(FALSE, 3);
    gtk_container_set_border_width(GTK_CONTAINER(editBox), 3);

    // Set up the combobox TreeModel
    _comboBox = gtk_combo_box_entry_new_with_model(
    				GTK_TREE_MODEL(gtk_list_store_new(1, G_TYPE_STRING)),
    				0); // number of the "text" column
    
    // Add completion functionality to the combobox entry
    GtkEntryCompletion* completion = gtk_entry_completion_new();
    gtk_entry_completion_set_model(
    	completion, 
    	GTK_TREE_MODEL(gtk_combo_box_get_model(GTK_COMBO_BOX(_comboBox)))
    );
	gtk_entry_completion_set_text_column(completion, 0);
    gtk_entry_set_completion(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(_comboBox))), 
    						 completion);

    // Create the Apply button
    GtkWidget* applyButton = gtk_button_new_from_stock(GTK_STOCK_APPLY);
    g_signal_connect(
    	G_OBJECT(applyButton), "clicked", G_CALLBACK(onApply), this
    );
    
    // Pack elements into main box
    std::string caption = name + ": ";
    gtk_box_pack_start(GTK_BOX(editBox), gtk_label_new(caption.c_str()), 
    				   FALSE, FALSE, 0);
    gtk_box_pack_end(GTK_BOX(editBox), _comboBox, TRUE, TRUE, 0);
    
    GtkWidget* vbox = gtk_vbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), editBox, TRUE, FALSE, 0);
    gtk_box_pack_end(
    	GTK_BOX(vbox), gtkutil::RightAlignment(applyButton), FALSE, FALSE, 0
    );
    
    gtk_container_set_border_width(GTK_CONTAINER(_widget), 6);
    gtk_box_pack_start(GTK_BOX(_widget), vbox, TRUE, TRUE, 0);
}

void ComboBoxPropertyEditor::onApply(GtkWidget *widget, 
									 ComboBoxPropertyEditor* self) 
{
	self->setKeyValue(
		self->_key, 
		gtk_combo_box_get_active_text(GTK_COMBO_BOX(self->_comboBox))
	);
}

} // namespace ui
