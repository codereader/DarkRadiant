#ifndef BLOCKINGTRANSIENTDIALOG_H_
#define BLOCKINGTRANSIENTDIALOG_H_

#include "TransientWindow.h"

#include <gtk/gtkmain.h>

namespace gtkutil
{

/**
 * A blocking version of TransientWindow. This window will enter a recursive
 * gtk_main() loop when shown, which will be terminated once it is destroyed.
 * A function which creates and displays a BlockingTransientWindow will not
 * continue executing until the user has closed the dialog.
 */
class BlockingTransientWindow 
: public TransientWindow
{
	// Is window shown? If so, we need to exit the main loop when destroyed
	bool _isShown;
	
private:
	
	// Called after the dialog is shown. Enter the recursive main loop.
	virtual void _postShow() {
		_isShown = true;
		gtk_main();
	}
	
	// Called after the dialog is destroyed. Exit the main loop if required.
	virtual void _postDestroy() {
		if (_isShown) {
			gtk_main_quit();
		}
	}
	
public:
	
	/**
	 * Construct a BlockingTransientDialog with the given title and parent.
	 */
	BlockingTransientWindow(const std::string& title, GtkWindow* parent)
	: TransientWindow(title, parent),
	  _isShown(false)
	{ }
	
};

}

#endif /*BLOCKINGTRANSIENTDIALOG_H_*/
