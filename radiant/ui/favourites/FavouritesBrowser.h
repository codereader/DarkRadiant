#pragma once

#include "imodule.h"
#include "icommandsystem.h"
#include "idecltypes.h"
#include <wx/listctrl.h>
#include <wx/imaglist.h>
#include <wx/toolbar.h>

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

public:
    FavouritesBrowser();

    const std::string& getName() const override;
    const StringSet& getDependencies() const override;
    void initialiseModule(const IApplicationContext& ctx) override;
    void shutdownModule() override;

private:
    void construct();
    wxToolBar* createToolBar();
    void onMainFrameConstructed();
    void reloadFavourites();

    void togglePage(const cmd::ArgumentList& args);
    void onCategoryToggled(wxCommandEvent& ev);
};

}
