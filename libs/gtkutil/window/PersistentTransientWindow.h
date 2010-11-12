#ifndef TRANSIENTWINDOW_
#define TRANSIENTWINDOW_

#include "TransientWindow.h"

#include <string>
#include <stdexcept>

#include <boost/shared_ptr.hpp>

typedef struct _GdkEventWindowState GdkEventWindowState;

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
private:
	Glib::RefPtr<Gtk::Window> _parent;
	sigc::connection _windowStateConn;

private:
	// Signal callback
	bool onParentWindowStateEvent(GdkEventWindowState* ev);

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
	void restore();

	// Minimise a child window, storing its position as GObject associated data in
	// order to allow it to be restored correctly.
	void minimise();

public:
	/**
	 * Construct a PersistentTransientWindow with the given title and parent.
	 */
	PersistentTransientWindow(const std::string& title,
							  const Glib::RefPtr<Gtk::Window>& parent,
							  bool hideOnDelete = false);

	// Override TransientWindow::setParentWindow()
	virtual void setParentWindow(const Glib::RefPtr<Gtk::Window>& parent);

}; // class PersistentTransientWindow

/**
 * Shared pointer typedef.
 */
typedef boost::shared_ptr<PersistentTransientWindow>
PersistentTransientWindowPtr;

} // namespace gtkutil

#endif /*TRANSIENTWINDOW_*/
