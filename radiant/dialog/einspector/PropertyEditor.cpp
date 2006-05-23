#include "PropertyEditor.h"
#include "PropertyEditorFactory.h"
#include "EntityKeyValueVisitor.h"

#include "exception/InvalidKeyException.h"

#include <gtk/gtk.h>

#include <iostream>

namespace ui
{
	
// Constructor

PropertyEditor::PropertyEditor(Entity* ent, const std::string& key, const std::string& type):
	_entity(ent),
	_key(key),
	_type(type),
	_applyButtonHbox(NULL),
	_titleBox(NULL),
	_editWindow(NULL),
	_activeCheckbox(NULL) {
}

// Return the apply buttons hbox

GtkWidget* PropertyEditor::getApplyButtonHbox() {
	if (_applyButtonHbox == NULL) {
		_applyButtonHbox = gtk_hbox_new(FALSE, 6);

		GtkWidget* applyButton = gtk_button_new_from_stock(GTK_STOCK_APPLY);
		GtkWidget* resetButton = gtk_button_new_from_stock(GTK_STOCK_UNDO);

		g_signal_connect(G_OBJECT(applyButton), "clicked", G_CALLBACK(callbackApply), this); // "this" will refer to the Derived class here
		g_signal_connect(G_OBJECT(resetButton), "clicked", G_CALLBACK(callbackReset), this);

		gtk_box_pack_end(GTK_BOX(_applyButtonHbox), applyButton, FALSE, FALSE, 0);
		gtk_box_pack_end(GTK_BOX(_applyButtonHbox), resetButton, FALSE, FALSE, 0);
		gtk_container_set_border_width(GTK_CONTAINER(_applyButtonHbox), 3);
	}
	return _applyButtonHbox;	
}

// Return the title bar box

GtkWidget* PropertyEditor::getTitleBox() {
	if (_titleBox == NULL) {
		_titleBox = gtk_hbox_new(FALSE, 3);

		// Icon (top-right)
		gtk_box_pack_end(GTK_BOX(_titleBox), 
							gtk_image_new_from_pixbuf(PropertyEditorFactory::getPixbufFor(_type)),
							FALSE, FALSE, 0);
		gtk_box_pack_end(GTK_BOX(_titleBox), gtk_vseparator_new(), FALSE, FALSE, 0);

		// Enabled checkbox (top-left)
		_activeCheckbox = gtk_check_button_new();
		gtk_box_pack_start(GTK_BOX(_titleBox), _activeCheckbox, FALSE, FALSE, 0);
		gtk_box_pack_start(GTK_BOX(_titleBox), gtk_vseparator_new(), FALSE, FALSE, 0);

		g_signal_connect(G_OBJECT(_activeCheckbox), "toggled", 
		                 G_CALLBACK(callbackActiveToggled), this);

		// Bold label
		std::string boldKey = std::string("<big>") + _key + std::string("</big>");
		GtkWidget* titleLabel = gtk_label_new(NULL);
		gtk_label_set_markup(GTK_LABEL(titleLabel), boldKey.c_str());

		gtk_box_pack_start(GTK_BOX(_titleBox), titleLabel, FALSE, FALSE, 0);

		gtk_container_set_border_width(GTK_CONTAINER(_titleBox), 3);
	}
	return _titleBox;
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
    try {
        const std::string val = EntityKeyValueVisitor::getKeyValue(getEntity(), getKey());
        setValue(val);
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(_activeCheckbox), TRUE);
        gtk_toggle_button_toggled(GTK_TOGGLE_BUTTON(_activeCheckbox)); // force a toggle to correctly set sensitivity of the edit pane
    } catch (InvalidKeyException e) {
        setValue("");        
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(_activeCheckbox), FALSE);
        gtk_toggle_button_toggled(GTK_TOGGLE_BUTTON(_activeCheckbox));
    }
}

/* 
 * GTK CALLBACKS
 */

inline void PropertyEditor::callbackApply(GtkWidget* caller, PropertyEditor* self) {
	const std::string newValue(self->getValue()); // retrieve the new keyval from the child
    std::cout << "getValue() returned " << newValue << std::endl;
}

inline void PropertyEditor::callbackReset(GtkWidget* caller, PropertyEditor* self) {
	self->refresh();
}

// Callback for when the active checkbox is toggled. This enables or disables
// the central edit pane.

inline void PropertyEditor::callbackActiveToggled(GtkWidget* caller, PropertyEditor* self) {
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(caller)))
		gtk_widget_set_sensitive(self->_editWindow, TRUE);
	else 
		gtk_widget_set_sensitive(self->_editWindow, FALSE);
}

} // namespace ui
