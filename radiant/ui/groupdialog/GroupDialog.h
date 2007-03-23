#ifndef GROUPDIALOG_H_
#define GROUPDIALOG_H_

#include <string>
#include "gtkutil/WindowPosition.h"

/** greebo: The GroupDialog class creates the Window and the Notebook widget
 * 			as soon as construct() is called. This method takes a parent GtkWindow*
 * 			as argument, to set the "transient for" property. As the GroupDialog
 * 			is constructed at a time when the MainFrame is still "under construction"
 * 			the window can't be created in the GroupDialog constructor.
 * 
 * 			Use the Instance() method to access the static instance of this dialog and 
 * 			the addPage method to add new notebook tabs. 
 * 
 * 			ui::GroupDialog::Instance().addPage()
 * 
 * 			The name passed to the addPage() method can be used to directly toggle 
 * 			the notebook widgets via setPage(<name>).
 * 
 * 			Call the shutdown() method to save the window state to the registry and
 * 			disconnect the dialog from the EventManager before DarkRadiant's exit.
 */
 
// Forward Declarations
typedef struct _GtkWidget GtkWidget;
typedef struct _GtkWindow GtkWindow;

namespace ui {

class GroupDialog 
{
	GtkWidget* _dialog;
	
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

public:
	GroupDialog();
	
	/** greebo: Use this to trigger
	 */
	void construct(GtkWindow* parent);
	
	/** greebo: Shows the instance (if it is invisible), does nothing otherwise
	 */
	void show();
	
	/** greebo: Returns true, if the window widget is not NULL and the
	 * 			window is visible. 
	 */
	bool visible();
	
	/** greebo: Returns the window widget to set child windows "transient" 
	 */
	GtkWindow* getWindow();
	
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
	
	/** greebo: Returns the widget of the currently visible page.
	 */
	GtkWidget* getPage();
	
	/** greebo: Safely disconnects this window from
	 * 			the eventmanager and saves the window position.
	 */
	void shutdown();
	
	/** greebo: The home of the static instance of this dialog
	 */
	static GroupDialog& Instance();
	
	/** greebo: The command target to toggle the visibility
	 */
	static void toggle();

private:
	/** greebo: Gets invoked on toggle() and takes care of the details
	 */
	void toggleWindow();
	
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
