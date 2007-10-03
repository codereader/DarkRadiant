#ifndef MENUITEMACCELERATOR_H_
#define MENUITEMACCELERATOR_H_

#include <string>
#include "gtk/gtkwidget.h"

namespace gtkutil {

/* greebo: Encapsulation for a menu item with a right-aligned accelerator label 
 */
class TextMenuItemAccelerator
{
	// Label to display
	std::string _labelText;
	// The corresponding widget
	GtkWidget* _label;
	
	// Label of the accelerator
	std::string _accelLabelText;
	// The corresponding widget
	GtkWidget* _accel;
	
	// The icon pixbuf
	GdkPixbuf* _icon;
	// The corresponding image widget
	GtkWidget* _iconImage;
	
	// Flag to indicate this is a toggle menu item
	bool _isToggle;

public:

	/**
	 * Construct a menu item with the given label, accelerator and icon. The
	 * icon may be the empty string if no icon is required.
	 */
	TextMenuItemAccelerator(const std::string& label, 
							const std::string& accelLabel,
							GdkPixbuf* icon,
							bool isToggle);
	
	// Operator cast to GtkWidget* for packing into a menu
	virtual operator GtkWidget* ();
	
	// Changes teh label text of the given menu item 
	// (This works AFTER the class has been cast onto a GtkWidget*)
	void setLabel(const std::string& newLabel);
	
	// Changes the accelerator text of this menutem
	// (This works AFTER the class has been cast onto a GtkWidget*)
	void setAccelerator(const std::string& newAccel);
	
	// Change the icon
	// (This works AFTER the class has been cast onto a GtkWidget*)
	void setIcon(GdkPixbuf* icon);
	
	// Change the icon
	// (This works BEFORE the class has been cast onto a GtkWidget*)
	void setIsToggle(bool isToggle);
};

}

#endif /*MENUITEMACCELERATOR_H_*/
