#ifndef MENUITEMACCELERATOR_H_
#define MENUITEMACCELERATOR_H_

#include <gtk/gtkcheckmenuitem.h>
#include <gtk/gtkhbox.h>
#include <gtk/gtklabel.h>
#include <gtk/gtkimage.h>

#include "image.h"

namespace gtkutil {

/* greebo: Encapsulation for a menu item with a right-aligned accelerator label 
 */
class TextMenuItemAccelerator
{
	// Label to display
	std::string _label;
	
	// Label of the accelerator
	std::string _accelLabel;
	
	// Name of the icon image, if set
	std::string _iconName;	
	
	// Flag to indicate this is a toggle menu item
	bool _isToggle;

public:

	/**
	 * Construct a menu item with the given label, accelerator and icon. The
	 * icon may be the empty string if no icon is required.
	 */
	TextMenuItemAccelerator(const std::string& label, 
							const std::string& accelLabel,
							const std::string& iconName,
							bool isToggle) 
	:	_label(label),
		_accelLabel(accelLabel),
		_iconName(iconName),
		_isToggle(isToggle)
	{}
	
	// Operator cast to GtkWidget* for packing into a menu
	virtual operator GtkWidget* () {
		
		// Create the menu item, with or without a toggle
		GtkWidget* menuItem;
		if (_isToggle)
			menuItem = gtk_check_menu_item_new();
		else
			menuItem = gtk_menu_item_new();
		
		// Create the text. This consists of the icon, the label string (left-
		// aligned) and the accelerator string (right-aligned).
		GtkWidget* hbx = gtk_hbox_new(FALSE, 4);
		gtk_box_pack_start(GTK_BOX(hbx), 
			gtk_image_new_from_pixbuf(gtkutil::getLocalPixbuf(_iconName)),
			FALSE, FALSE, 0);
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
};

}

#endif /*MENUITEMACCELERATOR_H_*/
