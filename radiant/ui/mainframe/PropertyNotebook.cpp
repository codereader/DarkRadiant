#include "PropertyNotebook.h"

#include "ui/iuserinterface.h"
#include "wxutil/Bitmap.h"
#include "registry/registry.h"
#include "AuiLayout.h"
#include "i18n.h"

#include <wx/settings.h>

#include "util/ScopedBoolLock.h"

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
    _layout(owner),
    _dropHint(nullptr),
    _restoreInProgress(false)
{
    SetName("PropertyNotebook");

    _imageList.reset(new wxImageList(16, 16));
    SetImageList(_imageList.get());

    Bind(wxEVT_AUINOTEBOOK_PAGE_CHANGED, &PropertyNotebook::onPageSwitch, this);
    Bind(wxEVT_AUINOTEBOOK_TAB_RIGHT_UP, &PropertyNotebook::onTabRightClick, this);

    _popupMenu = std::make_shared<wxutil::PopupMenu>();
    _popupMenu->addItem(
        new wxMenuItem(nullptr, wxID_ANY, _("Undock"), ""),
        [this]() { undockSelectedTab(); }
    );
    _popupMenu->addItem(
        new wxMenuItem(nullptr, wxID_ANY, _("Close"), ""),
        [this]() { closeSelectedTab(); }
    );
}

PropertyNotebook::~PropertyNotebook()
{}

void PropertyNotebook::addControl(const std::string& controlName)
{
    if (controlExists(controlName))
    {
        rWarning() << "Control " << controlName << " has already been added as tab" << std::endl;
        return;
    }

    auto control = GlobalUserInterface().findControl(controlName);

    if (!control)
    {
        throw std::logic_error("There's no such control: " + controlName);
    }

    // Make sure the notebook is visible before adding pages
    Show();

    // Load the icon
    int tabIconIndex = control->getIcon().empty() ? -1 :
        _imageList->Add(wxutil::GetLocalBitmap(control->getIcon()));
    
    auto content = control->createWidget(this);
    content->Reparent(this);

    AddPage(content, control->getDisplayName(), false, tabIconIndex);

    // Add this page by copy to the local list
    _pages.emplace_back(Page
    {
        control->getControlName(),
        tabIconIndex,
        content,
    });

    // Select the new page
    SetSelection(GetPageIndex(content));
}

void PropertyNotebook::removeControl(const std::string& controlName)
{
    // Find the page with that name
    for (auto i = _pages.begin(); i != _pages.end(); ++i)
    {
        // Skip the wrong ones
        if (i->controlName != controlName) continue;

        // Remove the page from the notebook
        DeletePage(GetPageIndex(i->page));

        // Remove the entry and break the loop
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

void PropertyNotebook::onNotebookPaneRestored()
{
    auto selectedPageIndex = GetSelection();
    auto page = GetPage(selectedPageIndex);

    _layout.ensureControlIsActive(page);
}

void PropertyNotebook::onNotebookPaneClosed()
{
    auto selectedPageIndex = GetSelection();
    auto page = GetPage(selectedPageIndex);

    _layout.ensureControlIsInactive(page);
}

void PropertyNotebook::onPageSwitch(wxBookCtrlEvent& ev)
{
    if (_restoreInProgress) return;

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

void PropertyNotebook::undockSelectedTab()
{
    auto controlName = getSelectedControlName();

    closeSelectedTab();

    // Get the control name and create a floating window
    _layout.createFloatingControl(controlName);
}

void PropertyNotebook::closeSelectedTab()
{
    auto controlName = getSelectedControlName();

    if (!controlName.empty())
    {
        removeControl(controlName);
    }
}

std::string PropertyNotebook::findControlNameByWindow(wxWindow* window)
{
    for (const auto& page : _pages)
    {
        if (page.page == window)
        {
            return page.controlName;
        }
    }

    return {};
}

int PropertyNotebook::findControlIndexByName(const std::string& controlName)
{
    for (const auto& page : _pages)
    {
        if (page.controlName == controlName)
        {
            return GetPageIndex(page.page);
        }
    }

    return -1;
}

int PropertyNotebook::findImageIndexForControl(const std::string& controlName)
{
    for (const auto& page : _pages)
    {
        if (page.controlName == controlName)
        {
            return page.tabIconIndex;
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

void PropertyNotebook::restoreState(const xml::NodeList& pages)
{
    // Load the last shown page before restoring the pages
    // Page switches are going to overwrite the saved value
    auto lastShownPage = registry::getValue<std::string>(RKEY_LAST_SHOWN_PAGE);

    for (int i = 0; i < pages.size(); ++i)
    {
        if (pages.at(i).getName() != PAGE_NODE_NAME) continue;

        auto controlName = pages.at(i).getAttributeValue(CONTROL_NAME_ATTRIBUTE);

        auto existingIndex = findControlIndexByName(controlName);

        if (existingIndex == -1)
        {
            addControl(controlName);
            existingIndex = findControlIndexByName(controlName);
        }

        if (existingIndex == wxNOT_FOUND) {
            // Sanity check; avoid passing an incorrect index to wxWidgets
            rWarning() << "PropertyNotebook::restoreState(): failed to find index of control ["
                       << controlName << "]\n";
        }
        else if (existingIndex != i) {
            // Move to correct position, keeping image and caption intact
            auto window = GetPage(existingIndex);
            auto imageIndex = findImageIndexForControl(controlName);
            auto pageText = GetPageText(existingIndex);

            RemovePage(existingIndex);
            InsertPage(i, window, pageText, false, imageIndex);
        }
    }

    // Bring the last shown page to front
    focusControl(lastShownPage);
}

void PropertyNotebook::restoreDefaultState()
{
    restoreState(GlobalRegistry().findXPath(RKEY_PAGES + "[@default='true']//*"));
}

void PropertyNotebook::restoreState()
{
    // Inhibit the page switch events during restore
    util::ScopedBoolLock restoreLock(_restoreInProgress);

    // Check the user settings first
    auto userDefinedPages = GlobalRegistry().findXPath(RKEY_PAGES + "[not(@default='true')]//*");

    // Fall back to the factory defaults if no pages are defined
    if (userDefinedPages.empty())
    {
        restoreDefaultState();
        return;
    }

    restoreState(userDefinedPages);
}

bool PropertyNotebook::controlExists(const std::string& controlName)
{
    return findControlIndexByName(controlName) != -1;
}

bool PropertyNotebook::controlIsVisible(const std::string& controlName)
{
    return getSelectedControlName() == controlName;
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

void PropertyNotebook::showDropHint(const wxRect& size)
{
    if (!_dropHint)
    {
        _dropHint = new wxFrame(this, wxID_ANY, wxEmptyString,
            wxDefaultPosition, wxSize(1, 1),
            wxFRAME_TOOL_WINDOW | wxFRAME_FLOAT_ON_PARENT | wxFRAME_NO_TASKBAR | wxNO_BORDER);

        _dropHint->SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_ACTIVECAPTION));
        _dropHint->SetTransparent(128);
    }

    _dropHint->SetSize(size);
    _dropHint->Raise();
    _dropHint->Show();
}

void PropertyNotebook::hideDropHint()
{
    if (!_dropHint) return;

    _dropHint->Destroy();
    _dropHint = nullptr;
}

}
