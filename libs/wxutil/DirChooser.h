#pragma once

#include "ifilechooser.h"
#include <wx/wxprec.h>
#include <wx/dirdlg.h>

class wxWindow;

namespace wxutil
{

class DirChooser :
	public ui::IDirChooser
{
private:
	wxDirDialog* _dialog;

	// Window title
	std::string _title;

public:
	DirChooser(wxWindow* parent, const std::string& title);

	virtual ~DirChooser();

	// Lets the dialog start at a certain path
	virtual void setCurrentPath(const std::string& path);

	/**
	 * Returns the selected folder name.
	 */
	virtual std::string getSelectedFolderName();

	/**
	 * greebo: Displays the dialog and enters the main loop.
	 * Returns the filename or "" if the user hit cancel.
	 *
	 * The returned file name is normalised using the os::standardPath() method.
	 */
	virtual std::string display();
};

} // namespace
