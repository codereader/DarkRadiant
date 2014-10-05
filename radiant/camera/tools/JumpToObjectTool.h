#pragma once

#include "imousetool.h"
#include "../GlobalCamera.h"

namespace ui
{

class JumpToObjectTool :
    public MouseTool
{
public:
    const std::string& getName()
    {
        static std::string name("JumpToObjectTool");
        return name;
    }

    Result onMouseDown(Event& ev)
    {
        try
        {
            CameraMouseToolEvent& camEvent = dynamic_cast<CameraMouseToolEvent&>(ev);

            SelectionTestPtr selectionTest = camEvent.getView().createSelectionTestForPoint(camEvent.getDevicePosition());

            CamWndPtr cam = GlobalCamera().getActiveCamWnd();

            if (cam != NULL)
            {
                cam->jumpToObject(*selectionTest);
            }

            return Result::Finished;
        }
        catch (std::bad_cast&)
        {
        }

        return Result::Ignored; // not handled

    }

    Result onMouseMove(Event& ev)
    {
        return Result::Finished;
    }

    Result onMouseUp(Event& ev)
    {
        return Result::Finished;
    }
};

} // namespace
