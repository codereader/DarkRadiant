#pragma once

#include "i18n.h"
#include "ui/iusercontrol.h"
#include "AIEditingPanel.h"

namespace ui
{

class AIEditingControl :
    public IUserControl
{
public:
    constexpr static const char* const Name = "AIEditingPanel";

    std::string getControlName() override
    {
        return Name;
    }

    std::string getDisplayName() override
    {
        return _("AI");
    }

    std::string getIcon() override
    {
        return "icon_ai.png";
    }

    wxWindow* createWidget(wxWindow* parent) override
    {
        return new AIEditingPanel(parent);
    }
};

}
