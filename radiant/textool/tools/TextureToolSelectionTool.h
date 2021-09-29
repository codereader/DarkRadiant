#pragma once

#include "i18n.h"
#include "itexturetoolmodel.h"
#include "selection/SelectionMouseTools.h"
#include "selection/SelectionVolume.h"

namespace ui
{

class TextureToolSelectionTool :
    public BasicSelectionTool
{
public:
    virtual const std::string& getName() override
    {
        static std::string name("TextureToolSelectionTool");
        return name;
    }

    virtual const std::string& getDisplayName() override
    {
        static std::string displayName(_("Select"));
        return displayName;
    }

    virtual void performSelectionTest(SelectionVolume& volume, SelectionType type, MouseTool::Event& ev) override
    {
        if (type == SelectionType::Area)
        {
            GlobalTextureToolSelectionSystem().selectArea(volume, selection::SelectionSystem::eToggle);
        }
        else
        {
            GlobalTextureToolSelectionSystem().selectPoint(volume, selection::SelectionSystem::eToggle);
        }
    }
};

}
