#ifndef ABOUTDIALOG_H_
#define ABOUTDIALOG_H_

#include <string>
#include "icommandsystem.h"
#include "gtk/gtkwidget.h"
#include "gtkutil/window/BlockingTransientWindow.h"

/** greebo: The AboutDialog displays information about DarkRadiant and
 * 			the subsystems OpenGL and GTK+ 
 *  
 * Note: Show the dialog by instantiating this class with NEW on the heap, 
 * as it's deriving from gtkutil::DialogWindow. It destroys itself upon dialog closure 
 * and frees the allocated memory. 
 */
namespace ui {

class AboutDialog :
	public gtkutil::BlockingTransientWindow
{
	// The treeview containing the above liststore
	GtkWidget* _treeView;

public:
	// Constructor
	AboutDialog();
	
	// This is called to initialise the dialog window / create the widgets
	virtual void populateWindow();
	
	/** greebo: Shows the dialog (allocates on heap, dialog self-destructs)
	 */
	static void showDialog(const cmd::ArgumentList& args);
	
private:
	// The callback for the buttons
	static void callbackClose(GtkWidget* widget, AboutDialog* self);
}; // class CommandListDialog

} // namespace ui

#endif /*ABOUTDIALOG_H_*/
