#pragma once

#include "imousetool.h"
#include "selection/shaderclipboard/ShaderClipboard.h"

namespace ui
{

namespace
{
    const char* const RKEY_SELECT_EPSILON = "user/ui/selectionEpsilon";
}

class PickTextureTool :
    public MouseTool
{
public:
    const std::string& getName()
    {
        static std::string name("PickTextureTool");
        return name;
    }

    Result onMouseDown(Event& ev)
    {
        try
        {
            CameraMouseToolEvent& camEvent = dynamic_cast<CameraMouseToolEvent&>(ev);

            SelectionTestPtr selectionTest = camEvent.getView().createSelectionTestForPoint(ev.getDevicePosition());

            // Set the source texturable from the given test
            GlobalShaderClipboard().setSource(*selectionTest);

            return Result::Finished;
        }
        catch (std::bad_cast&)
        {
        }

        return Result::Ignored; // not handled
    }

    Result onMouseMove(Event& ev)
    {
        try
        {
            return Result::Ignored;
        }
        catch (std::bad_cast&)
        {
        }

        return Result::Ignored;
    }

    Result onMouseUp(Event& ev)
    {
        try
        {
            return Result::Finished;
        }
        catch (std::bad_cast&)
        {
        }

        return Result::Ignored;
    }
};

}
