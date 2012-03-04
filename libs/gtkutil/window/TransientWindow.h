#pragma once

#include <gtkmm/window.h>

#include <string>

namespace gtkutil
{

/**
 * A basic GtkWindow that is transient for the given parent window.
 */
class TransientWindow
: public Gtk::Window
{
	// Whether this window should be hidden rather than destroyed
	bool _hideOnDelete;

    // Whether the window is currently fullscreened
    bool _isFullscreen;

protected:

	/* Customisable virtuals implemented by subclasses */

	virtual void _preShow() { }
	virtual void _postShow() { }

	virtual void _preHide() { }
	virtual void _postHide() { }

	virtual void _preDestroy() { }
	virtual void _postDestroy() { }

	virtual void _onDeleteEvent();

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
					bool hideOnDelete = false);

	virtual void setParentWindow(const Glib::RefPtr<Gtk::Window>& parent);

	/**
     * Create a new Glib::RefPtr<> from this class. There is no
     * shared_from_this() equivalent, but I had to use this hack several times
     * to get a smart pointer from an instance to pass it as parent window.
	 */
	Glib::RefPtr<Gtk::Window> getRefPtr();

	/**
	 * Show the dialog. If the window is already visible, this has no effect.
	 */
	void show();

	/**
	 * Hide the window.
	 */
	void hide();

    /// Toggle visibility
    void toggleVisibility();

	/**
	 * Destroy the window. If the window is currently visible, the hide()
	 * operation will be automatically performed first.
	 */
	void destroy();

	void toggleFullscreen();

	bool isFullscreen();

	void setFullscreen(bool isFullScreen);
};

}

