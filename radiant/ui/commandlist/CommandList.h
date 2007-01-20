#ifndef COMMANDLIST_H_
#define COMMANDLIST_H_

#include <string>
#include <iostream>
#include "gtk/gtkwidget.h"
#include "gtk/gtkliststore.h"
#include "gtkutil/DialogWindow.h"

	namespace {
		const int CMDLISTDLG_DEFAULT_SIZE_X = 450;
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
	
	// The label to hold the status text of the shortcut chooser
	GtkWidget* _statusWidget;
	
	// Working variables to store the new key/modifier from the user input
	unsigned int _keyval;
	unsigned int _state;

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
	
	// Asks the user to enter a new shortcut
	void retrieveShortcut(const std::string& commandName);
	
	// Create the actual dialog and return a string with the result (TODO: Move into helper class)
	bool shortcutDialog(const std::string& title, const std::string& label);

	// The callback for the buttons
	static void callbackClose(GtkWidget* widget, CommandListDialog* self);
	static void callbackClear(GtkWidget* widget, CommandListDialog* self);
	static void callbackAssign(GtkWidget* widget, CommandListDialog* self);
	
	// The callback to catch the double click on a treeview row
	static gboolean callbackViewButtonPress(GtkWidget* widget, GdkEventButton* event, CommandListDialog* self);
	
	// The callback for catching the keypress events in the shortcut entry field
	static gboolean onShortcutKeyPress(GtkWidget* widget, GdkEventKey* event, CommandListDialog* self);
	static gboolean onShortcutKeyRelease(GtkWidget* widget, GdkEventKey* event, CommandListDialog* self);
	
}; // class CommandListDialog

} // namespace ui

// -------------------------------------------------------------------------------

// This is the actual command that instantiates the dialog
void ShowCommandListDialog();

#endif /*COMMANDLIST_H_*/
