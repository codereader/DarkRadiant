#include "SkinPropertyEditor.h"
#include "SkinChooser.h"
#include "PropertyEditorFactory.h"

#include <gtk/gtk.h>
#include "ientity.h"

namespace ui
{

// Main constructor
SkinPropertyEditor::SkinPropertyEditor(Entity* entity,
									   const std::string& name,
									   const std::string& options)
: _entity(entity),
  _key(name)
{
	_widget = gtk_vbox_new(FALSE, 6);
	
	// Horizontal box contains keyname, text entry and browse button
	GtkWidget* hbx = gtk_hbox_new(FALSE, 3);
	gtk_container_set_border_width(GTK_CONTAINER(hbx), 3);
	
	// Create the browse button
	GtkWidget* browseButton = gtk_button_new_with_label("Choose skin...");
	gtk_button_set_image(
		GTK_BUTTON(browseButton),
		gtk_image_new_from_pixbuf(
			PropertyEditorFactory::getPixbufFor("skin")
		)
	);
	g_signal_connect(
		G_OBJECT(browseButton), "clicked", G_CALLBACK(_onBrowseButton), this
	);
	
	gtk_box_pack_start(GTK_BOX(hbx), browseButton, TRUE, FALSE, 0);
	
	// Pack hbox into vbox (to limit vertical size), then edit frame
	GtkWidget* vbx = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbx), hbx, TRUE, FALSE, 0);
	
	gtk_box_pack_start(GTK_BOX(_widget), vbx, TRUE, TRUE, 0);
	
}

/* GTK CALLBACKS */

void SkinPropertyEditor::_onBrowseButton(GtkWidget* w, 
										 SkinPropertyEditor* self)
{
	// Display the SkinChooser to get a skin from the user
	std::string modelName = self->_entity->getKeyValue("model");
	std::string prevSkin = self->_entity->getKeyValue(self->_key);
	std::string skin = SkinChooser::chooseSkin(modelName, prevSkin);
	
	// Apply the key to the entity
	self->_entity->setKeyValue(self->_key, skin);
}

}
