#ifndef COMMANDLIST_H_
#define COMMANDLIST_H_

#include <string>
#include "gtk/gtkwidget.h"
#include "gtk/gtkliststore.h"
#include "gtkutil/window/BlockingTransientWindow.h"

/* greebo: The CommandListDialog class displays a list of all available
 * DarkRadiant commands and provides methods to clear and assign the shortcuts.
 * 
 * The actual re-assignment is taken care of by the ShortcutChooser helper class.
 * 
 * Note: Show the dialog by instantiating this class. It blocks the GUI and
 * destroys itself upon dialog closure and returns control to the calling function.  
 */
namespace ui {

class CommandList :
	public gtkutil::BlockingTransientWindow
{
	// The list store containing the list of ColourSchemes		
	GtkListStore* _listStore;
	
	// The treeview containing the above liststore
	GtkWidget* _treeView;

public:
	// Constructor
	CommandList();
	
	// This is called to initialise the dialog window / create the widgets
	virtual void populateWindow();
	
	/** greebo: Shows the dialog (allocates on heap, dialog self-destructs)
	 */
	static void showDialog();
	
private:
	// Handles the assignment of a new shortcut to the selected row
	void assignShortcut();
	
	// Removes all items from the treeview and reloads the list
	void reloadList();
	
	// Gets the currently selected event name
	std::string getSelectedCommand(); 
	
	// The callback for the buttons
	static void callbackClose(GtkWidget* widget, CommandList* self);
	static void callbackClear(GtkWidget* widget, CommandList* self);
	static void callbackAssign(GtkWidget* widget, CommandList* self);
	
	// The callback to catch the double click on a treeview row
	static gboolean callbackViewButtonPress(GtkWidget* widget, GdkEventButton* event, CommandList* self);
	
}; // class CommandListDialog

} // namespace ui

#endif /*COMMANDLIST_H_*/
