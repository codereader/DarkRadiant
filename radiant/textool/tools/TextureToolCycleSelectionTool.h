#pragma once

#include "i18n.h"
#include "math/Vector2.h"
#include "itexturetoolmodel.h"
#include "selection/SelectionMouseTools.h"
#include "selection/SelectionVolume.h"

namespace ui
{

/**
 * Specialisation of the regular Cycle Selection Mouse Tool,
 * aimed to cycle-select between items in the Texture Tool Scene
 */
class TextureToolCycleSelectionTool :
    public CycleSelectionMouseTool
{
public:
    virtual const std::string& getName() override
    {
        static std::string name("TextureToolCycleSelectionTool");
        return name;
    }

    virtual const std::string& getDisplayName() override
    {
        static std::string displayName(_("Cycle Select"));
        return displayName;
    }

    virtual void performPointSelection(SelectionVolume& volume, selection::SelectionSystem::EModifier modifier) override
    {
        GlobalTextureToolSelectionSystem().selectPoint(volume, modifier);
    }
};

}
