#pragma once

#include "imousetool.h"
#include "selection/shaderclipboard/ShaderClipboard.h"

namespace ui
{

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
            // Set the source texturable from the given test
            //GlobalShaderClipboard().setSource(volume);

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
