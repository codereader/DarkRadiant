#pragma once

#include <wx/aui/auibook.h>

#include "xmlutil/Node.h"
#include "wxutil/menu/PopupMenu.h"

namespace ui
{

class AuiLayout;

/**
 * Specialised AUI notebook implementation, adding save/restore capabilities
 * and an interface to allow floating panes to be dragged in as new tab.
 */
class PropertyNotebook :
    public wxAuiNotebook
{
private:
    AuiLayout& _layout;

    struct Page
    {
        // The name of the control
        std::string controlName;

        // If present into the image list, this is the bitmap index
        int tabIconIndex = -1;

        // the actual widget to be added
        wxWindow* page = nullptr;
    };

    std::list<Page> _pages;
    std::unique_ptr<wxImageList> _imageList;

    wxutil::PopupMenuPtr _popupMenu;

    wxFrame* _dropHint;

public:
    PropertyNotebook(wxWindow* parent, AuiLayout& owner);
    ~PropertyNotebook() override;

    void addControl(const std::string& controlName);
    void focusControl(const std::string& controlName);
    void removeControl(const std::string& controlName);

    // Returns true if the given control is loaded in a tab
    bool controlExists(const std::string& controlName);

    void saveState();
    void restoreState();
    void restoreDefaultState();

    void showDropHint(const wxRect& size);
    void hideDropHint();

private:
    void restoreState(const xml::NodeList& pagesPath);

    std::string getSelectedPageName();
    std::string getSelectedControlName();
    int findControlIndexByName(const std::string& controlName);
    std::string findControlNameByWindow(wxWindow* window);
    int findImageIndexForControl(const std::string& controlName);

    void onPageSwitch(wxBookCtrlEvent& ev);
    void onTabRightClick(wxAuiNotebookEvent& ev);

    void undockSelectedTab();
    void closeSelectedTab();
};

}
