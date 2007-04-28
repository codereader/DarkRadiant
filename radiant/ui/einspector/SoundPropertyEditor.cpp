#include "SoundPropertyEditor.h"
#include "PropertyEditorFactory.h"
#include "SoundChooser.h"

#include "ientity.h"

#include <gtk/gtk.h>

namespace ui
{

// Main constructor
SoundPropertyEditor::SoundPropertyEditor(Entity* entity,
									     const std::string& name,
									     const std::string& options)
: _entity(entity),
  _key(name)
{
	_widget = gtk_vbox_new(FALSE, 0);

	// Horizontal box contains the browse button
	GtkWidget* hbx = gtk_hbox_new(FALSE, 3);
	gtk_container_set_border_width(GTK_CONTAINER(hbx), 3);
	
	// Browse button
	GtkWidget* browseButton = gtk_button_new_with_label("Choose sound...");
	gtk_button_set_image(
		GTK_BUTTON(browseButton),
		gtk_image_new_from_pixbuf(
			PropertyEditorFactory::getPixbufFor("sound")
		)
	);
			
	g_signal_connect(G_OBJECT(browseButton), 
					 "clicked", 
					 G_CALLBACK(_onBrowseButton), 
					 this);
	gtk_box_pack_start(GTK_BOX(hbx), browseButton, TRUE, FALSE, 0);
	
	// Pack hbox into vbox (to limit vertical size), then edit frame
	GtkWidget* vbx = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbx), hbx, TRUE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(_widget), vbx, TRUE, TRUE, 0);
}

/* GTK CALLBACKS */

void SoundPropertyEditor::_onBrowseButton(GtkWidget* w, 
										  SoundPropertyEditor* self)
{
	// Use a SoundChooser dialog to get a selection from the user
	SoundChooser chooser;
	std::string selection = chooser.chooseSound(); 
	if (!selection.empty()) {
		// Apply the change to the entity
		self->_entity->setKeyValue(self->_key, selection);
	}
}

} // namespace ui
