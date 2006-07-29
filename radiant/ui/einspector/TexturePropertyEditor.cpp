#include "TexturePropertyEditor.h"
#include "TextureChooser.h"

#include "ishaders.h"
#include "generic/callback.h"

#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>

namespace ui
{

// Main constructor

TexturePropertyEditor::TexturePropertyEditor(Entity* entity, const std::string& name, const std::string& options)
: PropertyEditor(entity, name, "texture"),
  _prefixes(options)
{
	GtkWidget* outer = gtk_vbox_new(FALSE, 0);

	GtkWidget* editBox = gtk_hbox_new(FALSE, 3);
	gtk_container_set_border_width(GTK_CONTAINER(editBox), 3);
	_textEntry = gtk_entry_new();
	
	std::string caption = getKey();
	caption.append(": ");
	gtk_box_pack_start(GTK_BOX(editBox), gtk_label_new(caption.c_str()), FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(editBox), _textEntry, TRUE, TRUE, 0);

	GtkWidget* browseButton = gtk_button_new_with_label("...");
	gtk_box_pack_start(GTK_BOX(editBox), browseButton, FALSE, FALSE, 0);
	g_signal_connect(G_OBJECT(browseButton), "clicked", G_CALLBACK(callbackBrowse), this);
	
	gtk_box_pack_start(GTK_BOX(outer), editBox, TRUE, FALSE, 0);
	
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(getEditWindow()),
										  outer);
}

// Browse button callback, with local functor object

void TexturePropertyEditor::callbackBrowse(GtkWidget* widget, TexturePropertyEditor* self) {
	new TextureChooser(self->_textEntry, self->_prefixes); // self-destructs on close
}

// Get and set functions

void TexturePropertyEditor::setValue(const std::string& val) {
	gtk_entry_set_text(GTK_ENTRY(_textEntry), val.c_str());
}

const std::string TexturePropertyEditor::getValue() {
	return std::string(gtk_entry_get_text(GTK_ENTRY(_textEntry)));
}

} // namespace ui
