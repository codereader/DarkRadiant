#include "PropertyNotebook.h"

#include "wxutil/Bitmap.h"
#include "AuiLayout.h"

namespace ui
{

namespace
{
    const std::string RKEY_ROOT = "user/ui/mainFrame/propertyPanel/";
    const std::string RKEY_LAST_SHOWN_PAGE = RKEY_ROOT + "lastShownPage";
    const std::string RKEY_PAGES = RKEY_ROOT + "pages";

    const std::string PAGE_NODE_NAME = "page";
    const std::string CONTROL_NAME_ATTRIBUTE = "controlName";
}

PropertyNotebook::PropertyNotebook(wxWindow* parent, AuiLayout& owner) :
    wxAuiNotebook(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, 
        wxNB_TOP | wxAUI_NB_TAB_MOVE | wxAUI_NB_SCROLL_BUTTONS),
    _layout(owner)
{
    SetName("PropertyNotebook");

    _imageList.reset(new wxImageList(16, 16));
    SetImageList(_imageList.get());

    Bind(wxEVT_AUINOTEBOOK_PAGE_CHANGED, &PropertyNotebook::onPageSwitch, this);
    Bind(wxEVT_AUINOTEBOOK_TAB_RIGHT_UP, &PropertyNotebook::onTabRightClick, this);

    _popupMenu = std::make_shared<wxutil::PopupMenu>();
    _popupMenu->addItem(
        new wxMenuItem(nullptr, wxID_ANY, _("Undock"), ""),
        [this]() { undockTab(); }
    );
    _popupMenu->addItem(
        new wxMenuItem(nullptr, wxID_ANY, _("Close"), ""),
        [this]() { closeTab(); }
    );
}

void PropertyNotebook::addControl(const std::string& controlName)
{
    auto control = GlobalUserInterface().findControl(controlName);

    if (!control)
    {
        throw std::logic_error("There's no such control: " + controlName);
    }

    _controls.emplace(controlName, control);

    auto page = std::make_shared<Page>();

    page->name = controlName;
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

wxWindow* PropertyNotebook::addPage(const PagePtr& page)
{
    // Make sure the notebook is visible before adding pages
    Show();

    // Load the icon
    page->tabIconIndex = page->tabIcon.empty() ? -1 :
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
    InsertPage(insertPosition, page->page, page->tabLabel, false, page->tabIconIndex);

    // Add this page by copy to the local list
    _pages.emplace(page->position, Page(*page));

    return page->page;
}

void PropertyNotebook::removePage(const std::string& name)
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

std::string PropertyNotebook::getSelectedPageName()
{
    auto selectedPageIndex = GetSelection();

    if (selectedPageIndex < 0) return {};

    // Look up the page in the _pages dictionary by the page widget
    auto win = GetPage(static_cast<size_t>(selectedPageIndex));

    return findControlNameByWindow(win);
}

void PropertyNotebook::onPageSwitch(wxBookCtrlEvent& ev)
{
    // Store the page's name into the registry for later retrieval
    registry::setValue(RKEY_LAST_SHOWN_PAGE, getSelectedPageName());

    auto selectedPageIndex = GetSelection();

    // Set the active/inactive status of all windows
    for (std::size_t i = 0; i < GetPageCount(); ++i)
    {
        auto win = GetPage(i);

        if (i == selectedPageIndex)
        {
            _layout.ensureControlIsActive(win);
        }
        else
        {
            _layout.ensureControlIsInactive(win);
        }
    }

    // Be sure to skip the event, otherwise pages stay hidden
    ev.Skip();
}

void PropertyNotebook::onTabRightClick(wxAuiNotebookEvent& ev)
{
    SetSelection(ev.GetSelection());
    _popupMenu->show(this);
}

std::string PropertyNotebook::getSelectedControlName()
{
    auto selectedPageIndex = GetSelection();

    if (selectedPageIndex < 0) return {};

    // Look up the page in the _pages dictionary by the page widget
    auto win = GetPage(static_cast<size_t>(selectedPageIndex));

    if (win == nullptr) return {};

    return findControlNameByWindow(win);
}

void PropertyNotebook::undockTab()
{
    auto controlName = getSelectedControlName();

    if (!controlName.empty())
    {
        // Remove the page
        removePage(controlName);

        // Get the control name and create a floating window
        _layout.createFloatingControl(controlName);
    }
}

void PropertyNotebook::closeTab()
{
    auto controlName = getSelectedControlName();

    if (!controlName.empty())
    {
        // Remove the page
        removePage(controlName);
    }
}

std::string PropertyNotebook::findControlNameByWindow(wxWindow* window)
{
    for (const auto& page : _pages)
    {
        if (page.second.page == window)
        {
            return page.second.name;
        }
    }

    return {};
}

int PropertyNotebook::findControlIndexByName(const std::string& controlName)
{
    for (const auto& page : _pages)
    {
        if (page.second.name == controlName)
        {
            return FindPage(page.second.page);
        }
    }

    return -1;
}

int PropertyNotebook::getImageIndexForControl(const std::string& controlName)
{
    for (const auto& page : _pages)
    {
        if (page.second.name == controlName)
        {
            return page.second.tabIconIndex;
        }
    }

    return -1;
}

void PropertyNotebook::saveState()
{
    GlobalRegistry().deleteXPath(RKEY_PAGES);

    auto pagesKey = GlobalRegistry().createKey(RKEY_PAGES);

    // Save the names and position of all controls
    for (std::size_t i = 0; i < GetPageCount(); ++i)
    {
        auto controlName = findControlNameByWindow(GetPage(i));

        if (controlName.empty()) continue;

        auto paneNode = pagesKey.createChild(PAGE_NODE_NAME);
        paneNode.setAttributeValue(CONTROL_NAME_ATTRIBUTE, controlName);
    }

    registry::setValue(RKEY_LAST_SHOWN_PAGE, getSelectedPageName());
}

void PropertyNotebook::restoreState()
{
    std::vector<std::string> sortedControlNames;

    // Restore all missing pages
    auto pageNodes = GlobalRegistry().findXPath(RKEY_PAGES + "//*");
    for (int i = 0; i < pageNodes.size(); ++i)
    {
        if (pageNodes.at(i).getName() != PAGE_NODE_NAME) continue;

        auto controlName = pageNodes.at(i).getAttributeValue(CONTROL_NAME_ATTRIBUTE);

        auto existingIndex = findControlIndexByName(controlName);

        if (existingIndex == -1)
        {
            addControl(controlName);
            existingIndex = findControlIndexByName(controlName);
        }

        if (existingIndex != i)
        {
            // Move to correct position, keeping image and caption intact
            auto window = GetPage(existingIndex);
            auto imageIndex = getImageIndexForControl(controlName);
            auto pageText = GetPageText(existingIndex);

            RemovePage(existingIndex);
            InsertPage(i, window, pageText, false, imageIndex);
        }
    }

    auto lastShownPage = registry::getValue<std::string>(RKEY_LAST_SHOWN_PAGE);

    for (const auto& page : _pages)
    {
        if (page.second.name == lastShownPage)
        {
            SetSelection(FindPage(page.second.page));
            break;
        }
    }
}

bool PropertyNotebook::controlExists(const std::string& controlName)
{
    return findControlIndexByName(controlName) != -1;
}

void PropertyNotebook::focusControl(const std::string& controlName)
{
    if (auto index = findControlIndexByName(controlName); index != -1)
    {
        SetSelection(index);

        // To prevent the Surface Inspector from focusing the entry box after pressing the shortcut
        if (auto panel = wxDynamicCast(GetPage(index), wxPanel); panel != nullptr)
        {
            panel->SetFocusIgnoringChildren();
        }
    }
}

}
