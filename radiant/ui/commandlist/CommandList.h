#ifndef COMMANDLIST_H_
#define COMMANDLIST_H_

#include <string>
#include "icommandsystem.h"
#include <gtkmm/liststore.h>
#include "gtkutil/window/BlockingTransientWindow.h"

namespace Gtk
{
	class TreeView;
}

/* greebo: The CommandListDialog class displays a list of all available
 * DarkRadiant commands and provides methods to clear and assign the shortcuts.
 *
 * The actual re-assignment is taken care of by the ShortcutChooser helper class.
 *
 * Note: Show the dialog by instantiating this class. It blocks the GUI and
 * destroys itself upon dialog closure and returns control to the calling function.
 */
namespace ui
{

class CommandList :
	public gtkutil::BlockingTransientWindow
{
public:
	struct Columns :
		public Gtk::TreeModel::ColumnRecord
	{
		Columns() { add(command); add(key); }

		Gtk::TreeModelColumn<Glib::ustring> command;
		Gtk::TreeModelColumn<Glib::ustring> key;
	};

private:
	Columns _columns;

	// The list store containing the list of ColourSchemes
	Glib::RefPtr<Gtk::ListStore> _listStore;

	// The treeview containing the above liststore
	Gtk::TreeView* _treeView;

public:
	// Constructor
	CommandList();

	/** greebo: Shows the dialog (allocates on heap, dialog self-destructs)
	 */
	static void showDialog(const cmd::ArgumentList& args);

private:
	// This is called to initialise the dialog window / create the widgets
	void populateWindow();

	// Handles the assignment of a new shortcut to the selected row
	void assignShortcut();

	// Removes all items from the treeview and reloads the list
	void reloadList();

	// Gets the currently selected event name
	std::string getSelectedCommand();

	// The gtkmm callback for the buttons
	void callbackClose();
	void callbackClear();
	void callbackAssign();

	// The callback to catch the double click on a treeview row
	void callbackViewButtonPress(GdkEventButton* ev);

}; // class CommandListDialog

} // namespace ui

#endif /*COMMANDLIST_H_*/
