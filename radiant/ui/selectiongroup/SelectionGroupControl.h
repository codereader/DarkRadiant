#pragma once

#include "i18n.h"
#include "ui/iusercontrol.h"
#include "SelectionGroupPanel.h"

namespace ui
{

class SelectionGroupControl :
    public IUserControlCreator
{
public:
    std::string getControlName() override
    {
        return UserControl::SelectionGroupPanel;
    }

    std::string getDisplayName() override
    {
        return _("Selection Groups");
    }

    std::string getIcon() override
    {
        return "group_selection.png";
    }

    wxWindow* createWidget(wxWindow* parent) override
    {
        return new SelectionGroupPanel(parent);
    }
};

}
