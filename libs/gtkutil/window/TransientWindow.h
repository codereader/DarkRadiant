#ifndef TRANSIENTDIALOG_H_
#define TRANSIENTDIALOG_H_

#include <gtk/gtkwindow.h>
#include <gtkmm/window.h>
#include "gtkutil/pointer.h"

#include <string>
#include <iostream>

namespace gtkutil
{

/**
 * A basic GtkWindow that is transient for the given parent window.
 */
class TransientWindow :
	public Gtk::Window
{
private:

	// Whether this window should be hidden rather than destroyed
	bool _hideOnDelete;
	
protected:
	
	/* Customisable virtuals implemented by subclasses */
	
	virtual void _preShow() { }
	virtual void _postShow() { }

	virtual void _preHide() { }
	virtual void _postHide() { }
	
	virtual void _preDestroy() { }
	virtual void _postDestroy() { }

	virtual void _onDeleteEvent() 
	{
		if (_hideOnDelete)
		{
			hide();
		}
		else
		{
			destroy();
		}
	}

private:
	
	// GTKmm delete callback
	bool _onDelete(GdkEventAny* ev) 
	{
		// Invoke the virtual function
		_onDeleteEvent();
		return true;
	}
	
public:
	
	/**
	 * Construct a TransientWindow with the specified title and parent window.
	 * 
	 * @param title
	 * The displayed title for the window.
	 * 
	 * @param parent
	 * The parent window for which this window should be a transient.
	 * 
	 * @param hideOnDelete
	 * Set to true if the delete-event triggered by the close button should
	 * only hide the window, rather than deleting it. In this case the
	 * _preHide() and _postHide() methods will be triggered, rather than the
	 * _preDestroy() and _postDestroy() equivalents. The default value is false.
	 */
	TransientWindow(const std::string& title, 
					const Glib::RefPtr<Gtk::Window>& parent, 
					bool hideOnDelete = false)
	: Gtk::Window(Gtk::WINDOW_TOPLEVEL),
	  _hideOnDelete(hideOnDelete)
	{
		// Set up the window
		set_title(title);

		// Set transient
		setParentWindow(parent);

		set_position(Gtk::WIN_POS_CENTER_ON_PARENT);

#ifdef POSIX
		set_skip_taskbar_hint(true);
#endif
	    set_skip_pager_hint(true);
	    
	    // Connect up the destroy signal (close box)
		signal_delete_event().connect(sigc::mem_fun(*this, &TransientWindow::_onDelete));
	}

	virtual void setParentWindow(const Glib::RefPtr<Gtk::Window>& parent)
	{
		if (parent)
		{
			Gtk::Container* toplevel = parent->get_toplevel();

			if (toplevel != NULL && toplevel->is_toplevel() &&
				dynamic_cast<Gtk::Window*>(toplevel) != NULL)
			{
				set_transient_for(*static_cast<Gtk::Window*>(toplevel));
			}
		}
	}
	
	virtual ~TransientWindow()
	{
		// Destruction is handled by the Gtk::Window destructor
	}
	
	/**
	 * Return the underlying GtkWindow. 
	 * DEPRECATED, use *this to retrieve the Gtk::Window& reference
	 */
	GtkWidget* getWindow()
	{ 
		return GTK_WIDGET(gobj()); 
	}

	/**
	 * Create a new Glib::RefPtr<> from this class. There is no shared_from_this() equivalent,
	 * but I had to use this hack several times to get a smart pointer from an instance to
	 * pass it as parent window.
	 */
	Glib::RefPtr<Gtk::Window> getRefPtr()
	{
		return Glib::RefPtr<Gtk::Window>(Glib::wrap(gobj(), true)); // copy reference
	}
	
	/**
	 * Show the dialog. If the window is already visible, this has no effect.
	 */
	void show()
	{
		if (!isVisible())
		{
			_preShow();
			show_all();
			_postShow();
		}
	}

	/**
	 * Hide the window.
	 */
	void hide()
	{
		_preHide();

		Window::hide();

		_postHide();
	}
	
	/**
	 * Test for visibility.
	 */
	bool isVisible()
	{
		return is_visible();
	}

	/**
	 * Sets the window title.
	 */
	void setTitle(const std::string& title)
	{
		set_title(title);
	}
	
	/**
	 * Destroy the window. If the window is currently visible, the hide()
	 * operation will be automatically performed first.
	 */
	void destroy()
	{
		// Trigger a hide sequence if necessary
		if (isVisible())
		{
			TransientWindow::hide();
		}
		
		// Invoke destroy callbacks and destroy the Gtk widget
		_preDestroy();

		// No destroy anymore, this is handled by the destructors
		//Gtk::Widget::destroy();

		_postDestroy();
	}

	void toggleFullscreen()
	{
		setFullscreen(!isFullscreen());
	}

	bool isFullscreen()
	{
		intptr_t val = reinterpret_cast<intptr_t>(get_data("dr-fullscreen"));
		
		return val != 0;
	}

	void setFullscreen(bool isFullScreen)
	{
		if (isFullScreen)
		{
			fullscreen();

			// Set the flag to 1
			set_data("dr-fullscreen", reinterpret_cast<void*>(1));
		}
		else
		{
			unfullscreen();

			// Set the flag to 0
			set_data("dr-fullscreen", NULL);
		}
	}
};

}

#endif /*TRANSIENTDIALOG_H_*/
