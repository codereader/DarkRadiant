#pragma once

#include "i18n.h"
#include "ui/iusercontrol.h"
#include "FindShader.h"

namespace ui
{

class FindShaderControl :
    public IUserControl
{
public:
    std::string getControlName() override
    {
        return UserControl::FindAndReplaceMaterial;
    }

    std::string getDisplayName() override
    {
        return _("Find and Replace Material");
    }

    std::string getIcon() override
    {
        return "texwindow_findandreplace.png";
    }

    wxWindow* createWidget(wxWindow* parent) override
    {
        return new FindAndReplaceShader(parent);
    }
};

}
