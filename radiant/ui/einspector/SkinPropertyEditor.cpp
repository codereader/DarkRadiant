#include "SkinPropertyEditor.h"

#include <gtk/gtkhbox.h>
#include <gtk/gtkentry.h>
#include <gtk/gtkscrolledwindow.h>
#include <gtk/gtklabel.h>

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
	
	// Pack box into edit frame
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(getEditWindow()),
										  hbx);
	
}

// Set the value in the widgets
void SkinPropertyEditor::setValue(const std::string& val) {
	gtk_entry_set_text(GTK_ENTRY(_textEntry), val.c_str());
}

// Return the value in the widgets
const std::string SkinPropertyEditor::getValue() {
	return gtk_entry_get_text(GTK_ENTRY(_textEntry));
}

}
