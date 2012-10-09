#include "GroupDialog.h"

#include "iregistry.h"
#include "imainframe.h"
#include "iuimanager.h"
#include "ieventmanager.h"
#include "i18n.h"
#include "gtkutil/window/PersistentTransientWindow.h"
#include <iostream>
#include <vector>

#include <gtkmm/box.h>
#include <gtkmm/image.h>
#include <gtkmm/label.h>
#include <gtkmm/notebook.h>

namespace ui
{
	namespace
	{
		const std::string RKEY_ROOT = "user/ui/groupDialog/";
		const std::string RKEY_WINDOW_STATE = RKEY_ROOT + "window";

		const char* const WINDOW_TITLE = N_("Entity");
	}

GroupDialog::GroupDialog() :
	gtkutil::PersistentTransientWindow(_(WINDOW_TITLE), GlobalMainFrame().getTopLevelWindow(), true),
	_currentPage(0)
{
	// Set the default border width in accordance to the HIG
	set_border_width(12);
	set_type_hint(Gdk::WINDOW_TYPE_HINT_DIALOG);
	
	// Create all the widgets and pack them into the window
	populateWindow();

	// Register this dialog to the EventManager, so that shortcuts can propagate to the main window

	// greebo: Disabled this, because the EntityInspector was propagating keystrokes back to the main
	//         main window, even when the cursor was focused on entry fields.
	// greebo: Enabled this again, it seems to annoy users (issue #458)
	GlobalEventManager().connectDialogWindow(this);

	// Connect the window position tracker
	_windowPosition.loadFromPath(RKEY_WINDOW_STATE);

	_windowPosition.connect(this);
	_windowPosition.applyPosition();
}

GroupDialog::~GroupDialog()
{
	_notebookSwitchEvent.disconnect();
}

Glib::RefPtr<Gtk::Window> GroupDialog::getDialogWindow()
{
	return getRefPtr();
}

// Public static method to construct the instance
void GroupDialog::construct()
{
	InstancePtr() = GroupDialogPtr(new GroupDialog);
	GlobalRadiant().signal_radiantShutdown().connect(
        sigc::mem_fun(*InstancePtr(), &GroupDialog::onRadiantShutdown)
    );
}

void GroupDialog::reparentNotebook(Gtk::Widget* newParent)
{
	// greebo: Use the reparent method, the commented code below
	// triggers an unrealise signal.
	_notebook->reparent(*newParent);
}

void GroupDialog::reparentNotebookToSelf()
{
	reparentNotebook(this);
}

void GroupDialog::populateWindow()
{
	_notebook = Gtk::manage(new Gtk::Notebook);
	add(*_notebook);

	_notebook->set_tab_pos(Gtk::POS_TOP);

	_notebookSwitchEvent = _notebook->signal_switch_page().connect(
        sigc::mem_fun(*this, &GroupDialog::onPageSwitch)
    );
}

Gtk::Widget* GroupDialog::getPage()
{
	return _notebook->get_nth_page(_notebook->get_current_page());
}

std::string GroupDialog::getPageName()
{
	// Get the widget
	Gtk::Widget* curPage = getPage();

	// Now cycle through the list of pages and find the matching one
	for (std::size_t i = 0; i < _pages.size(); i++)
	{
		if (_pages[i].page == curPage)
		{
			// Found page. Set it to active if it is not already active.
			return _pages[i].name;
		}
	}

	// not found
	return "";
}

// Display the named page
void GroupDialog::setPage(const std::string& name)
{
	// Force a Pageswitch to ensure the texture browser is properly redrawn.
	setPage(_pages[0].page);

	// Now search for the correct page.
	for (std::size_t i = 0; i < _pages.size(); i++)
	{
		if (_pages[i].name == name)
		{
			// Found page. Set it to active if it is not already active.
			if (getPage() != _pages[i].page)
			{
				setPage(_pages[i].page);
			}

			// Show the window if the notebook is hosted here
			if (_notebook->get_parent() == this)
			{
				show();
			}

			// Don't continue the loop, we've found the page
			break;
		}
	}
}

void GroupDialog::setPage(Gtk::Widget* page)
{
	_currentPage = _notebook->page_num(*page);
	_notebook->set_current_page(_currentPage);
}

void GroupDialog::togglePage(const std::string& name)
{
	// We still own the notebook in this dialog
	if (getPageName() != name || !is_visible())
	{
		// page not yet visible, show it
		setPage(name);
	}
	else
	{
		// page is already active, hide the dialog
		hideDialogWindow();
	}
}

GroupDialogPtr& GroupDialog::InstancePtr()
{
	static GroupDialogPtr _instancePtr;
	return _instancePtr;
}

// Public method to retrieve the instance
GroupDialog& GroupDialog::Instance()
{
	if (InstancePtr() == NULL)
	{
		construct();
	}

	return *InstancePtr();
}

void GroupDialog::showDialogWindow()
{
	show();
}

void GroupDialog::hideDialogWindow()
{
	hide();
}

// Public static method to toggle the window visibility
void GroupDialog::toggle()
{
    Instance().toggleVisibility();
}

// Pre-hide callback from TransientWindow
void GroupDialog::_preHide()
{
	if (is_visible())
	{
		// Save the window position, to make sure
		_windowPosition.readPosition();
	}

	// Tell the position tracker to save the information
	_windowPosition.saveToPath(RKEY_WINDOW_STATE);
}

// Pre-show callback from TransientWindow
void GroupDialog::_preShow()
{
	// Restore the position
	_windowPosition.applyPosition();
}

// Post-show callback from TransientWindow
void GroupDialog::_postShow()
{
	// Unset the focus widget for this window to avoid the cursor
	// from jumping into any entry fields
	unset_focus();
}

void GroupDialog::onRadiantShutdown()
{
	hide();

	GlobalEventManager().disconnectDialogWindow(this);

	// Call the PersistentTransientWindow::destroy chain
	destroy();

	InstancePtr().reset();
}

Gtk::Widget* GroupDialog::addPage(const std::string& name,
								const std::string& tabLabel,
								const std::string& tabIcon,
								Gtk::Widget& page,
								const std::string& windowLabel,
								const std::string& insertBefore)
{
	// Make sure the notebook is visible before adding pages
	_notebook->show();

	// Create the icon GtkImage and tab label
	Gtk::Image* icon = Gtk::manage(new Gtk::Image(GlobalUIManager().getLocalPixbuf(tabIcon)));
	Gtk::Label* label = Gtk::manage(new Gtk::Label(tabLabel));

	// Pack into an hbox to create the title widget
	Gtk::HBox* titleWidget = Gtk::manage(new Gtk::HBox(false, 3));
	titleWidget->pack_start(*icon, false, false, 0);
	titleWidget->pack_start(*label, false, false, 0);
	titleWidget->show_all();

	// Show the child page before adding it to the notebook (GTK recommendation)
	page.show();

	// Create the notebook page
	gint position = -1;
	Pages::iterator insertIter = _pages.end();

	if (!insertBefore.empty())
	{
		// Find the page with that name
		for (Pages::iterator i = _pages.begin(); i != _pages.end(); ++i)
        {
			// Skip the wrong ones
			if (i->name != insertBefore) continue;

			// Found, extract the tab position and break the loop
			position = _notebook->page_num(*i->page);
			insertIter = i;
			break;
		}
	}

	Gtk::Widget* notebookPage = _notebook->get_nth_page(
		_notebook->insert_page(page, *titleWidget, position)
	);

	// Add this page to the local list
	Page newPage;
	newPage.name = name;
	newPage.page = notebookPage;
	newPage.title = windowLabel;

	_pages.insert(insertIter, newPage);

	return notebookPage;
}

void GroupDialog::removePage(const std::string& name)
{
	// Find the page with that name
	for (Pages::iterator i = _pages.begin(); i != _pages.end(); ++i)
	{
		// Skip the wrong ones
		if (i->name != name) continue;

		// Remove the page from the notebook
		_notebook->remove(*i->page);

		// Remove the page and break the loop, iterators are invalid
		_pages.erase(i);
		break;
	}
}

void GroupDialog::updatePageTitle(unsigned int pageNumber)
{
	if (pageNumber < _pages.size())
	{
		set_title(_pages[pageNumber].title);
	}
}

void GroupDialog::onPageSwitch(GtkNotebookPage* notebookPage, guint pageNumber)
{
    // Check if window is realised first, because we may be being called during
    // widget destruction and the set_title call will crash if so.
	if (is_realized())
    {
        updatePageTitle(pageNumber);
    }
}

} // namespace ui
