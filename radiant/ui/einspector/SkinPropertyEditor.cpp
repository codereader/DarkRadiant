#include "SkinPropertyEditor.h"
#include "SkinChooser.h"

#include <gtk/gtkhbox.h>
#include <gtk/gtkvbox.h>
#include <gtk/gtkentry.h>
#include <gtk/gtkscrolledwindow.h>
#include <gtk/gtklabel.h>
#include <gtk/gtkbutton.h>

namespace ui
{

// Main constructor
SkinPropertyEditor::SkinPropertyEditor(Entity* entity,
									   const std::string& name,
									   const std::string& options)
: PropertyEditor(entity, name)
{
	// Horizontal box contains keyname, text entry and browse button
	GtkWidget* hbx = gtk_hbox_new(FALSE, 3);
	gtk_container_set_border_width(GTK_CONTAINER(hbx), 3);
	
	gtk_box_pack_start(GTK_BOX(hbx), 
					   gtk_label_new((name + ": ").c_str()),
					   FALSE, FALSE, 0);
					   
	_textEntry = gtk_entry_new();
	gtk_box_pack_start(GTK_BOX(hbx), _textEntry, TRUE, TRUE, 0);
	
	GtkWidget* browseButton = gtk_button_new_with_label("...");
	g_signal_connect(G_OBJECT(browseButton), 
					 "clicked", 
					 G_CALLBACK(_onBrowseButton), 
					 this);
	gtk_box_pack_start(GTK_BOX(hbx), browseButton, FALSE, FALSE, 0);
	
	// Pack hbox into vbox (to limit vertical size), then edit frame
	GtkWidget* vbx = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbx), hbx, TRUE, FALSE, 0);
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(getEditWindow()),
										  vbx);
	
}

// Set the value in the widgets
void SkinPropertyEditor::setValue(const std::string& val) {
	gtk_entry_set_text(GTK_ENTRY(_textEntry), val.c_str());
}

// Return the value in the widgets
const std::string SkinPropertyEditor::getValue() {
	return gtk_entry_get_text(GTK_ENTRY(_textEntry));
}

/* GTK CALLBACKS */

void SkinPropertyEditor::_onBrowseButton(GtkWidget* w, 
										 SkinPropertyEditor* self)
{
	// Display the SkinChooser to get a skin from the user
	std::string modelName = self->getEntity()->getKeyValue("model");
	std::string prevSkin = gtk_entry_get_text(GTK_ENTRY(self->_textEntry));
	std::string skin = SkinChooser::chooseSkin(modelName, prevSkin);
	
	// Add the skin to the entry box
	gtk_entry_set_text(GTK_ENTRY(self->_textEntry), skin.c_str());
}

}
