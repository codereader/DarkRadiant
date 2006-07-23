#ifndef ORTHOCONTEXTMENU_H_
#define ORTHOCONTEXTMENU_H_

#include <gtk/gtk.h>

#include "generic/vector.h"

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
	
	// Last provided 3D point for action
	Vector3 _lastPoint;
	
private:

	/* Gtk Callbacks */
	
	static void callbackAddEntity(GtkMenuItem* item, OrthoContextMenu* self);
	
public:

	/** Constructor. Create the GTK content here.
	 */

	OrthoContextMenu();

	/** Display the menu at the current mouse position, and act on the
	 * choice.
	 * 
	 * @param point
	 * The point in 3D space at which the chosen operation should take
	 * place.
	 */
	 
	void show(const Vector3& point);
	
	/** Static instance display function. Obtain the singleton instance and
	 * call its show() function.
	 * 
	 * @param point
	 * The point in 3D space at which the chosen operation should take
	 * place.
	 */
	 
	static void displayInstance(const Vector3& point);

};

}

#endif /*ORTHOCONTEXTMENU_H_*/
