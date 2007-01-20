#ifndef COMMANDLIST_H_
#define COMMANDLIST_H_

#include <string>
#include <iostream>
#include "gtk/gtkwidget.h"
#include "gtk/gtkliststore.h"
#include "gtkutil/DialogWindow.h"

/* greebo: The CommandListDialog class displays a list of all available
 * DarkRadiant commands and provides methods to clear and assign the shortcuts.
 * 
 * The actual re-assignment is taken care of by the ShortcutChooser helper class.
 * 
 * Note: Show the dialog by instantiating this class with NEW on the heap, 
 * as it's deriving from gtkutil::DialogWindow. It destroys itself upon dialog closure 
 * and frees the allocated memory. 
 */

	namespace {
		const int CMDLISTDLG_DEFAULT_SIZE_X = 550;
	    const int CMDLISTDLG_DEFAULT_SIZE_Y = 400;
	    	    
	    const std::string CMDLISTDLG_WINDOW_TITLE = "Shortcut List";
	}

namespace ui {

class CommandListDialog :
	public gtkutil::DialogWindow
{
	// The list store containing the list of ColourSchemes		
	GtkListStore* _listStore;
	
	// The treeview containing the above liststore
	GtkWidget* _treeView;

public:
	// Constructor
	CommandListDialog();
	
	// This is called to initialise the dialog window / create the widgets
	virtual void populateWindow();
	
private:
	// Handles the assignment of a new shortcut to the selected row
	void assignShortcut();
	
	// Removes all items from the treeview and reloads the list
	void reloadList();
	
	// Gets the currently selected event name
	std::string getSelectedCommand(); 
	
	// The callback for the buttons
	static void callbackClose(GtkWidget* widget, CommandListDialog* self);
	static void callbackClear(GtkWidget* widget, CommandListDialog* self);
	static void callbackAssign(GtkWidget* widget, CommandListDialog* self);
	
	// The callback to catch the double click on a treeview row
	static gboolean callbackViewButtonPress(GtkWidget* widget, GdkEventButton* event, CommandListDialog* self);
	
}; // class CommandListDialog

} // namespace ui

// -------------------------------------------------------------------------------

// This is the actual command that instantiates the dialog
void ShowCommandListDialog();

#endif /*COMMANDLIST_H_*/
