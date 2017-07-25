#include "GroupDialog.h"

#include "iregistry.h"
#include "imainframe.h"
#include "iuimanager.h"
#include "ieventmanager.h"
#include "i18n.h"
#include <iostream>
#include <vector>

#include <wx/wxprec.h>
#include <wx/notebook.h>
#include <wx/bookctrl.h>
#include <wx/artprov.h>
#include <wx/sizer.h>
#include <wx/panel.h>

#include "registry/registry.h"
#include "LocalBitmapArtProvider.h"

namespace ui
{
	
namespace
{
	const std::string RKEY_ROOT = "user/ui/groupDialog/";
	const std::string RKEY_WINDOW_STATE = RKEY_ROOT + "window";
	const std::string RKEY_LAST_SHOWN_PAGE = RKEY_ROOT + "lastShownPage";

	const char* const WINDOW_TITLE = N_("Entity");
}

GroupDialog::GroupDialog() :
	TransientWindow(_(WINDOW_TITLE), GlobalMainFrame().getWxTopLevelWindow(), true),
	_currentPage(0)
{
	SetName("GroupDialog");
	
	// Create all the widgets and pack them into the window
	populateWindow();

	// Connect the window position tracker
	InitialiseWindowPosition(300, 400, RKEY_WINDOW_STATE);
}

wxFrame* GroupDialog::getDialogWindow()
{
	return this;
}

// Public static method to construct the instance
void GroupDialog::construct()
{
	InstancePtr() = GroupDialogPtr(new GroupDialog);

	GlobalRadiant().signal_radiantShutdown().connect(
        sigc::mem_fun(*InstancePtr(), &GroupDialog::onRadiantShutdown)
    );

	GlobalRadiant().signal_radiantStarted().connect(
		sigc::mem_fun(*InstancePtr(), &GroupDialog::onRadiantStartup)
	);
}

void GroupDialog::reparentNotebook(wxWindow* newParent)
{
	if (_notebook->GetContainingSizer() != NULL)
    {
        _notebook->GetContainingSizer()->Detach(_notebook.get());
    }

    _notebook->Reparent(newParent);

	if (newParent->GetSizer() != NULL)
	{
		newParent->GetSizer()->Add(_notebook.get(), 1, wxEXPAND);
	}
}

void GroupDialog::reparentNotebookToSelf()
{
	reparentNotebook(this);
}

void GroupDialog::populateWindow()
{
	wxPanel* panel = new wxPanel(this, wxID_ANY);
	panel->SetSizer(new wxBoxSizer(wxVERTICAL));
	
	wxBoxSizer* vbox = new wxBoxSizer(wxVERTICAL);
	panel->GetSizer()->Add(vbox, 1, wxEXPAND | wxALL, 12);

	_notebook = new wxNotebook(panel, wxID_ANY, 
		wxDefaultPosition, wxDefaultSize, wxNB_TOP, "GroupDialogNB");

	_notebook->Connect(wxEVT_NOTEBOOK_PAGE_CHANGED, 
		wxBookCtrlEventHandler(GroupDialog::onPageSwitch), NULL, this);

	_imageList.reset(new wxImageList(16, 16));
	_notebook->SetImageList(_imageList.get());

	vbox->Add(_notebook.get(), 1, wxEXPAND);
}

wxWindow* GroupDialog::getPage()
{
	return _notebook->GetCurrentPage();
}

std::string GroupDialog::getPageName()
{
	// Get the widget
	wxWindow* curPage = getPage();

	// Now cycle through the list of pages and find the matching one
	for (Pages::value_type& i : _pages)
	{
		if (i.second.page == curPage)
		{
			// Found the page
			return i.second.name;
		}
	}

	// not found
	return "";
}

// Display the named page
void GroupDialog::setPage(const std::string& name)
{
	// Now search for the correct page.
	for (Pages::value_type& i : _pages)
	{
		if (i.second.name == name)
		{
			// Found page. Set it to active if it is not already active.
			if (i.second.page != nullptr && getPage() != i.second.page)
			{
				setPage(i.second.page);
			}

			// Show the window if the notebook is hosted here
			if (_notebook->GetParent() == this)
			{
				Show();
			}

			// Don't continue the loop, we've found the page
			break;
		}
	}
}

void GroupDialog::setPage(wxWindow* page)
{
	if (page == nullptr) return;

	int pageIndex = _notebook->FindPage(page);

	if (pageIndex != wxNOT_FOUND)
	{
		_notebook->SetSelection(pageIndex);
	}
}

void GroupDialog::togglePage(const std::string& name)
{
	// We still own the notebook in this dialog
	if (getPageName() != name || !IsShownOnScreen())
	{
		// page not yet visible, show it
		setPage(name);

		// Make sure the group dialog is visible, but only if we own the notebook
		if (!IsShownOnScreen() && wxGetTopLevelParent(_notebook.get()) == this)
		{
			showDialogWindow();
		}
	}
	else
	{
        if (wxGetTopLevelParent(_notebook.get()) == this)
        {
            // page is already active, hide the dialog
            hideDialogWindow();
        }
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
	Show();
}

void GroupDialog::hideDialogWindow()
{
	Hide();
}

// Public static method to toggle the window visibility
void GroupDialog::toggle()
{
    Instance().ToggleVisibility();
}

// Post-show callback from TransientWindow
void GroupDialog::_postShow()
{
	TransientWindow::_postShow();

	// Activate the most recently shown page
	std::string lastShownPage = registry::getValue<std::string>(RKEY_LAST_SHOWN_PAGE);

	if (!lastShownPage.empty())
	{
		setPage(lastShownPage);
	}

	// Unset the focus widget for this window to avoid the cursor
	// from jumping into any entry fields
	this->SetFocus();
}

void GroupDialog::onRadiantStartup()
{
	std::string lastShownPage = registry::getValue<std::string>(RKEY_LAST_SHOWN_PAGE);	
	
	if (!lastShownPage.empty())
	{
		setPage(lastShownPage);
	}
}

void GroupDialog::onRadiantShutdown()
{
	if (IsShownOnScreen())
	{
		Hide();
	}

	// Safely disconnect from the notebook before shutting down
    _notebook->Disconnect(wxEVT_NOTEBOOK_PAGE_CHANGED,
                          wxBookCtrlEventHandler(GroupDialog::onPageSwitch), NULL, this);

	// Destroy the window (after it has been disconnected from the Eventmanager)
	SendDestroyEvent();
	InstancePtr().reset();
}

wxWindow* GroupDialog::addPage(const PagePtr& page)
{
	// Make sure the notebook is visible before adding pages
	_notebook->Show();

	// Load the icon
	int imageId = page->tabIcon.empty() ? -1 : 
		_imageList->Add(wxArtProvider::GetBitmap(LocalBitmapArtProvider::ArtIdPrefix() + page->tabIcon));
	
	// Handle position conflicts first
	Pages::const_iterator conflictingPage = _pages.find(page->position);

	// Move back one position until we find a free slot
	while (conflictingPage != _pages.end())
	{
		page->position = conflictingPage->second.position + 1;

		conflictingPage = _pages.find(page->position);
	}

	// Create the notebook page
	size_t insertPosition = _notebook->GetPageCount();

	// Find a page with a higher position value and sort the incoming one to the left
	for (Pages::value_type existing : _pages)
	{
		if (page->position < existing.second.position)
		{
			// Found, extract the tab position and break the loop
			insertPosition = _notebook->FindPage(existing.second.page);
			break;
		}
	}

	page->page->Reparent(_notebook.get());
	_notebook->InsertPage(insertPosition, page->page, page->tabLabel, false, imageId);

	// Add this page by copy to the local list
	_pages.insert(std::make_pair(page->position, Page(*page)));

	// During the startup phase (when pages are added) we want to activate the 
	// newly added page if it was the active one during the last shutdown.
	if (this->IsShownOnScreen())
	{
		std::string lastShownPage = registry::getValue<std::string>(RKEY_LAST_SHOWN_PAGE);

		if (!lastShownPage.empty() && page->name == lastShownPage)
		{
			setPage(lastShownPage);
		}
	}

	return page->page;
}

void GroupDialog::removePage(const std::string& name)
{
	// Find the page with that name
	for (Pages::iterator i = _pages.begin(); i != _pages.end(); ++i)
	{
		// Skip the wrong ones
		if (i->second.name != name) continue;

		// Remove the page from the notebook
		_notebook->DeletePage(_notebook->FindPage(i->second.page));

		// Remove the page and break the loop, iterators are invalid
		_pages.erase(i);
		break;
	}
}

void GroupDialog::updatePageTitle(int pageNumber)
{
	if (pageNumber < 0) return;

	// Look up the page in the _pages dictionary by the page widget
	wxWindow* win = _notebook->GetPage(static_cast<size_t>(pageNumber));

	if (win == nullptr) return;

	for (const Pages::value_type& page : _pages)
	{
		if (page.second.page == win)
		{
			SetTitle(page.second.windowLabel);
			break;
		}
	}
}

void GroupDialog::onPageSwitch(wxBookCtrlEvent& ev)
{
	updatePageTitle(ev.GetSelection());
    
	// Store the page's name into the registry for later retrieval
	registry::setValue(RKEY_LAST_SHOWN_PAGE, getPageName());

    // Be sure to skip the event, otherwise pages stay hidden
    ev.Skip();
}

} // namespace ui
