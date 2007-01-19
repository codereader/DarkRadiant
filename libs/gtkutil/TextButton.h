#ifndef ICONTEXTBUTTON_H_
#define ICONTEXTBUTTON_H_

#include <gtk/gtkbutton.h>

namespace gtkutil
{

/** An encapsulation of a simple Button with a label on it.
 * 
 *  Use the GtkWidget* operator to retrieve the widget (is shown automatically)
 */

class TextButton
{
	std::string _name;
	
public:

	/** Construct an TextButton with the given label text
	 * 
	 * @param name
	 * The text to display on the button.
	 */
	TextButton(const std::string& name) : 
		_name(name)
	{}
	
	/** Operator cast to GtkWidget* returns a button with the label.
	 */
	operator GtkWidget* () {
		GtkWidget* button = gtk_button_new_with_label(_name.c_str());
		
		gtk_widget_show(button);
		
		// Return the button
		return button;
	}
};

}

#endif /*ICONTEXTBUTTON_H_*/
