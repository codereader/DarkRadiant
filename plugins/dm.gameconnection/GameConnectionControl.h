#pragma once

#include "i18n.h"
#include "ui/iusercontrol.h"
#include "GameConnectionPanel.h"

namespace ui
{

class GameConnectionControl :
    public IUserControl
{
public:
    constexpr static const char* Name = "GameConnectionPanel";

    std::string getControlName() override
    {
        return Name;
    }

    std::string getDisplayName() override
    {
        return _("Game Connection");
    }

    std::string getIcon() override
    {
        return {};
    }

    wxWindow* createWidget(wxWindow* parent) override
    {
        return new GameConnectionPanel(parent);
    }
};

}
