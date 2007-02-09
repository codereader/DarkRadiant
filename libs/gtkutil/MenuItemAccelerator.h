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
							bool isToggle);
	
	// Operator cast to GtkWidget* for packing into a menu
	virtual operator GtkWidget* ();
};

}

#endif /*MENUITEMACCELERATOR_H_*/
