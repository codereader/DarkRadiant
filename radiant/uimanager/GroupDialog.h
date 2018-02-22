#pragma once

#include <memory>
#include <map>
#include "iradiant.h"
#include "igroupdialog.h"
#include "wxutil/window/TransientWindow.h"

#include <wx/windowptr.h>

class wxNotebook;
class wxBookCtrlEvent;
class wxImageList;

/**
 * greebo: The GroupDialog class creates the Window and the Notebook widget
 * as soon as construct() is called.
 *
 * Use the Instance() method to access the static instance of this dialog and
 * the addPage method to add new notebook tabs.
 *
 * ui::GroupDialog::Instance().addPage()
 *
 * The name passed to the addPage() method can be used to directly toggle
 * the notebook widgets via setPage(<name>).
 */

namespace ui
{

class GroupDialog;
typedef std::shared_ptr<GroupDialog> GroupDialogPtr;

class GroupDialog : 
	public wxutil::TransientWindow,
	public IGroupDialog
{
private:
	// Pages, sorted by position
	typedef std::map<int, Page> Pages;

	// The actual list instance
	Pages _pages;

	// The page number of the currently active page widget
	int _currentPage;

	wxWindowPtr<wxNotebook> _notebook;
	std::unique_ptr<wxImageList> _imageList;

private:
	// Private constructor creates GTK widgets etc.
	GroupDialog();

	// TransientWindow events. These deal with window position tracking.
	void _postShow();

public:
	/**
	 * Static method to construct the GroupDialog instance.
	 */
	static void construct();

	// Documentation: see igroupdialog.h
	wxWindow* addPage(const PagePtr& page);

	// Removes a given page
	void removePage(const std::string& name);

	/** greebo: Sets the active tab to the given widget.
	 *
	 * @page: The widget that should be displayed, must have been added
	 * 		  using addPage() beforehand.
	 */
	void setPage(wxWindow* page);

	/** greebo: Activated the named page. The <name> parameter
	 * 			refers to the name string passed to the addPage() method.
	 */
	void setPage(const std::string& name);

	// Toggles page/dialog visibility
	virtual void togglePage(const std::string& name);

	/** greebo: Returns the widget of the currently visible page.
	 */
	wxWindow* getPage();

	/**
	 * greebo: Returns the name of the current groupdialog page or "" if none is set.
	 */
	std::string getPageName();

	// Returns the window widget containing the GroupDialog.
	wxFrame* getDialogWindow();
	void showDialogWindow();
	void hideDialogWindow();

	// Detaches the notebook and relocates it to another parent container
	void reparentNotebook(wxWindow* newParent);
	void reparentNotebookToSelf();

	/**
	 * Retrieve the static GroupDialog instance.
	 */
	static GroupDialog& Instance();

	/** greebo: The command target to toggle the visibility
	 */
	static void toggle();

private:
	/** greebo: Safely disconnects this window from
	* 			the eventmanager and saves the window position.
	*/
	void onRadiantShutdown();

	// Shows the most recently active page
	void onRadiantStartup();

	// This is where the static shared_ptr of the singleton instance is held.
	static GroupDialogPtr& InstancePtr();

	/** greebo: Adds the basic widgets to the groupdialog.
	 */
	void populateWindow();

	/** greebo: Updates the pagetitle and the internal page number
	 */
	void updatePageTitle(int pageNumber);

	// Gets called when the user selects a new tab (updates the title)
	void onPageSwitch(wxBookCtrlEvent& ev);
};

} // namespace ui
