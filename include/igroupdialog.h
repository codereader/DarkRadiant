#ifndef INCLUDE_GROUP_DIALOG_H_
#define INCLUDE_GROUP_DIALOG_H_

#include <string>

// Forward declarations
typedef struct _GtkWidget GtkWidget;

/**
 * greebo: This defines the interface for accessing the GroupDialog
 *         (i.e. the window housing the Entity Inspector, the Media Browser et al).
 *
 * Use the addPage() method to add a new tab to the GroupDialog.
 */
class IGroupDialog 
{
public:
	/** Adds a page to the group dialog.
	 * 
	 * @name: The name of this window (unique, can be used to show the page)
	 * @tabLabel: The label string to be displayed on the tab
	 * @tabIcon: The image to be displayed in the tab
	 * @page: the actual page to be added
	 * @windowLabel: the title string for the groupdialog window 
	 * 				 displayed when this tab is active
	 * 
	 * @returns: the notebook page widget
	 */
	virtual GtkWidget* addPage(const std::string& name, 
							   const std::string& tabLabel, const std::string& tabIcon, 
							   GtkWidget* page, const std::string& windowLabel) = 0;

	/** greebo: Sets the active tab to the given widget.
	 * 
	 * @page: The widget that should be displayed, must have been added
	 * 		  using addPage() beforehand.
	 */
	virtual void setPage(GtkWidget* page) = 0;
	
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
	virtual GtkWidget* getPage() = 0;

	/**
	 * greebo: Returns the name of the current groupdialog page or "" if none is set.
	 */
	virtual std::string getPageName() = 0;

	// Returns the window widget containing the GroupDialog.
	virtual GtkWidget* getDialogWindow() = 0;

	// Shows the dialog
	virtual void showDialogWindow() = 0;

	// Hides the dialog
	virtual void hideDialogWindow() = 0;
};

#endif /* INCLUDE_GROUP_DIALOG_H_ */
