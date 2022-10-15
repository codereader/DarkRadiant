#pragma once

#include "ui/iusercontrol.h"
#include "FavouritesBrowser.h"

namespace ui
{

class FavouritesBrowserControl :
    public IUserControl
{
public:
    std::string getControlName() override
    {
        return UserControl::FavouritesBrowser;
    }

    std::string getDisplayName() override
    {
        return _("Favourites");
    }

    std::string getIcon() override
    {
        return "favourite.png";
    }

    wxWindow* createWidget(wxWindow* parent) override
    {
        return new FavouritesBrowser(parent);
    }
};

}
