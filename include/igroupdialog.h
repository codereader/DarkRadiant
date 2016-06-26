#pragma once

#include <string>
#include <memory>

class wxWindow;
class wxFrame;

/**
 * greebo: This defines the interface for accessing the GroupDialog
 *         (i.e. the window housing the Entity Inspector, the Media Browser et al).
 *
 * Use the addPage() method to add a new tab to the GroupDialog.
 */
class IGroupDialog
{
public:
    /**
	 * Destructor
	 */
	virtual ~IGroupDialog() {}

	struct Page
	{
		// The name of this window (unique, can be used to show the page)
		std::string name;

		// The label string to be displayed on the tab
		std::string tabLabel;

		// The image to be displayed in the tab
		std::string tabIcon;

		// the actual widget to be added
		wxWindow* page;

		// the title string for the groupdialog window
		// to be displayed when this tab is active
		std::string windowLabel;

		// Define the order of the "native" group dialog pages
		// Use this enum values to indicate which tab position 
		// you need your page to have sorted at
		struct Position
		{
			enum PredefinedValues
			{
				EntityInspector = 100,
				MediaBrowser = 200,
				Console = 300,
				TextureBrowser = 400,
				End = 5000
			};
		};

		// Defines the position page in the group dialog (defaults to "End")
		// See the predefined Position enum for already existing positions
		int position = Position::End;
	};
	typedef std::shared_ptr<Page> PagePtr;

	/**
	 * Adds a page to the group dialog.
	 * @returns: the notebook page widget
	 */
	virtual wxWindow* addPage(const PagePtr& page) = 0;

	/**
	 * Removes the named page from the TextureBrowser. If the page doesn't exist,
	 * nothing happens.
	 */
	virtual void removePage(const std::string& name) = 0;

	/** greebo: Sets the active tab to the given widget.
	 *
	 * @page: The widget that should be displayed, must have been added
	 * 		  using addPage() beforehand.
	 */
	virtual void setPage(wxWindow* page) = 0;

	/** greebo: Activated the named page. The <name> parameter
	 * 			refers to the name string passed to the addPage() method.
	 * This also shows the GroupDialog, should it be hidden before the call.
	 */
	virtual void setPage(const std::string& name) = 0;

	/** greebo: Toggle the named page. The <name> parameter
	 * refers to the name string passed to the addPage() method.
	 *
	 * The behaviour is as follows: Calling this command opens the
	 * GroupDialog (even if it is hidden) and switches to the given page.
	 *
	 * If the GroupDialog is already visible and focusing the requested page,
	 * the dialog is hidden.
	 */
	virtual void togglePage(const std::string& name) = 0;

	/** greebo: Returns the widget of the currently visible page.
	 */
	virtual wxWindow* getPage() = 0;

	/**
	 * greebo: Returns the name of the current groupdialog page or "" if none is set.
	 */
	virtual std::string getPageName() = 0;

	// Returns the window widget containing the GroupDialog.
	virtual wxFrame* getDialogWindow() = 0;

	// Shows the dialog
	virtual void showDialogWindow() = 0;

	// Hides the dialog
	virtual void hideDialogWindow() = 0;

	/**
	 * Special function for mainframe layouts. This method allows to detach the
	 * contained notebook from the group dialog to pack it somewhere else.
	 * Layout code shouldn't forget to reparent it to the groupdialog again
	 * on deactivation.
	 */
	virtual void reparentNotebook(wxWindow* newParent) = 0;

	/**
	 * Reparents the groupdialog notebook back to the GroupDialog itself.
	 */
	virtual void reparentNotebookToSelf() = 0;
};
