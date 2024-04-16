#pragma once

#include <string>
#include "icommandsystem.h"
#include "wxutil/dialog/DialogBase.h"
#include "wxutil/XmlResourceBasedWidget.h"

/// Core user interface functionality
namespace ui
{

/**
 * greebo: The AboutDialog displays information about DarkRadiant and
 * the subsystems OpenGL and GTK+.
 */
class AboutDialog :
	public wxutil::DialogBase,
	private wxutil::XmlResourceBasedWidget
{
private:
	// Private constructor
	AboutDialog();

public:
	/** greebo: Shows the dialog (blocks, dialog self-destruct on close)
	 */
	static void showDialog(const cmd::ArgumentList& args);

private:
	// The callback for the close button
	void _onClose(wxCommandEvent& ev);
	void _onDeleteEvent(wxCloseEvent& ev);

	// This is called to initialise the dialog window / create the widgets
	void populateWindow();
}; // class AboutDialog

} // namespace ui
