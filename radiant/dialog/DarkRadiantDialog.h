#ifndef DARKRADIANTDIALOG_H_
#define DARKRADIANTDIALOG_H_

#include <gtk/gtk.h>

namespace darkradiant
{

// Container object for DarkRadiant GTK dialogs. This is an abstract class which
// all DarkRadiant dialogs will inherit.

class DarkRadiantDialog
{
	GtkWindow *theWindow;
public:
	DarkRadiantDialog();
	virtual ~DarkRadiantDialog();

	// Each DarkRadiantDialog will have functions to create and destroy themselves.
	// All other functions will be added to the derived classes.
	void display() { doDisplay(); }
	void destroy() { doDestroy(); }

	// Virtual function responsible for creating and displaying this dialog
	virtual void doDisplay() = 0;
	
	// Virtual function responsible for destroying and cleaning up the dialog
	virtual void doDestroy() = 0;
};

}

#endif /*DARKRADIANTDIALOG_H_*/
