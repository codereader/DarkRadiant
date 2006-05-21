#include "PropertyEditor.h"
#include "PropertyEditorFactory.h"

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
	_editWindow(NULL) {
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

		// Icon
		std::cout << "PE: type is " << _type << std::endl;
		gtk_box_pack_start(GTK_BOX(_titleBox), 
							gtk_image_new_from_pixbuf(PropertyEditorFactory::getPixbufFor(_type)),
							FALSE, FALSE, 0);
		gtk_box_pack_start(GTK_BOX(_titleBox), gtk_vseparator_new(), FALSE, FALSE, 0);

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

std::string PropertyEditor::getKey() {
	return _key;
}

Entity* PropertyEditor::getEntity() {
	return _entity;
}

// Gtk callbacks. These just invoke the respective functions on the derived
// class, which is passed as the second argument to the callback.

inline void PropertyEditor::callbackApply(GtkWidget* caller, gpointer self) {
	reinterpret_cast<PropertyEditor*>(self)->commit();
}

inline void PropertyEditor::callbackReset(GtkWidget* caller, gpointer self) {
	reinterpret_cast<PropertyEditor*>(self)->refresh();
}

} // namespace ui
