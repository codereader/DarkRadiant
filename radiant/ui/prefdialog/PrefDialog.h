#pragma once

#include <map>
#include "iradiant.h"
#include "icommandsystem.h"

#include "PrefPage.h"
#include "wxutil/dialog/DialogBase.h"

class wxTreebook;

namespace ui
{

class PrefDialog :
	public wxutil::DialogBase
{
private:
	wxTreebook* _notebook;

	// Each notebook page is created and maintained by a PrefPage class
	// Map the page path to its widget
	typedef std::map<std::string, PrefPage*> PageMap;
	PageMap _pages;

	PrefDialog(wxWindow* parent);

public:
	/** greebo: Runs the modal dialog
	 */
	static void ShowPrefDialog(const cmd::ArgumentList& args);

	/** greebo: Makes sure that the dialog is visible.
	 * 			(does nothing if the dialog is already on screen)
	 */
	static void ShowDialog(const std::string& path = "");

	/** greebo: The command target to show the Game settings preferences.
	 */
	static void ShowProjectSettings(const cmd::ArgumentList& args);

	/** greebo: Displays the page with the specified path.
	 *
	 * @path: a string like "Settings/Patches"
	 */
	void showPage(const std::string& path);

private:
	void showModal(const std::string& requestedPage);

	void createPages();
};

} // namespace ui
