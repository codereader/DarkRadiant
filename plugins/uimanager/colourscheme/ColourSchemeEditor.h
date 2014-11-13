#pragma once

#include "wxutil/dialog/DialogBase.h"
#include <string>
#include "icommandsystem.h"
#include "ColourScheme.h"

#include "wxutil/TreeModel.h"

class wxButton;
class wxPanel;
class wxDataViewEvent;
class wxColourPickerEvent;
class wxSizer;

namespace wxutil { class TreeView; }

namespace ui
{

class ColourSchemeEditor :
	public wxutil::DialogBase
{
private:
	// The treeview and its selection pointer
	wxutil::TreeView* _treeView;

	struct Columns :
		public wxutil::TreeModel::ColumnRecord
	{
		Columns() : 
			name(add(wxutil::TreeModel::Column::String))
		{}

		wxutil::TreeModel::Column name;
	};

	// The list store containing the list of ColourSchemes
	Columns _columns;
	wxutil::TreeModel::Ptr _listStore;

	// The vbox containing the colour buttons and its frame
	wxPanel* _colourFrame;

	// The "delete scheme" button
	wxButton* _deleteButton;

public:
	// Constructor
	ColourSchemeEditor();

	// Command target
	static void DisplayDialog(const cmd::ArgumentList& args);

	int ShowModal();

private:
	// private helper functions
	void populateTree();
    void createTreeView();
	void constructWindow();
	wxSizer* constructColourSelector(ColourItem& colour, const std::string& name);
	void updateColourSelectors();

	// Queries the user for a string and returns it
	// Returns "" if the user aborts or nothing is entered
	std::string inputDialog(const std::string& title, const std::string& label);

	// Puts the cursor on the currently active scheme
	void selectActiveScheme();

	// Updates the colour selectors after a selection change
	void selectionChanged();

	// Returns the name of the currently selected scheme
	std::string	getSelectedScheme();

	// Deletes or copies a scheme
	void deleteScheme();
	void copyScheme();

	// Deletes a scheme from the list store (called from deleteScheme())
	void deleteSchemeFromList();

	// Callbacks
	void callbackSelChanged(wxDataViewEvent& ev);
	void callbackColorChanged(wxColourPickerEvent& ev, ColourItem& item);
	void callbackDelete(wxCommandEvent& ev);
	void callbackCopy(wxCommandEvent& ev);

	// Destroy window and delete self, called by both Cancel and window
	// delete callbacks
	void doCancel();

	// Updates the windows after a colour change
	static void updateWindows();
};

} // namespace ui
