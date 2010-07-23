#include "PersistentTransientWindow.h"

namespace gtkutil {

PersistentTransientWindow::PersistentTransientWindow(const std::string& title, 
								 					 GtkWindow* parent,
								 					 bool hideOnDelete) 
: TransientWindow(title, parent, hideOnDelete),
  _parent(parent != NULL ? static_cast<Gtk::Window*>(Glib::wrap(parent)) : NULL)
{
	// Connect to the window-state-event signal of the parent window
	_windowStateConn = _parent->signal_window_state_event().connect(
		sigc::mem_fun(*this, &PersistentTransientWindow::onParentWindowStateEvent)
	);
}

PersistentTransientWindow::PersistentTransientWindow(const std::string& title, 
													 Gtk::Window* parent,
								 					 bool hideOnDelete) 
: TransientWindow(title, parent, hideOnDelete),
  _parent(parent)
{
	// Connect to the window-state-event signal of the parent window
	_windowStateConn = _parent->signal_window_state_event().connect(
		sigc::mem_fun(*this, &PersistentTransientWindow::onParentWindowStateEvent)
	);
}

bool PersistentTransientWindow::onParentWindowStateEvent(GdkEventWindowState* ev)
{
	// Check, if the event is of interest
	if ((ev->changed_mask & (GDK_WINDOW_STATE_ICONIFIED|GDK_WINDOW_STATE_WITHDRAWN)) != 0)
	{
		// Now let's see what the new state of the main window is
		if ((ev->new_window_state & (GDK_WINDOW_STATE_ICONIFIED|GDK_WINDOW_STATE_WITHDRAWN)) != 0)
		{
			// The parent got minimised, minimise the child as well
			minimise();
		}
		else
		{
			// Restore the child as the parent is now visible again
			restore();
		}
	}

	return false;
}

// Activate parent if necessary
void PersistentTransientWindow::activateParent()
{
	// Only activate if this window is active already
	if (gobj()->is_active)
	{
		Gtk::Container* toplevel = get_toplevel(); 
		
		if (toplevel != NULL && toplevel->is_toplevel() && 
			toplevel->is_visible() && dynamic_cast<Gtk::Window*>(toplevel) != NULL)
		{
			static_cast<Gtk::Window*>(toplevel)->present();
		}
	}
}

// Post-hide event from TransientWindow
void PersistentTransientWindow::_postHide()
{
	activateParent();
}

// Virtual pre-destroy callback, called by TransientWindow before the window
// itself has been destroyed
void PersistentTransientWindow::_preDestroy()
{
	// If this window is active, make the parent active instead
	activateParent();

	_windowStateConn.disconnect();
}

void PersistentTransientWindow::restore()
{
	if (reinterpret_cast<intptr_t>(get_data("was_mapped")) != 0)
	{
		intptr_t x = reinterpret_cast<intptr_t>(get_data("old_x_position"));
		intptr_t y = reinterpret_cast<intptr_t>(get_data("old_y_position"));

		// Be sure to un-flag the window as "mapped", otherwise it will be restored again
		set_data("was_mapped", NULL);

		move(x, y);

		// Set the window to visible, as it was visible before minimising the parent
		Gtk::Widget::show();

		// Workaround for some linux window managers resetting window positions after show()
		move(x, y);
	}
}

void PersistentTransientWindow::minimise()
{
	if (is_visible())
	{
		// Set the "mapped" flag to mark this window as "to be restored again"
		set_data("was_mapped", reinterpret_cast<void*>(1));

		// Store the position into the child window
		int x, y;
		get_position(x, y);
		set_data("old_x_position", reinterpret_cast<void*>(x));
		set_data("old_y_position", reinterpret_cast<void*>(y));

		Gtk::Widget::hide();
	}
}

} // namespace gtkutil
