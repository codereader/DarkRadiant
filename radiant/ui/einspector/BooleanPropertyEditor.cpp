#include "BooleanPropertyEditor.h"

#include <gtk/gtk.h>

namespace ui
{

// Blank ctor

BooleanPropertyEditor::BooleanPropertyEditor() {}

// Constructor. Create the GTK widgets here

BooleanPropertyEditor::BooleanPropertyEditor(Entity* entity, 
											 const std::string& name)
: _widget(gtk_vbox_new(FALSE, 6)),
  _entity(entity)
{
	GtkWidget* editBox = gtk_hbox_new(FALSE, 3);
	gtk_container_set_border_width(GTK_CONTAINER(editBox), 3);

	_checkBox = gtk_check_button_new_with_label(name.c_str());
	
	gtk_box_pack_start(GTK_BOX(editBox), _checkBox, TRUE, FALSE, 0);
	
	gtk_box_pack_start(GTK_BOX(_widget), editBox, TRUE, TRUE, 0);
}

}
