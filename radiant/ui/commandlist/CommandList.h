#ifndef COMMANDLIST_H_
#define COMMANDLIST_H_

#include <string>
#include <iostream>
#include "gtk/gtkwidget.h"
#include "gtk/gtkliststore.h"
#include "gtkutil/DialogWindow.h"

	namespace {
		const int CMDLISTDLG_DEFAULT_SIZE_X = 350;
	    const int CMDLISTDLG_DEFAULT_SIZE_Y = 400;
	    	    
	    const std::string CMDLISTDLG_WINDOW_TITLE = "Shortcut List";
	}

namespace ui {

class CommandListDialog :
	public gtkutil::DialogWindow
{
	// The list store containing the list of ColourSchemes		
	GtkListStore* _listStore;

public:
	// Constructor
	CommandListDialog();
	
	// This is called to initialise the dialog window / create the widgets
	virtual void populateWindow();
	
private:
	// The callback for the GTK delete-event
	static void callbackClose(GtkWidget* widget, CommandListDialog* self);
	
}; // class CommandListDialog

} // namespace ui

// -------------------------------------------------------------------------------

// This is the actual command that instantiates the dialog
void ShowCommandListDialog();

#endif /*COMMANDLIST_H_*/
