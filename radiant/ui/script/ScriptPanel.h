#pragma once

#include "i18n.h"
#include "ui/iusercontrol.h"
#include "ScriptWindow.h"

namespace ui
{

class ScriptPanel :
    public IUserControl
{
public:
    constexpr static const char* const Name = "ScriptPanel";

    std::string getControlName() override
    {
        return Name;
    }

    std::string getDisplayName() override
    {
        return _("Script");
    }

    std::string getIcon() override
    {
        return "icon_script.png";
    }

    wxWindow* createWidget(wxWindow* parent) override
    {
        return new ScriptWindow(parent);
    }
};

}
