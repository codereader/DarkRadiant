#include "BooleanPropertyEditor.h"

#include "ientity.h"

#include <gtk/gtk.h>

namespace ui
{

// Blank ctor
BooleanPropertyEditor::BooleanPropertyEditor() {}

// Constructor. Create the GTK widgets here
BooleanPropertyEditor::BooleanPropertyEditor(Entity* entity, 
											 const std::string& name)
: _entity(entity),
  _key(name)
{
	_widget = gtk_vbox_new(FALSE, 6);
	
	GtkWidget* editBox = gtk_hbox_new(FALSE, 3);
	gtk_container_set_border_width(GTK_CONTAINER(editBox), 3);

	// Create the checkbox with correct initial state, and connect up the
	// toggle callback
	_checkBox = gtk_check_button_new_with_label(name.c_str());
	gtk_toggle_button_set_active(
		GTK_TOGGLE_BUTTON(_checkBox), 
		_entity->getKeyValue(_key) == "1" ? TRUE : FALSE
	);
	g_signal_connect(
		G_OBJECT(_checkBox), "toggled", G_CALLBACK(_onToggle), this
	);
	
	gtk_box_pack_start(GTK_BOX(editBox), _checkBox, TRUE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(_widget), editBox, TRUE, TRUE, 0);
}

/* GTK callbacks */

void BooleanPropertyEditor::_onToggle(GtkWidget* w, BooleanPropertyEditor* self)
{
	// Set the key based on the checkbutton state
	gboolean state = 
		gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(self->_checkBox));
	if (state)
		self->_entity->setKeyValue(self->_key, "1");
	else
		self->_entity->setKeyValue(self->_key, "0");
	
}


}
