#pragma once

#include "i18n.h"
#include "math/Vector2.h"
#include "selection/SelectionMouseTools.h"

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

    virtual void testSelect(Event& ev) override
    {
        int i = 6;
    }
};

}
