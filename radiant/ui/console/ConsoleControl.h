#pragma once

#include "ui/iusercontrol.h"
#include "Console.h"

namespace ui
{

class ConsoleControl :
    public IUserControl
{
public:
    std::string getControlName() override
    {
        return UserControl::Console;
    }

    std::string getDisplayName() override
    {
        return _("Console");
    }

    std::string getIcon() override
    {
        return "iconConsole16.png";
    }

    wxWindow* createWidget(wxWindow* parent) override
    {
        return new Console(parent);
    }
};

}
