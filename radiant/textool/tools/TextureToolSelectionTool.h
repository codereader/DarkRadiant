#pragma once

#include "i18n.h"
#include "math/Vector2.h"
#include "selection/SelectionMouseTools.h"
#include "selection/SelectionVolume.h"

namespace ui
{

class TextureToolSelectionTool :
    public BasicSelectionTool
{
private:
    Vector2 _start;		// Position at mouseDown
    Vector2 _current;	// Position during mouseMove

public:
    TextureToolSelectionTool()
    {}

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
        auto mouseEvent = dynamic_cast<TextureToolMouseEvent*>(&ev);
        if (mouseEvent == nullptr) return;

        mouseEvent->getView().testSelect(volume);
    }
};

}
