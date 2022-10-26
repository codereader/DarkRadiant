#pragma once

#include "i18n.h"
#include "ui/iusercontrol.h"
#include "SurfaceInspector.h"

namespace ui
{

class SurfaceInspectorControl :
    public IUserControl
{
public:
    std::string getControlName() override
    {
        return UserControl::SurfaceInspector;
    }

    std::string getDisplayName() override
    {
        return _("Surface Inspector");
    }

    std::string getIcon() override
    {
        return {};
    }

    wxWindow* createWidget(wxWindow* parent) override
    {
        return new SurfaceInspector(parent);
    }
};

}
