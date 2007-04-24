#ifndef TRANSIENTWINDOW_
#define TRANSIENTWINDOW_

#include <string>
#include "gtk/gtkwindow.h"
#include "gtk/gtkwidget.h"

namespace gtkutil
{

/** greebo: Encapsulation of a GtkWindow with title that is transient to the given parent
 */

class TransientWindow
{
	
protected:
	// The text label
	const std::string _title;
	
	// The window that this window is transient for
	GtkWindow* _parent;
	
	// The actual transient window
	GtkWidget* _window;
	
	// True, if the onDelete event is connected to a self-destruct method
	bool _deletable;

public:

	// Constructor
	TransientWindow(const std::string& title, GtkWindow* parent, bool deletable = true);
	
	// Operator cast to GtkWindow* (use this to create and retrieve the GtkWidget* pointer)
	virtual operator GtkWidget* ();
	
private:

	/* greebo: This gets called when the _parent window is minimised or otherwise resized. If the
	 * parent is actually minimised, minimise the child as well (and vice versa).
	 * 
	 * Parts of this are taken from original GtkRadiant code (window.cpp)
	 */
	static gboolean onParentResize(GtkWidget* widget, GdkEventWindowState* event, GtkWidget* child);

public:	
	/* greebo: The following two functions are copied over from window.cpp (GtkRadiant original code)
	 */

	// Re-show a child window when the main window is restored. It is necessary to
	// call gtk_window_move() after showing the window, since with some Linux
	// window managers, the window position is lost after gtk_window_show().
	static void restore(GtkWidget* window);
	
	// Minimise a child window, storing its position as GObject associated data in
	// order to allow it to be restored correctly.
	static void minimise(GtkWidget* window);

	/** greebo: Just toggles the visibility of this window and blocks the delete event propagation.
	 */
	static gboolean toggleOnDelete(GtkWidget* widget, GdkEvent* event, GtkWindow* parent);

private:
	/** greebo: This shows the parent again, so that it doesn't disappear behind other applications
	 */
	static gboolean showParentOnDelete(GtkWidget* widget, GdkEvent* event, GtkWindow* parent);

	/* greebo: This disconnects the onResize handler from the _parent window
	 */	
	static gboolean onDelete(GtkWidget* widget, GdkEvent* event, GtkWindow* parent);

}; // class TransientWindow

} // namespace gtkutil

#endif /*TRANSIENTWINDOW_*/
