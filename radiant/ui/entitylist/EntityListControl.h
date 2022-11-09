#pragma once

#include "i18n.h"
#include "ui/iusercontrol.h"
#include "EntityList.h"

namespace ui
{

class EntityListControl :
    public IUserControl
{
public:
    std::string getControlName() override
    {
        return UserControl::EntityList;
    }

    std::string getDisplayName() override
    {
        return _("Entity List");
    }

    std::string getIcon() override
    {
        return "cmenu_add_entity.png";
    }

    wxWindow* createWidget(wxWindow* parent) override
    {
        return new EntityList(parent);
    }
};

}
