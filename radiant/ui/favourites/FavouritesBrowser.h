#pragma once

#include "imodule.h"
#include "idecltypes.h"
#include <wx/listctrl.h>
#include <wx/imaglist.h>

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

    // Maps decl type to icon index
    std::map<decl::Type, int> _favouritesToList;

public:
    FavouritesBrowser();

    const std::string& getName() const override;
    const StringSet& getDependencies() const override;
    void initialiseModule(const IApplicationContext& ctx) override;
    void shutdownModule() override;

private:
    void construct();
    void onMainFrameConstructed();
    void reloadFavourites();
};

}
