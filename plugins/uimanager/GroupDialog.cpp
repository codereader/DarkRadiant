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

#include "LocalBitmapArtProvider.h"

namespace ui
{
	
namespace
{
	const std::string RKEY_ROOT = "user/ui/groupDialog/";
	const std::string RKEY_WINDOW_STATE = RKEY_ROOT + "window";

	const char* const WINDOW_TITLE = N_("Entity");
}

GroupDialog::GroupDialog() :
	TransientWindow(_(WINDOW_TITLE), GlobalMainFrame().getWxTopLevelWindow(), true),
	_currentPage(0)
{
	SetName("GroupDialog");
	
	// Create all the widgets and pack them into the window
	populateWindow();

	// Register this dialog to the EventManager, so that shortcuts can propagate to the main window

	// greebo: Disabled this, because the EntityInspector was propagating keystrokes back to the main
	//         main window, even when the cursor was focused on entry fields.
	// greebo: Enabled this again, it seems to annoy users (issue #458)
	GlobalEventManager().connect(*this);

    // Propagate global shortcuts if the notebook receives them
    GlobalEventManager().connect(*_notebook);

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
}

void GroupDialog::reparentNotebook(wxWindow* newParent)
{
	_notebook->Reparent(newParent);

	if (newParent->GetSizer() != NULL)
	{
		newParent->GetSizer()->Add(_notebook, 1, wxEXPAND);
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

	vbox->Add(_notebook, 1, wxEXPAND);
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
			if (_pages[i].page != NULL && getPage() != _pages[i].page)
			{
				setPage(_pages[i].page);
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
	_notebook->SetSelection(_notebook->FindPage(page));
}

void GroupDialog::togglePage(const std::string& name)
{
	// We still own the notebook in this dialog
	if (getPageName() != name || !IsShownOnScreen())
	{
		// page not yet visible, show it
		setPage(name);

		// Make sure the group dialog is visible, but only if we own the notebook
		if (!IsShownOnScreen() && wxGetTopLevelParent(_notebook) == this)
		{
			showDialogWindow();
		}
	}
	else
	{
        if (wxGetTopLevelParent(_notebook) == this)
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

	// Unset the focus widget for this window to avoid the cursor
	// from jumping into any entry fields
	this->SetFocus();
}

void GroupDialog::onRadiantShutdown()
{
	if (IsShownOnScreen())
	{
		Hide();
	}

	GlobalEventManager().disconnect(*_notebook);
    GlobalEventManager().disconnect(*this);

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
	
	// Create the notebook page
	size_t position = _notebook->GetPageCount();
	Pages::iterator insertIter = _pages.end();

	if (!page->insertBefore.empty())
	{
		// Find the page with that name
		for (Pages::iterator i = _pages.begin(); i != _pages.end(); ++i)
        {
			// Skip the wrong ones
			if (i->name != page->insertBefore) continue;

			// Found, extract the tab position and break the loop
			position = _notebook->FindPage(i->page);
			insertIter = i;
			break;
		}
	}

	page->page->Reparent(_notebook);
	_notebook->InsertPage(position, page->page, page->tabLabel, false, imageId);

	// Add this page by copy to the local list
	_pages.insert(insertIter, Page(*page));

	return page->page;
}

void GroupDialog::removePage(const std::string& name)
{
	// Find the page with that name
	for (Pages::iterator i = _pages.begin(); i != _pages.end(); ++i)
	{
		// Skip the wrong ones
		if (i->name != name) continue;

		// Remove the page from the notebook
		_notebook->DeletePage(_notebook->FindPage(i->page));

		// Remove the page and break the loop, iterators are invalid
		_pages.erase(i);
		break;
	}
}

void GroupDialog::updatePageTitle(int pageNumber)
{
	if (pageNumber >= 0 && pageNumber < static_cast<int>(_pages.size()))
	{
		SetTitle(_pages[pageNumber].windowLabel);
	}
}

void GroupDialog::onPageSwitch(wxBookCtrlEvent& ev)
{
	updatePageTitle(ev.GetSelection());
}

} // namespace ui
