#pragma once

#include <string>
#include "icommandsystem.h"
#include "wxutil/dialog/DialogBase.h"
#include "wxutil/TreeModel.h"
#include "wxutil/TreeView.h"

class wxButton;

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
	public wxutil::DialogBase
{
public:
	struct Columns :
		public wxutil::TreeModel::ColumnRecord
	{
		Columns() :
			command(add(wxutil::TreeModel::Column::String)),
			key(add(wxutil::TreeModel::Column::String))
		{}

		wxutil::TreeModel::Column command;
		wxutil::TreeModel::Column key;
	};

private:
	Columns _columns;

	// The list store containing the list of ColourSchemes
	wxutil::TreeModel::Ptr _listStore;

	// The treeview containing the above liststore
	wxutil::TreeView* _treeView;

	wxButton* _assignButton;
	wxButton* _clearButton;

public:
	// Constructor
	CommandList();

	/** greebo: Shows the dialog (allocates on heap, dialog self-destructs)
	 */
	static void ShowDialog(const cmd::ArgumentList& args);

private:
	// This is called to initialise the dialog window / create the widgets
	void populateWindow();

	void updateButtonState();

	// Handles the assignment of a new shortcut to the selected row
	void assignShortcut();

	// Removes all items from the treeview and reloads the list
	void reloadList();

	// Gets the currently selected event name
	std::string getSelectedCommand();

	// The callbacks for the buttons
	void onClose(wxCommandEvent& ev);
	void onClear(wxCommandEvent& ev);
	void onAssign(wxCommandEvent& ev);
	void onResetToDefault(wxCommandEvent& ev);

	// The callback to catch the double click on a treeview row
	void onItemDoubleClicked(wxDataViewEvent& ev);
	void onSelectionChanged(wxDataViewEvent& ev);

}; // class CommandListDialog

} // namespace ui
