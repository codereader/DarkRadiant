#ifndef TRANSIENTDIALOG_H_
#define TRANSIENTDIALOG_H_

#include <gtk/gtkwindow.h>

namespace gtkutil
{

/**
 * A transient dialog window without the complexity of Radiant's persistent
 * transient windows.
 */
class TransientDialog
{
	// The main window
	GtkWidget* _window;
	
private:
	
	// Post-show and post-destroy virtuals overridden by subclasses
	virtual void _postShow() { }
	virtual void _postDestroy() { }
	
	// GTK destroy callback
	static gboolean _onDestroy(GtkWidget* w, GdkEvent* e, TransientDialog* self) 
	{
		self->destroy();
		return TRUE;
	}
	
public:
	
	/**
	 * Construct a TransientDialog with the specified title and parent window.
	 */
	TransientDialog(const std::string& title, GtkWindow* parent)
	: _window(gtk_window_new(GTK_WINDOW_TOPLEVEL))
	{
		// Set up the window
		gtk_window_set_title(GTK_WINDOW(_window), title.c_str());
		gtk_window_set_transient_for(GTK_WINDOW(_window), parent);
		gtk_window_set_modal(GTK_WINDOW(_window), TRUE);
	    gtk_window_set_position(
	    	GTK_WINDOW(_window), GTK_WIN_POS_CENTER_ON_PARENT
	    );
	    gtk_window_set_type_hint(
	    	GTK_WINDOW(_window), GDK_WINDOW_TYPE_HINT_DIALOG
	    );
	    
	    // Connect up the destroy signal (close box)
	    g_signal_connect(
	    	G_OBJECT(_window), "delete-event", G_CALLBACK(_onDestroy), this
	    );
	}
	
	/**
	 * Return the underlying GtkWindow.
	 */
	GtkWidget* getWindow() { 
		return _window; 
	}
	
	/**
	 * Show the dialog.
	 */
	void show() {
		gtk_widget_show_all(_window);
		_postShow();
	}
	
	/**
	 * Destroy the dialog.
	 */
	void destroy() {
		gtk_widget_destroy(_window);
		_postDestroy();
	}
};

}

#endif /*TRANSIENTDIALOG_H_*/
