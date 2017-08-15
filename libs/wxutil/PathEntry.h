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

	// The filetype determining the available filters in the FileChooser
	std::string _fileType;

	// The extension to use as default (preselects the corresponding filter
	// in the FileChooser dialog
	std::string _defaultExt;

private:
	// Private shared constructor
	PathEntry(wxWindow* parent, bool foldersOnly, const std::string& fileType, const std::string& defaultExt);

public:
	/**
	 * Construct a new Path Entry. Use the boolean
	 * to specify whether this widget should be used to
	 * browser for folders or files.
	 */
	explicit PathEntry(wxWindow* parent, bool foldersOnly);

	/**
	 * Construct a new Path Entry which browses for files
	 * matching the given filetype. The filetype is used to populate
	 * the filter dropdown with the corresponding options registered 
	 * in the FileTypeRegistry.
	 */
	explicit PathEntry(wxWindow* parent, const std::string& fileType);

	// Same constructor as above but accepting const char* to avoid
	// the char* being converted to bool even though it's marked explicit
	explicit PathEntry(wxWindow* parent, const char* fileType);

	/**
	* Construct a new Path Entry which browses for files
	* matching the given filetype. The filetype is used to populate
	* the filter dropdown with the corresponding options registered
	* in the FileTypeRegistry. The dropdown matching the default extension
	* is preselected by default.
	*/
	explicit PathEntry(wxWindow* parent, const std::string& fileType, const std::string& defaultExt);

	// Set the selected path, this does not fire the EV_PATH_ENTRY_CHANGED event
	void setValue(const std::string& val);

    // Get the currently selected path
    std::string getValue() const;

	// Returns the text entry widget
	wxTextCtrl* getEntryWidget();

	// Set the default extension to use in the FileChooser variant
	void setDefaultExtension(const std::string& defaultExt);

private:
	// callbacks
	void onBrowseFiles(wxCommandEvent& ev);
	void onBrowseFolders(wxCommandEvent& ev);
};

wxDECLARE_EVENT(EV_PATH_ENTRY_CHANGED, wxCommandEvent);

} // namespace wxutil
