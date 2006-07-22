#ifndef ORTHOCONTEXTMENU_H_
#define ORTHOCONTEXTMENU_H_

#include <gtk/gtk.h>

namespace ui
{

/** Displays a menu when the mouse is right-clicked in the ortho window.
 * This is a singleton class which remains in existence once constructed,
 * and is hidden and displayed as appropriate.
 */

class OrthoContextMenu
{
private:
	
	// The GtkWidget representing the menu
	GtkWidget* _widget;
	
public:

	/** Constructor. Create the GTK content here.
	 */

	OrthoContextMenu();

	/** Display the menu at the current mouse position, and act on the
	 * choice.
	 */
	 
	void show();
	
	/* Static instance display function. Obtain the singleton instance and
	 * call its show() function.
	 */
	 
	static void displayInstance();

};

}

#endif /*ORTHOCONTEXTMENU_H_*/
