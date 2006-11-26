#include "PropertyEditor.h"

#include "entity.h"

#include "iundo.h"

#include <gtk/gtk.h>

#include <iostream>

namespace ui
{
	
// Blank constructor for registration in map

PropertyEditor::PropertyEditor() {}

// Constructor

PropertyEditor::PropertyEditor(Entity* ent, const std::string& key):
    _widget(gtk_vbox_new(FALSE, 0)),
	_entity(ent),
	_key(key),
	_applyButtonHbox(NULL),
	_editWindow(NULL)
{
    gtk_box_pack_start(GTK_BOX(_widget), getEditWindow(), TRUE, TRUE, 0);
    gtk_box_pack_end(GTK_BOX(_widget), getApplyButtonHbox(), FALSE, FALSE, 0);
}

// Destructor. Hide and destroy the Gtk widgets

PropertyEditor::~PropertyEditor() {
    gtk_widget_hide(_widget);
    gtk_widget_destroy(_widget);
}

// Return the apply buttons hbox

GtkWidget* PropertyEditor::getApplyButtonHbox() {
	if (_applyButtonHbox == NULL) {
		_applyButtonHbox = gtk_hbox_new(FALSE, 6);

		GtkWidget* applyButton = gtk_button_new_from_stock(GTK_STOCK_APPLY);
        g_signal_connect(G_OBJECT(applyButton), "clicked", G_CALLBACK(callbackApply), this); // "this" will refer to the Derived class here
        gtk_box_pack_end(GTK_BOX(_applyButtonHbox), applyButton, FALSE, FALSE, 0);

#ifdef PROPERTY_EDITOR_HAS_UNDO_BUTTON
		GtkWidget* resetButton = gtk_button_new_from_stock(GTK_STOCK_UNDO);
        g_signal_connect(G_OBJECT(resetButton), "clicked", G_CALLBACK(callbackReset), this);
        gtk_box_pack_end(GTK_BOX(_applyButtonHbox), resetButton, FALSE, FALSE, 0);
#endif

		gtk_container_set_border_width(GTK_CONTAINER(_applyButtonHbox), 3);
	}
	return _applyButtonHbox;	
}

// Return the central edit window

GtkWidget* PropertyEditor::getEditWindow() {
	if (_editWindow == NULL) {
		_editWindow = gtk_scrolled_window_new(NULL, NULL);
		gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(_editWindow),
										GTK_POLICY_AUTOMATIC,
										GTK_POLICY_AUTOMATIC);
	}
	return _editWindow;	
}

// Return the key and the Entity we are editing

const std::string& PropertyEditor::getKey() {
	return _key;
}

Entity* PropertyEditor::getEntity() {
	return _entity;
}

// Retrieve the current keyvalue from the Entity and call the virtual setValue() 
// function accordingly. Also update the status of the Active checkbox based on
// whether the key is actually set on the Entity.

void PropertyEditor::refresh() {
    gtk_widget_show_all(_widget);
    const std::string val = getEntity()->getKeyValue(getKey().c_str());
    if (val.size() > 0)
        setValue(val);
    else
        setValue("");        
}

/* 
 * GTK CALLBACKS
 */

void PropertyEditor::callbackApply(GtkWidget* caller, PropertyEditor* self) {
	const std::string newValue = self->getValue(); // retrieve the new keyval from the child

	// Save the undo information (also triggers "map changed" state).
	std::string cmd = "entitySetKeyValue -key \"" + self->_key 
					  + "\" -value \"" + newValue + "\"";

	GlobalUndoSystem().start();

	// Set key on entity
    self->_entity->setKeyValue(self->_key.c_str(), newValue.c_str());
        
	GlobalUndoSystem().finish(cmd.c_str());
}

} // namespace ui
