#pragma once

#include "i18n.h"
#include "ui/iusercontrol.h"
#include "TexTool.h"

namespace ui
{

class TextureToolControl :
    public IUserControl
{
public:
    std::string getControlName() override
    {
        return UserControl::TextureTool;
    }

    std::string getDisplayName() override
    {
        return _("Texture Tool");
    }

    std::string getIcon() override
    {
        return {};
    }

    wxWindow* createWidget(wxWindow* parent) override
    {
        return new TexTool(parent);
    }
};

}
