#pragma once

#include "i18n.h"
#include "ui/iusercontrol.h"
#include "AasVisualisationPanel.h"

namespace ui
{

class AasVisualisationControl :
    public IUserControl
{
public:
    std::string getControlName() override
    {
        return UserControl::AasVisualisationPanel;
    }

    std::string getDisplayName() override
    {
        return _("AAS Area Viewer");
    }

    std::string getIcon() override
    {
        return {};
    }

    wxWindow* createWidget(wxWindow* parent) override
    {
        return new AasVisualisationPanel(parent);
    }
};

}
