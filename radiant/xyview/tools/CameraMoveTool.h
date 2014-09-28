#pragma once

#include "imousetool.h"
#include "iorthoview.h"
#include "camera/GlobalCamera.h"

namespace ui
{

/**
 * The CameraMoveTool translates the camera to the
 * world position that is clicked on by the mouse.
 */
class CameraMoveTool :
    public MouseTool
{
public:
    const std::string& getName()
    {
        static std::string name("CameraMoveTool");
        return name;
    }

    Result onMouseDown(Event& ev)
    {
        try
        {
            positionCamera(dynamic_cast<XYMouseToolEvent&>(ev));

            return Result::Activated;
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
            positionCamera(dynamic_cast<XYMouseToolEvent&>(ev));

            return Result::Continued;
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
            // We only operate on XY view events, so attempt to cast
            dynamic_cast<XYMouseToolEvent&>(ev);
            return Result::Finished;
        }
        catch (std::bad_cast&)
        {
        }

        return Result::Ignored;
    }

private:
    void positionCamera(XYMouseToolEvent& xyEvent)
    {
        CamWndPtr camwnd = GlobalCamera().getActiveCamWnd();

        if (!camwnd)
        {
            return;
        }

        Vector3 origin = xyEvent.getWorldPos();

        switch (xyEvent.getViewType())
        {
        case XY:
            origin[2] = camwnd->getCameraOrigin()[2];
            break;
        case YZ:
            origin[0] = camwnd->getCameraOrigin()[0];
            break;
        case XZ:
            origin[1] = camwnd->getCameraOrigin()[1];
            break;
        };

        xyEvent.getView().snapToGrid(origin);
        camwnd->setCameraOrigin(origin);

        xyEvent.getView().queueDraw();
    }
};

}
