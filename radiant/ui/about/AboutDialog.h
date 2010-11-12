#ifndef ABOUTDIALOG_H_
#define ABOUTDIALOG_H_

#include <string>
#include "icommandsystem.h"
#include "gtkutil/window/BlockingTransientWindow.h"

/**
 * greebo: The AboutDialog displays information about DarkRadiant and
 * the subsystems OpenGL and GTK+.
 */
namespace ui
{

class AboutDialog :
	public gtkutil::BlockingTransientWindow
{
private:
	// Private constructor
	AboutDialog();

public:
	/** greebo: Shows the dialog (blocks, dialog self-destruct on close)
	 */
	static void showDialog(const cmd::ArgumentList& args);

private:
	// The callback for the close button
	void callbackClose();

	// This is called to initialise the dialog window / create the widgets
	void populateWindow();
}; // class AboutDialog

} // namespace ui

#endif /*ABOUTDIALOG_H_*/
