#pragma once

#include "ui/iusercontrol.h"
#include "LayerControlPanel.h"

namespace ui
{

class LayerControl :
    public IUserControl
{
public:
    std::string getControlName() override
    {
        return UserControl::LayerControlPanel;
    }

    std::string getDisplayName() override
    {
        return _("Layers");
    }

    std::string getIcon() override
    {
        return "layers.png";
    }

    wxWindow* createWidget(wxWindow* parent) override
    {
        return new LayerControlPanel(parent);
    }
};

}
