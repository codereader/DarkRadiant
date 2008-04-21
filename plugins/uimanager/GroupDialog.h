#ifndef GROUPDIALOG_H_
#define GROUPDIALOG_H_

#include "iradiant.h"
#include "igroupdialog.h"
#include "gtkutil/WindowPosition.h"
#include "gtkutil/window/PersistentTransientWindow.h"

/** greebo: The GroupDialog class creates the Window and the Notebook widget
 * 			as soon as construct() is called. 
 * 
 * 			Use the Instance() method to access the static instance of this dialog and 
 * 			the addPage method to add new notebook tabs. 
 * 
 * 			ui::GroupDialog::Instance().addPage()
 * 
 * 			The name passed to the addPage() method can be used to directly toggle 
 * 			the notebook widgets via setPage(<name>).
 */
 
// Forward Declarations
typedef struct _GtkWidget GtkWidget;
typedef struct _GtkWindow GtkWindow;

namespace ui {

class GroupDialog;
typedef boost::shared_ptr<GroupDialog> GroupDialogPtr;

class GroupDialog 
: public gtkutil::PersistentTransientWindow,
	public RadiantEventListener,
	public IGroupDialog
{
	// The window position tracker
	gtkutil::WindowPosition _windowPosition;

	// The structure for each notebook page
	struct Page {
		std::string name;
		GtkWidget* page;
		std::string title;
	};
	typedef std::vector<Page> Pages;
	
	// The actual list instance
	Pages _pages;

	// The tab widget
	GtkWidget* _notebook;
	
	// The page number of the currently active page widget
	int _currentPage;

private:
	// Private constructor creates GTK widgets etc.
	GroupDialog();

	// TransientWindow events. These deal with window position tracking.
	virtual void _preShow();
	virtual void _postShow();
	virtual void _preHide();
	
public:
	/** 
	 * Static method to construct the GroupDialog instance.
	 */
	static void construct();

	/** greebo: Adds a page to the group dialog.
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
	GtkWidget* addPage(const std::string& name, 
					   const std::string& tabLabel, const std::string& tabIcon, 
					   GtkWidget* page, const std::string& windowLabel);
	
	/** greebo: Sets the active tab to the given widget.
	 * 
	 * @page: The widget that should be displayed, must have been added
	 * 		  using addPage() beforehand.
	 */
	void setPage(GtkWidget* page);
	
	/** greebo: Activated the named page. The <name> parameter
	 * 			refers to the name string passed to the addPage() method.
	 */
	void setPage(const std::string& name);

	// Toggles page/dialog visibility
	virtual void togglePage(const std::string& name);
	
	/** greebo: Returns the widget of the currently visible page.
	 */
	GtkWidget* getPage();

	/**
	 * greebo: Returns the name of the current groupdialog page or "" if none is set.
	 */
	std::string getPageName();

	// Returns the window widget containing the GroupDialog.
	GtkWidget* getDialogWindow();
	void showDialogWindow();
	void hideDialogWindow();
	
	/** greebo: Safely disconnects this window from
	 * 			the eventmanager and saves the window position.
	 */
	virtual void onRadiantShutdown();
	
	/** 
	 * Retrieve the static GroupDialog instance.
	 */
	static GroupDialog& Instance();
	
	/** greebo: The command target to toggle the visibility
	 */
	static void toggle();

private:
	// This is where the static shared_ptr of the singleton instance is held.
	static GroupDialogPtr& InstancePtr();

	/** greebo: Adds the basic widgets to the groupdialog.
	 */
	void populateWindow();
	
	/** greebo: Updates the pagetitle and the internal page number
	 */
	void updatePageTitle(unsigned int pageNumber);
	
	// The callback to catch the "delete-event"
	static gboolean onDelete(GtkWidget* widget, GdkEvent* event, GroupDialog* self);
	
	// Gets called when the user selects a new tab (updates the title)
	static gboolean onPageSwitch(GtkWidget* notebook, GtkWidget* page, 
								 guint pageNumber, GroupDialog* self);
};

} // namespace ui

#endif /*GROUPDIALOG_H_*/
