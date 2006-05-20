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
	_titleBox(NULL) {
}

// Return the apply buttons hbox
GtkWidget* PropertyEditor::getApplyButtonHbox() {
	if (_applyButtonHbox == NULL) {
		_applyButtonHbox = gtk_hbox_new(FALSE, 6);
		gtk_box_pack_end(GTK_BOX(_applyButtonHbox), gtk_button_new_from_stock(GTK_STOCK_APPLY), FALSE, FALSE, 0);
		gtk_box_pack_end(GTK_BOX(_applyButtonHbox), gtk_button_new_from_stock(GTK_STOCK_UNDO), FALSE, FALSE, 0);
		gtk_container_set_border_width(GTK_CONTAINER(_applyButtonHbox), 3);
	}
	return _applyButtonHbox;	
}

// Return the title bar box
GtkWidget* PropertyEditor::getTitleBox() {
	if (_titleBox == NULL) {
		_titleBox = gtk_hbox_new(FALSE, 6);
		gtk_box_pack_start(GTK_BOX(_titleBox), 
							gtk_image_new_from_pixbuf(PropertyEditorFactory::getPixbufFor(_type)),
							FALSE, FALSE, 0);

		// Bold label with icon
		std::string boldKey = std::string("<big>") + _key + std::string("</big>");
		GtkWidget* titleLabel = gtk_label_new(NULL);
		gtk_label_set_markup(GTK_LABEL(titleLabel), boldKey.c_str());

		gtk_box_pack_start(GTK_BOX(_titleBox), titleLabel, FALSE, FALSE, 0);
		gtk_container_set_border_width(GTK_CONTAINER(_titleBox), 3);
	}
	return _titleBox;
}

} // namespace ui
