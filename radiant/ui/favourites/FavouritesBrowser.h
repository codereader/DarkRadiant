#pragma once

#include <wx/listctrl.h>
#include <wx/imaglist.h>
#include <wx/toolbar.h>
#include <wx/checkbox.h>
#include <sigc++/connection.h>

#include "wxutil/DockablePanel.h"
#include "wxutil/event/SingleIdleCallback.h"
#include "wxutil/menu/PopupMenu.h"

namespace ui
{

class FavouritesBrowser :
    public wxutil::DockablePanel,
    public wxutil::SingleIdleCallback
{
private:
    // Holds the data used to construct the wxListItem
    struct FavouriteItem
    {
        std::string typeName;
        std::string fullPath;
        std::string leafName;
    };

    wxListView* _listView;

    std::unique_ptr<wxImageList> _iconList;

    struct FavouriteCategory
    {
        std::string typeName;
        std::string displayName;
        std::string iconName;
        int iconIndex;
        wxToolBarToolBase* checkButton;
    };

    // Maps decl type to icon index
    std::list<FavouriteCategory> _categories;
    std::list<sigc::connection> _changedConnections;
    std::list<FavouriteItem> _listItems;

    wxCheckBox* _showFullPath;
    bool _reloadFavouritesOnIdle;

    wxutil::PopupMenuPtr _popupMenu;

public:
    FavouritesBrowser(wxWindow* parent);
    ~FavouritesBrowser() override;

protected:
    void onIdle() override;
    void onPanelActivated() override;
    void onPanelDeactivated() override;

private:
    void queueUpdate();

    void connectListeners();
    void disconnectListeners();

    wxToolBar* createLeftToolBar();
    wxToolBar* createRightToolBar();
    void onFavouritesChanged();
    void reloadFavourites();
    void setupCategories();
    void constructPopupMenu();
    void clearItems();

    void onRemoveFromFavourite();
    void onCategoryToggled(wxCommandEvent& ev);
    void onShowFullPathToggled(wxCommandEvent& ev);
    void onContextMenu(wxContextMenuEvent& ev);
    void onItemActivated(wxListEvent& ev);

    // Returns the list of all selected item indices
    std::vector<long> getSelectedItems();
    std::string getSelectedTypeName();

    void onApplyTextureToSelection();
    bool testSingleTextureSelected();
    void onApplySoundToSelection();
    bool testApplySoundToSelection();
    void onCreateEntity();
    bool testCreateEntity();
    void onCreateSpeaker();
    bool testCreateSpeaker();

    bool selectionAllowsEntityCreation();
};

}
