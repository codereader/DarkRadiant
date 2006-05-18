#include "PropertyEditor.h"

#include <gtk/gtk.h>

#include <iostream>

namespace ui
{
	
// Constructor

PropertyEditor::PropertyEditor(Entity* ent, const std::string& key):
	_entity(ent),
	_key(key),
	_applyButtonHbox(NULL),
	_titleBox(NULL) {
}

// Return the apply buttons hbox
GtkWidget* PropertyEditor::getApplyButtonHbox() {
	if (_applyButtonHbox == NULL) {
		_applyButtonHbox = gtk_hbox_new(FALSE, 6);
		gtk_box_pack_end(GTK_BOX(_applyButtonHbox), gtk_button_new_from_stock(GTK_STOCK_APPLY), FALSE, FALSE, 0);
		gtk_box_pack_end(GTK_BOX(_applyButtonHbox), gtk_button_new_from_stock(GTK_STOCK_UNDO), FALSE, FALSE, 0);
	}
	return _applyButtonHbox;	
}

// Return the title bar box
GtkWidget* PropertyEditor::getTitleBox() {
	if (_titleBox == NULL) {
		_titleBox = gtk_hbox_new(FALSE, 6);
		gtk_box_pack_start(GTK_BOX(_titleBox), gtk_label_new(_key.c_str()), FALSE, FALSE, 0);
	}
	return _titleBox;
}

} // namespace ui
