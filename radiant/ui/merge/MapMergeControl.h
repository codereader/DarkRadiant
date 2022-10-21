#pragma once

#include "i18n.h"
#include "ui/iusercontrol.h"
#include "MapMergePanel.h"

namespace ui
{

class MapMergeControl :
    public IUserControl
{
public:
    std::string getControlName() override
    {
        return UserControl::MapMergePanel;
    }

    std::string getDisplayName() override
    {
        return _("Merge Map");
    }

    std::string getIcon() override
    {
        return {};
    }

    wxWindow* createWidget(wxWindow* parent) override
    {
        return new MapMergePanel(parent);
    }
};

}
