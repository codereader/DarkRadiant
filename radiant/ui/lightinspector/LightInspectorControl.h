#pragma once

#include "i18n.h"
#include "ui/iusercontrol.h"
#include "LightInspector.h"

namespace ui
{

class LightInspectorControl :
    public IUserControl
{
public:
    std::string getControlName() override
    {
        return UserControl::LightInspector;
    }

    std::string getDisplayName() override
    {
        return _("Light Inspector");
    }

    std::string getIcon() override
    {
        return "bulb_lit.png";
    }

    wxWindow* createWidget(wxWindow* parent) override
    {
        return new LightInspector(parent);
    }
};

}
