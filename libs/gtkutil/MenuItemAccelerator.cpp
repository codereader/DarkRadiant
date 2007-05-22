#include "MenuItemAccelerator.h"

#include <gtk/gtkcheckmenuitem.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtklabel.h>
#include <gtk/gtkimage.h>

#include "image.h"

namespace gtkutil
{

TextMenuItemAccelerator::TextMenuItemAccelerator(const std::string& label, 
	const std::string& accelLabel,
	GdkPixbuf* icon,
	bool isToggle) 
:	_label(label),
	_accelLabel(accelLabel),
	_icon(icon),
	_isToggle(isToggle)
{}

// Operator cast to GtkWidget* for packing into a menu
TextMenuItemAccelerator::operator GtkWidget* () {
	
	// Create the menu item, with or without a toggle
	GtkWidget* menuItem;
	if (_isToggle)
		menuItem = gtk_check_menu_item_new();
	else
		menuItem = gtk_menu_item_new();
	
	// Create the text. This consists of the icon, the label string (left-
	// aligned) and the accelerator string (right-aligned).
	GtkWidget* hbx = gtk_hbox_new(FALSE, 4);
	
	// Try to pack in icon ONLY if it is valid
	if (_icon != NULL) {
		gtk_box_pack_start(GTK_BOX(hbx), 
			gtk_image_new_from_pixbuf(_icon),
			FALSE, FALSE, 0);
	}
	
	gtk_box_pack_start(GTK_BOX(hbx),
					   gtk_label_new_with_mnemonic(_label.c_str()),
					   FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbx), gtk_label_new(" "), FALSE, FALSE, 12);
	gtk_box_pack_end(GTK_BOX(hbx),
					 gtk_label_new_with_mnemonic(_accelLabel.c_str()),
					 FALSE, FALSE, 0);
	
	// Pack the label structure into the MenuItem
	gtk_container_add(GTK_CONTAINER(menuItem), hbx);
	
	return menuItem;
}

} // namespace gtkutil
