#ifndef SKINCHOOSER_H_
#define SKINCHOOSER_H_

#include <gtk/gtkwidget.h>
#include <string>

namespace ui
{

/** Dialog to allow selection of skins for a model entity. Skins are grouped
 * into two toplevel categories - matching skins which are associated with the
 * model, and all skins available.
 */
class SkinChooser
{
	// Main dialog widget
	GtkWidget* _widget;

private:

	// Constructor creates GTK widgets
	SkinChooser();
	
	// Show the dialog and block until selection is made
	std::string showAndBlock();
	
public:

	/** Display the dialog and return the skin chosen by the user, or an empty
	 * string if no selection was made. This static method maintains a singleton
	 * instance of the dialog, and enters are recursive GTK main loop during
	 * skin selection.
	 */
	static std::string chooseSkin();

};

}

#endif /*SKINCHOOSER_H_*/
