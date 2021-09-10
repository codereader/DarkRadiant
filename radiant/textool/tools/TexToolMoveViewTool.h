#pragma once

#include "imousetool.h"
#include "TextureToolMouseEvent.h"

namespace ui
{

/**
* The TexToolMoveViewTool implements the drag view functionality used in
* the texture tool. The mouse pointer is captured during its active
* phase and only the move deltas are processed to translate the view origin.
*/
class TexToolMoveViewTool :
    public MouseTool
{
public:
    const std::string& getName() override
    {
        static std::string name("TexToolMoveViewTool");
        return name;
    }

    const std::string& getDisplayName() override
    {
        static std::string displayName(_("Drag View"));
        return displayName;
    }

    unsigned int getPointerMode() override
    {
        return PointerMode::Capture | PointerMode::Freeze | 
            PointerMode::Hidden | PointerMode::MotionDeltas;
    }

    Result onMouseDown(Event& ev) override
    {
        try
        {
            dynamic_cast<TextureToolMouseEvent&>(ev);
            return Result::Activated;
        }
        catch (std::bad_cast&)
        {
        }

        return Result::Ignored; // not handled
    }

    Result onMouseMove(Event& ev) override
    {
        try
        {
            // We use capture mode, so the event will contain the delta only
            auto& texToolEvent = dynamic_cast<TextureToolMouseEvent&>(ev);

            // Scroll the view
            texToolEvent.getView().scrollByPixels(texToolEvent.getDeviceDelta().x(), texToolEvent.getDeviceDelta().y());

            return Result::Continued;
        }
        catch (std::bad_cast&)
        {
        }

        return Result::Ignored;
    }

    Result onMouseUp(Event& ev) override
    {
        try
        {
            // We only operate on texture tool view events, so attempt to cast
            dynamic_cast<TextureToolMouseEvent&>(ev);
            return Result::Finished;
        }
        catch (std::bad_cast&)
        {
        }

        return Result::Ignored;
    }
};

}
