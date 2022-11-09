#pragma once

#include "i18n.h"
#include "ui/iusercontrol.h"
#include "PatchInspector.h"

namespace ui
{

class PatchInspectorControl :
    public IUserControl
{
public:
    std::string getControlName() override
    {
        return UserControl::PatchInspector;
    }

    std::string getDisplayName() override
    {
        return _("Patch Inspector");
    }

    std::string getIcon() override
    {
        return {};
    }

    wxWindow* createWidget(wxWindow* parent) override
    {
        return new PatchInspector(parent);
    }
};

}
