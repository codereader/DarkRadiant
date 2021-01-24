#pragma once

#include "imodule.h"
#include "icommandsystem.h"
#include "idecltypes.h"

#include <wx/listctrl.h>
#include <wx/imaglist.h>
#include <wx/toolbar.h>
#include <wx/checkbox.h>
#include <sigc++/connection.h>

#include "wxutil/menu/PopupMenu.h"

namespace ui
{

class FavouritesBrowser :
    public RegisterableModule
{
private:
    // Holds the data used to construct the wxListItem
    struct FavouriteItem
    {
        decl::Type type;
        std::string fullPath;
    };

    wxFrame* _tempParent;
    wxWindow* _mainWidget;
    wxListView* _listView;

    std::unique_ptr<wxImageList> _iconList;

    struct FavouriteCategory
    {
        decl::Type type;
        std::string displayName;
        std::string iconName;
        int iconIndex;
        wxToolBarToolBase* checkButton;
    };

    // Maps decl type to icon index
    std::list<FavouriteCategory> _categories;
    std::list<sigc::connection> changedConnections;
    std::list<FavouriteItem> _listItems;

    wxCheckBox* _showFullPath;
    bool _updateNeeded;

    wxutil::PopupMenuPtr _popupMenu;

public:
    FavouritesBrowser();

    const std::string& getName() const override;
    const StringSet& getDependencies() const override;
    void initialiseModule(const IApplicationContext& ctx) override;
    void shutdownModule() override;

private:
    void construct();
    wxToolBar* createLeftToolBar();
    wxToolBar* createRightToolBar();
    void onMainFrameConstructed();
    void onFavouritesChanged();
    void reloadFavourites();
    void setupCategories();
    void constructPopupMenu();
    void clearItems();

    void onRemoveFromFavourite();
    void togglePage(const cmd::ArgumentList& args);
    void onCategoryToggled(wxCommandEvent& ev);
    void onShowFullPathToggled(wxCommandEvent& ev);
    void onListCtrlPaint(wxPaintEvent& ev);
    void onContextMenu(wxContextMenuEvent& ev);
    void onItemActivated(wxListEvent& ev);

    // Returns the list of all selected item indices
    std::vector<long> getSelectedItems();
    decl::Type getSelectedDeclType();

    void onApplyToSelection();
    bool testSingleTextureSelected();
    void onCreateEntity();
    bool testCreateEntity();
};

}
