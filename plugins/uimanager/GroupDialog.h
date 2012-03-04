#ifndef GROUPDIALOG_H_
#define GROUPDIALOG_H_

#include "iradiant.h"
#include "igroupdialog.h"
#include "gtkutil/WindowPosition.h"
#include "gtkutil/window/PersistentTransientWindow.h"

namespace Gtk { class Notebook; class Widget; }
typedef struct _GtkNotebookPage GtkNotebookPage;

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
typedef boost::shared_ptr<GroupDialog> GroupDialogPtr;

class GroupDialog
: public gtkutil::PersistentTransientWindow,
	public IGroupDialog
{
	// The window position tracker
	gtkutil::WindowPosition _windowPosition;

	// The structure for each notebook page
	struct Page
	{
		std::string name;
		Gtk::Widget* page;
		std::string title;
	};
	typedef std::vector<Page> Pages;

	// The actual list instance
	Pages _pages;

	// The tab widget
	Gtk::Notebook* _notebook;
	sigc::connection _notebookSwitchEvent;

	// The page number of the currently active page widget
	int _currentPage;

private:
	// Private constructor creates GTK widgets etc.
	GroupDialog();

	// TransientWindow events. These deal with window position tracking.
	void _preShow();
	void _postShow();
	void _preHide();

public:
	~GroupDialog();

	/**
	 * Static method to construct the GroupDialog instance.
	 */
	static void construct();

	// Documentation: see igroupdialog.h
	Gtk::Widget* addPage(const std::string& name,
					   const std::string& tabLabel, const std::string& tabIcon,
					   Gtk::Widget& page, const std::string& windowLabel,
					   const std::string& insertBefore);

	// Removes a given page
	void removePage(const std::string& name);

	/** greebo: Sets the active tab to the given widget.
	 *
	 * @page: The widget that should be displayed, must have been added
	 * 		  using addPage() beforehand.
	 */
	void setPage(Gtk::Widget* page);

	/** greebo: Activated the named page. The <name> parameter
	 * 			refers to the name string passed to the addPage() method.
	 */
	void setPage(const std::string& name);

	// Toggles page/dialog visibility
	virtual void togglePage(const std::string& name);

	/** greebo: Returns the widget of the currently visible page.
	 */
	Gtk::Widget* getPage();

	/**
	 * greebo: Returns the name of the current groupdialog page or "" if none is set.
	 */
	std::string getPageName();

	// Returns the window widget containing the GroupDialog.
	Glib::RefPtr<Gtk::Window> getDialogWindow();
	void showDialogWindow();
	void hideDialogWindow();

	// Detaches the notebook and relocates it to another parent container
	void reparentNotebook(Gtk::Widget* newParent);
	void reparentNotebookToSelf();

	/** greebo: Safely disconnects this window from
	 * 			the eventmanager and saves the window position.
	 */
	void onRadiantShutdown();

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

	// Gets called when the user selects a new tab (updates the title)
	void onPageSwitch(GtkNotebookPage* notebookPage, guint pageNumber);
};

} // namespace ui

#endif /*GROUPDIALOG_H_*/
