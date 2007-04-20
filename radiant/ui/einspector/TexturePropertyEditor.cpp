#include "TexturePropertyEditor.h"
#include "LightTextureChooser.h"
#include "PropertyEditorFactory.h"

#include "ientity.h"
#include "generic/callback.h"

#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>

namespace ui
{

// Main constructor
TexturePropertyEditor::TexturePropertyEditor(Entity* entity, 
											 const std::string& name, 
											 const std::string& options)
: _prefixes(options),
  _entity(entity),
  _key(name)
{
	_widget = gtk_vbox_new(FALSE, 6);
	
	GtkWidget* outer = gtk_vbox_new(FALSE, 0);
	GtkWidget* editBox = gtk_hbox_new(FALSE, 3);

	// Create the browse button	
	GtkWidget* browseButton = gtk_button_new_with_label("Choose texture...");
	gtk_button_set_image(
		GTK_BUTTON(browseButton),
		gtk_image_new_from_pixbuf(
			PropertyEditorFactory::getPixbufFor("texture")
		)
	);
	g_signal_connect(
		G_OBJECT(browseButton), "clicked", G_CALLBACK(callbackBrowse), this
	);
	
	gtk_box_pack_start(GTK_BOX(editBox), browseButton, TRUE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(outer), editBox, TRUE, FALSE, 0);
	
	gtk_box_pack_start(GTK_BOX(_widget), outer, TRUE, TRUE, 0);
}

// Browse button callback
void TexturePropertyEditor::callbackBrowse(GtkWidget* widget, 
										   TexturePropertyEditor* self) 
{
	// Light texture chooser (self-destructs on close)
	LightTextureChooser chooser;
	std::string texture = chooser.chooseTexture();
	if (!texture.empty()) {
		// Apply the keyvalue immediately
		self->_entity->setKeyValue(self->_key, texture);
	}
}

} // namespace ui
