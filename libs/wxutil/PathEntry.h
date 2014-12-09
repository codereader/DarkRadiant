#pragma once

#include <wx/panel.h>

class wxButton;
class wxTextCtrl;

namespace wxutil
{

/**
 * greebo: A PathEntry can be used to edit a path to a file or a directory.
 * It behaves like an ordinary text entry box, featuring a button to the right
 * which opens a FileChooser dialog when clicked.
 */
class PathEntry :
	public wxPanel
{
protected:
	// Browse button
	wxButton* _button;

	// The text entry box
	wxTextCtrl* _entry;

public:
	/**
	 * Construct a new Path Entry. Use the boolean
	 * to specify whether this widget should be used to
	 * browser for folders or files.
	 */
	PathEntry(wxWindow* parent, bool foldersOnly = false);

	// Set the selected path, this does not fire the EV_PATH_ENTRY_CHANGED event
	void setValue(const std::string& val);

    // Get the currently selected path
    std::string getValue() const;

	// Returns the text entry widget
	wxTextCtrl* getEntryWidget();

private:
	// callbacks
	void onBrowseFiles(wxCommandEvent& ev);
	void onBrowseFolders(wxCommandEvent& ev);
};

wxDECLARE_EVENT(EV_PATH_ENTRY_CHANGED, wxCommandEvent);

} // namespace wxutil
