#ifndef BLOCKINGTRANSIENTDIALOG_H_
#define BLOCKINGTRANSIENTDIALOG_H_

#include "TransientWindow.h"

#include <gtkmm/main.h>

namespace gtkutil
{

/**
 * A blocking version of TransientWindow. This window will enter a recursive
 * main loop when shown, which will be terminated once it is destroyed.
 * A function which creates and displays a BlockingTransientWindow will not
 * continue executing until the user has closed the dialog.
 */
class BlockingTransientWindow
: public TransientWindow
{
private:
	// Is window shown? If so, we need to exit the main loop when destroyed
	bool _isShown;

protected:

	// Called after the dialog is shown. Enter the recursive main loop.
	virtual void _postShow()
	{
		_isShown = true;
		Gtk::Main::run();
	}

	// Called after the dialog is destroyed. Exit the main loop if required.
	virtual void _postDestroy()
	{
		if (_isShown)
		{
			Gtk::Main::quit();
			_isShown = false;
		}
	}

	// greebo: When hidden, this dialog should break the gtk_main loop too
	virtual void _postHide()
	{
		if (_isShown)
		{
			Gtk::Main::quit();
			_isShown = false;
		}
	}

public:

	/**
	 * Construct a BlockingTransientDialog with the given title and parent.
	 */
	BlockingTransientWindow(const std::string& title, const Glib::RefPtr<Gtk::Window>& parent)
	: TransientWindow(title, parent),
	  _isShown(false)
	{
		set_modal(true);
	}

	virtual ~BlockingTransientWindow() {}

};

}

#endif /*BLOCKINGTRANSIENTDIALOG_H_*/
