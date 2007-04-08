#include "SoundPropertyEditor.h"
#include "PropertyEditorFactory.h"
#include "SoundChooser.h"

#include <gtk/gtk.h>

namespace ui
{

// Main constructor
SoundPropertyEditor::SoundPropertyEditor(Entity* entity,
									     const std::string& name,
									     const std::string& options)
: PropertyEditor(entity, name)
{
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
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(getEditWindow()),
										  vbx);
}

// Set the value in the widgets
void SoundPropertyEditor::setValue(const std::string& val) {
	_shader = val;
}

// Return the value in the widgets
const std::string SoundPropertyEditor::getValue() {
	return _shader;
}

/* GTK CALLBACKS */

void SoundPropertyEditor::_onBrowseButton(GtkWidget* w, 
										  SoundPropertyEditor* self)
{
	// Use a SoundChooser dialog to get a selection from the user
	SoundChooser chooser;
	self->_shader = chooser.chooseSound();
}

} // namespace ui
