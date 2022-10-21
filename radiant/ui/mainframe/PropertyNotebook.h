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

        // If already loaded into the image list, this is the bitmap index
        int tabIconIndex = -1;

        // the actual widget to be added
        wxWindow* page = nullptr;

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

    // Returns true if the given control is loaded in a tab
    bool controlExists(const std::string& controlName);

    void focusControl(const std::string& controlName);

    void saveState();
    void restoreState();

private:
    std::string getSelectedPageName();
    int findControlIndexByName(const std::string& controlName);
    std::string findControlNameByWindow(wxWindow* window);
    int getImageIndexForControl(const std::string& controlName);
    std::string getSelectedControlName();

    void onPageSwitch(wxBookCtrlEvent& ev);
    void onTabRightClick(wxAuiNotebookEvent& ev);

    void undockTab();
    void closeTab();
};

}
