#pragma once

#include "ui/iuserinterface.h"
#include <wx/aui/auibook.h>

#include "wxutil/Bitmap.h"
#include "wxutil/menu/PopupMenu.h"

namespace ui
{

class PropertyNotebook :
    public wxAuiNotebook
{
public:
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
                Favourites = 300,
                Console = 400,
                TextureBrowser = 500,
                End = 5000
            };
        };

        // Defines the position page in the group dialog (defaults to "End")
        // See the predefined Position enum for already existing positions
        int position = Position::End;
    };
    typedef std::shared_ptr<Page> PagePtr;

private:
    std::map<int, Page> _pages;
    std::map<std::string, IUserControl::Ptr> _controls;
    std::unique_ptr<wxImageList> _imageList;

    wxutil::PopupMenuPtr _popupMenu;

public:
    PropertyNotebook(wxWindow* parent) :
        wxAuiNotebook(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, 
            wxNB_TOP | wxAUI_NB_TAB_MOVE | wxAUI_NB_SCROLL_BUTTONS)
    {
        SetName("PropertyNotebook");

        _imageList.reset(new wxImageList(16, 16));
        SetImageList(_imageList.get());

        Bind(wxEVT_NOTEBOOK_PAGE_CHANGED, &PropertyNotebook::onPageSwitch, this);
        Bind(wxEVT_AUINOTEBOOK_TAB_RIGHT_UP, &PropertyNotebook::onTabRightClick, this);

        _popupMenu = std::make_shared<wxutil::PopupMenu>();
        _popupMenu->addItem(
            new wxMenuItem(nullptr, wxID_ANY, _("Undock"), ""),
            [this]() { undockTab(); }
        );
    }

    void addControl(const std::string& controlName)
    {
        auto control = GlobalUserInterface().findControl(controlName);

        if (!control)
        {
            throw std::logic_error("There's no such control: " + controlName);
        }

        _controls.emplace(controlName, control);

        auto page = std::make_shared<Page>();

        page->name = controlName;
        page->windowLabel = control->getDisplayName();
        page->page = control->createWidget(this);
        page->tabIcon = control->getIcon();
        page->tabLabel = control->getDisplayName();
        page->position = 0;

        // Move to the end of the existing controls
        for (const auto& existing : _pages)
        {
            page->position = std::max(page->position, existing.second.position);
        }

        addPage(page);
    }

    wxWindow* addPage(const PagePtr& page)
    {
        // Make sure the notebook is visible before adding pages
        Show();

        // Load the icon
        int imageId = page->tabIcon.empty() ? -1 :
            _imageList->Add(wxutil::GetLocalBitmap(page->tabIcon));

        // Handle position conflicts first
        auto conflictingPage = _pages.find(page->position);

        // Move back one position until we find a free slot
        while (conflictingPage != _pages.end())
        {
            page->position = conflictingPage->second.position + 1;

            conflictingPage = _pages.find(page->position);
        }

        // Create the notebook page
        size_t insertPosition = GetPageCount();

        // Find a page with a higher position value and sort the incoming one to the left
        for (auto existing : _pages)
        {
            if (page->position < existing.second.position)
            {
                // Found, extract the tab position and break the loop
                insertPosition = FindPage(existing.second.page);
                break;
            }
        }

        page->page->Reparent(this);
        InsertPage(insertPosition, page->page, page->tabLabel, false, imageId);

        // Add this page by copy to the local list
        _pages.insert(std::make_pair(page->position, Page(*page)));

        // During the startup phase (when pages are added) we want to activate the 
        // newly added page if it was the active one during the last shutdown.
        if (this->IsShownOnScreen())
        {
#if 0
            std::string lastShownPage = registry::getValue<std::string>(RKEY_LAST_SHOWN_PAGE);

            if (!lastShownPage.empty() && page->name == lastShownPage)
            {
                setPage(lastShownPage);
            }
#endif
        }

        return page->page;
    }

    void removePage(const std::string& name)
    {
        // Find the page with that name
        for (auto i = _pages.begin(); i != _pages.end(); ++i)
        {
            // Skip the wrong ones
            if (i->second.name != name) continue;

            // Remove the page from the notebook
            DeletePage(FindPage(i->second.page));

            // Remove the page and break the loop
            _pages.erase(i);
            break;
    }
}

private:
    void onPageSwitch(wxBookCtrlEvent& ev)
    {
#if 0
        updatePageTitle(ev.GetSelection());

        // Store the page's name into the registry for later retrieval
        registry::setValue(RKEY_LAST_SHOWN_PAGE, getPageName());
#endif
        // Be sure to skip the event, otherwise pages stay hidden
        ev.Skip();
    }

    void onTabRightClick(wxAuiNotebookEvent& ev)
    {
        SetSelection(ev.GetSelection());
        _popupMenu->show(this);
    }

    void undockTab()
    {
        auto selectedPageIndex = GetSelection();

        if (selectedPageIndex < 0) return;

        // Look up the page in the _pages dictionary by the page widget
        wxWindow* win = GetPage(static_cast<size_t>(selectedPageIndex));

        if (win == nullptr) return;

        for (const auto& page : _pages)
        {
            if (page.second.page == win)
            {
                auto controlName = page.second.name;

                // Remove the page, we're done
                removePage(page.second.name);

                // Get the control name and create a floating window
                GlobalMainFrame().createFloatingControl(controlName);
                break;
            }
        }
    }
};

}
