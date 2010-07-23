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
	
	/** 
	 * greebo: This (hopefully) is to prevent the parent window from staying hidden 
	 * (rarely) when Alt-TABbing from other applications. Often only one 
	 * of the many top-level windows gets shown which is very annoying.
	 * If this doesn't help, this callback can be removed, of course, the 
	 * bug is hard to reproduce.  
	 */ 
	bool _onExpose(GdkEventExpose* ev)
	{
		Gtk::Container* toplevel = get_toplevel(); 
		
		if (toplevel != NULL && toplevel->is_toplevel() && 
			toplevel->is_visible() && dynamic_cast<Gtk::Window*>(toplevel) != NULL)
		{ 
			static_cast<Gtk::Window*>(toplevel)->present();
			
			// Refocus on the self window
			TransientWindow::show();
		}

		return false;
	}

	void construct(const std::string& title, Gtk::Window& parent)
	{
		// Set up the window
		set_title(title);
		set_transient_for(parent);
		set_position(Gtk::WIN_POS_CENTER_ON_PARENT);

#ifdef POSIX
		set_skip_taskbar_hint(true);
#endif
	    set_skip_pager_hint(true);
	    
	    // Connect up the destroy signal (close box)
		signal_delete_event().connect(sigc::mem_fun(*this, &TransientWindow::_onDelete));
		signal_expose_event().connect(sigc::mem_fun(*this, &TransientWindow::_onExpose));	
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
	 *
	 * DEPRECATED: Use the gtkmm-compliant constructor instead.
	 */
	TransientWindow(const std::string& title, 
					GtkWindow* parent, 
					bool hideOnDelete = false)
	: Gtk::Window(Gtk::WINDOW_TOPLEVEL),
	  _hideOnDelete(hideOnDelete)
	{
		construct(title, *static_cast<Gtk::Window*>(Glib::wrap(parent)));
	}

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
					Gtk::Window& parent, 
					bool hideOnDelete = false)
	: Gtk::Window(Gtk::WINDOW_TOPLEVEL),
	  _hideOnDelete(hideOnDelete)
	{
		construct(title, parent);
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
		int val = reinterpret_cast<int>(get_data("dr-fullscreen"));
		
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
