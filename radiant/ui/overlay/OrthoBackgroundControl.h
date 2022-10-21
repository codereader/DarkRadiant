#pragma once

#include "i18n.h"
#include "ui/iusercontrol.h"
#include "OrthoBackgroundPanel.h"

namespace ui
{

class OrthoBackgroundControl :
    public IUserControl
{
public:
    std::string getControlName() override
    {
        return UserControl::OrthoBackgroundPanel;
    }

    std::string getDisplayName() override
    {
        return _("XY Background Image");
    }

    std::string getIcon() override
    {
        return {};
    }

    wxWindow* createWidget(wxWindow* parent) override
    {
        return new OrthoBackgroundPanel(parent);
    }
};

}
