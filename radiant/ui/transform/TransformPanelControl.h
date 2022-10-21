#pragma once

#include "i18n.h"
#include "ui/iusercontrol.h"
#include "TransformPanel.h"

namespace ui
{

class TransformPanelControl :
    public IUserControl
{
public:
    std::string getControlName() override
    {
        return UserControl::TransformPanel;
    }

    std::string getDisplayName() override
    {
        return _("Transformation");
    }

    std::string getIcon() override
    {
        return {};
    }

    wxWindow* createWidget(wxWindow* parent) override
    {
        return new TransformPanel(parent);
    }
};

}
