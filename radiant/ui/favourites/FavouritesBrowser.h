#pragma once

#include "imodule.h"
#include "icommandsystem.h"
#include "idecltypes.h"
#include <wx/listctrl.h>
#include <wx/imaglist.h>
#include <wx/toolbar.h>
#include <sigc++/connection.h>

namespace ui
{

class FavouritesBrowser :
    public RegisterableModule
{
private:
    wxFrame* _tempParent;
    wxWindow* _mainWidget;
    wxListCtrl* _listView;

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

    wxCheckBox* _showFullPath;
    bool _updateNeeded;

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

    void togglePage(const cmd::ArgumentList& args);
    void onCategoryToggled(wxCommandEvent& ev);
    void onShowFullPathToggled(wxCommandEvent& ev);
    void onListCtrlPaint(wxPaintEvent& ev);
};

}
