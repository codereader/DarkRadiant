#include "MenuItemAccelerator.h"

#include <gtk/gtkcheckmenuitem.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtklabel.h>
#include <gtk/gtkimage.h>

namespace gtkutil
{

TextMenuItemAccelerator::TextMenuItemAccelerator(const std::string& label, 
	const std::string& accelLabel,
	GdkPixbuf* icon,
	bool isToggle) 
:	_labelText(label),
	_label(NULL),
	_accelLabelText(accelLabel),
	_accel(NULL),
	_icon(icon),
	_iconImage(NULL),
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
		_iconImage = gtk_image_new_from_pixbuf(_icon);
		gtk_box_pack_start(GTK_BOX(hbx), _iconImage, FALSE, FALSE, 0);
	}
	
	_label = gtk_label_new_with_mnemonic(_labelText.c_str());
	_accel = gtk_label_new_with_mnemonic(_accelLabelText.c_str());
	
	gtk_box_pack_start(GTK_BOX(hbx), _label, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbx), gtk_label_new(" "), FALSE, FALSE, 12);
	gtk_box_pack_end(GTK_BOX(hbx), _accel, FALSE, FALSE, 0);
	
	// Pack the label structure into the MenuItem
	gtk_container_add(GTK_CONTAINER(menuItem), hbx);
	
	return menuItem;
}

void TextMenuItemAccelerator::setLabel(const std::string& newLabel) {
	if (_label != NULL) {
		_labelText = newLabel;
		gtk_label_set_markup_with_mnemonic(GTK_LABEL(_label), newLabel.c_str());
	}
}
	
void TextMenuItemAccelerator::setAccelerator(const std::string& newAccel) {
	if (_accel != NULL) {
		_accelLabelText = newAccel;
		gtk_label_set_markup_with_mnemonic(GTK_LABEL(_accel), newAccel.c_str());
	}
}

void TextMenuItemAccelerator::setIcon(GdkPixbuf* icon) {
	if (icon != NULL && _iconImage != NULL) {
		_icon = icon;
		gtk_image_set_from_pixbuf(GTK_IMAGE(_iconImage), _icon);		
	}
}

void TextMenuItemAccelerator::setIsToggle(bool isToggle) {
	_isToggle = isToggle;
}

} // namespace gtkutil
