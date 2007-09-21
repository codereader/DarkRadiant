#ifndef TRANSIENTDIALOG_H_
#define TRANSIENTDIALOG_H_

#include <gtk/gtkwindow.h>

#include <string>

namespace gtkutil
{

/**
 * A basic GtkWindow that is transient for the given parent window.
 */
class TransientWindow
{
	// The main window
	GtkWidget* _window;
	
	// Whether this window should be hidden rather than destroyed
	bool _hideOnDelete;
	
private:
	
	/* Customisable virtuals implemented by subclasses */
	
	virtual void _preShow() { }
	virtual void _postShow() { }

	virtual void _preHide() { }
	virtual void _postHide() { }
	
	virtual void _preDestroy() { }
	virtual void _postDestroy() { }
	
	// GTK delete callback
	static gboolean _onDelete(GtkWidget* w, GdkEvent* e, TransientWindow* self) 
	{
		if (self->_hideOnDelete)
			self->hide();
		else
			self->destroy();
		return TRUE;
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
					GtkWindow* parent, 
					bool hideOnDelete = false)
	: _window(gtk_window_new(GTK_WINDOW_TOPLEVEL)),
	  _hideOnDelete(hideOnDelete)
	{
		// Set up the window
		gtk_window_set_title(GTK_WINDOW(_window), title.c_str());
		gtk_window_set_transient_for(GTK_WINDOW(_window), parent);
	    gtk_window_set_position(
	    	GTK_WINDOW(_window), GTK_WIN_POS_CENTER_ON_PARENT
	    );
	    gtk_window_set_type_hint(
	    	GTK_WINDOW(_window), GDK_WINDOW_TYPE_HINT_DIALOG
	    );
	    
	    // Connect up the destroy signal (close box)
	    g_signal_connect(
	    	G_OBJECT(_window), "delete-event", G_CALLBACK(_onDelete), this
	    );
	}
	
	/**
	 * Destructor. Invokes the destroy() event if necessary.
	 */
	~TransientWindow() {
		if (GTK_IS_WIDGET(_window))
			destroy();
	}
	
	/**
	 * Return the underlying GtkWindow.
	 */
	GtkWidget* getWindow() { 
		return _window; 
	}
	
	/**
	 * Show the dialog. If the window is already visible, this has no effect.
	 */
	void show() {
		if (!isVisible()) {
			_preShow();
			gtk_widget_show_all(_window);
			_postShow();
		}
	}

	/**
	 * Hide the window.
	 */
	void hide() {
		_preHide();
		gtk_widget_hide(_window);
		_postHide();
	}
	
	/**
	 * Test for visibility.
	 */
	bool isVisible() {
		if (GTK_WIDGET_VISIBLE(_window))
			return true;
		else
			return false;
	}
	
	/**
	 * Destroy the window. If the window is currently visible, the hide()
	 * operation will be automatically performed first.
	 */
	void destroy() {
		// Trigger a hide sequence if necessary
		if (isVisible())
			hide();
		
		// Invoke destroy callbacks and destroy the Gtk widget
		_preDestroy();
		gtk_widget_destroy(_window);
		_postDestroy();
	}
};

}

#endif /*TRANSIENTDIALOG_H_*/
