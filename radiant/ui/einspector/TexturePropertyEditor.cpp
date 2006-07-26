#include "TexturePropertyEditor.h"

namespace ui
{

// Main constructor

TexturePropertyEditor::TexturePropertyEditor(Entity* entity, const std::string& name)
: PropertyEditor(entity, name, "texture") 
{
	GtkWidget* editBox = gtk_hbox_new(FALSE, 3);
	gtk_container_set_border_width(GTK_CONTAINER(editBox), 3);
	_textEntry = gtk_entry_new();
	
	std::string caption = getKey();
	caption.append(": ");
	gtk_box_pack_start(GTK_BOX(editBox), gtk_label_new(caption.c_str()), FALSE, FALSE, 0);
	gtk_box_pack_end(GTK_BOX(editBox), _textEntry, TRUE, TRUE, 0);
	
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(getEditWindow()),
										  editBox);
}

// Get and set functions

void TexturePropertyEditor::setValue(const std::string& val) {
	gtk_entry_set_text(GTK_ENTRY(_textEntry), val.c_str());
}

const std::string TexturePropertyEditor::getValue() {
	return std::string(gtk_entry_get_text(GTK_ENTRY(_textEntry)));
}

} // namespace ui
