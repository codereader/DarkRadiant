#pragma once

#include "ui/iuserinterface.h"
#include <wx/aui/auibook.h>

#include "wxutil/menu/PopupMenu.h"

namespace ui
{

class AuiLayout;

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
    AuiLayout& _layout;

    std::map<int, Page> _pages;
    std::map<std::string, IUserControl::Ptr> _controls;
    std::unique_ptr<wxImageList> _imageList;

    wxutil::PopupMenuPtr _popupMenu;

public:
    PropertyNotebook(wxWindow* parent, AuiLayout& owner);

    void addControl(const std::string& controlName);

    wxWindow* addPage(const PagePtr& page);

    void removePage(const std::string& name);

    void saveState(const std::string& registryRootPath);
    void restoreState(const std::string& registryRootPath);

private:
    void onPageSwitch(wxBookCtrlEvent& ev);
    void onTabRightClick(wxAuiNotebookEvent& ev);

    void undockTab();
};

}
