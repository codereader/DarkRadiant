#ifndef TRANSIENTWINDOW_
#define TRANSIENTWINDOW_

#include "TransientWindow.h"

#include "gtk/gtkwindow.h"
#include "gtk/gtkwidget.h"

#include <string>
#include <stdexcept>

#include <boost/shared_ptr.hpp>

namespace gtkutil
{

/**
 * Extension of TransientWindow that implements low-level GTK hackery to create
 * a persistent floating child window, which appears and disappears with the
 * main parent window.
 * 
 * Essentially this is an approximation of MDI, which is not supported directly
 * by GTK. Needless to say, there are a whole load of window management problems
 * associated with this technique.
 */
class PersistentTransientWindow
: public TransientWindow
{
	// Parent window and its resize signal handler
	GtkWindow* _parent;
	gulong _parentResizeHandler;
	
private:

	/* GTK CALLBACKS */
	
	// Parent window has been resized (minimised or maximised)
	static gboolean _onParentResize(GtkWidget* w, 
									GdkEventWindowState* e, 
									PersistentTransientWindow* self);

protected:
	/* TransientWindow events */
	
	// Virtual pre-destroy callback
	virtual void _preDestroy();
	
	// Post-hide event
	virtual void _postHide();
	
private:	
	// Activate the parent window when this window is hidden
	void activateParent();
	
	// Re-show a child window when the main window is restored. It is necessary to
	// call gtk_window_move() after showing the window, since with some Linux
	// window managers, the window position is lost after gtk_window_show().
	static void restore(GtkWidget* window);
	
	// Minimise a child window, storing its position as GObject associated data in
	// order to allow it to be restored correctly.
	static void minimise(GtkWidget* window);

public:
	
	/**
	 * Construct a PersistentTransientWindow with the given title and parent.
	 */
	PersistentTransientWindow(const std::string& title, 
							  GtkWindow* parent,
							  bool hideOnDelete = false);
	
	virtual ~PersistentTransientWindow();
	
	/**
	 * Operator cast to GtkWindow*.
	 */
	operator GtkWidget* () {
		return getWindow();
	}
	
}; // class PersistentTransientWindow

/**
 * Shared pointer typedef.
 */
typedef boost::shared_ptr<PersistentTransientWindow> 
PersistentTransientWindowPtr;

} // namespace gtkutil

#endif /*TRANSIENTWINDOW_*/
