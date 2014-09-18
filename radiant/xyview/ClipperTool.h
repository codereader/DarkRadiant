#pragma once

#include "MouseTool.h"
#include "iclipper.h"
#include "selection/algorithm/General.h"

namespace ui
{

class ClipperTool :
    public MouseTool
{
public:
    const std::string& getName()
    {
        static std::string name("ClipperTool");
        return name;
    }

    Result onMouseDown(Event& ev)
    {
        try
        {
            // We only operate on XY view events, so attempt to cast
            XYMouseToolEvent& xyEvent = dynamic_cast<XYMouseToolEvent&>(ev);

            // There are two possibilites for the "select" click: Clip or Select
            if (GlobalClipper().clipMode())
            {
                ClipPoint* foundClipPoint = GlobalClipper().find(xyEvent.getWorldPos(), xyEvent.getViewType(), xyEvent.getScale());

                GlobalClipper().setMovingClip(foundClipPoint);

                if (foundClipPoint == NULL)
                {
                    dropClipPoint(xyEvent);
                }

                return Result::Activated;
            }
        }
        catch (std::bad_cast&)
        {}

        return Result::Ignored; // not handled
    }

    Result onMouseMove(Event& ev)
    {
        try
        {
            // We only operate on XY view events, so attempt to cast
            XYMouseToolEvent& xyEvent = dynamic_cast<XYMouseToolEvent&>(ev);

            if (!GlobalClipper().clipMode())
            {
                return Result::Ignored;
            }

            // Check, if we have a clip point operation running
            if (GlobalClipper().getMovingClip() != NULL)
            {
                GlobalClipper().getMovingClipCoords() = xyEvent.getWorldPos();
                snapToGrid(GlobalClipper().getMovingClipCoords(), xyEvent.getViewType());
                GlobalClipper().update();

                GlobalMainFrame().updateAllWindows();

                return Result::Continued;
            }

            // Check the point below the cursor to detect manipulatable clip points
            if (GlobalClipper().find(xyEvent.getWorldPos(), xyEvent.getViewType(), xyEvent.getScale()) != NULL)
            {
                xyEvent.getView().setCursorType(IOrthoView::CursorType::Crosshair);
                return Result::Continued;
            }
            else
            {
                xyEvent.getView().setCursorType(IOrthoView::CursorType::Default);
                return Result::Continued;
            }
        }
        catch (std::bad_cast&)
        {}

        return Result::Ignored;
    }

    Result onMouseUp(Event& ev)
    {
        try
        {
            // We only operate on XY view events, so attempt to cast
            dynamic_cast<XYMouseToolEvent&>(ev);

            if (GlobalClipper().clipMode())
            {
                GlobalClipper().setMovingClip(NULL);
                return Result::Finished;
            }
        }
        catch (std::bad_cast&)
        {}

        return Result::Ignored;
    }

    bool alwaysReceivesMoveEvents()
    {
        return true;
    }

private:
    void snapToGrid(Vector3& point, EViewType viewType)
    {
        if (viewType == XY)
        {
            point[0] = float_snapped(point[0], GlobalGrid().getGridSize());
            point[1] = float_snapped(point[1], GlobalGrid().getGridSize());
        }
        else if (viewType == YZ)
        {
            point[1] = float_snapped(point[1], GlobalGrid().getGridSize());
            point[2] = float_snapped(point[2], GlobalGrid().getGridSize());
        }
        else
        {
            point[0] = float_snapped(point[0], GlobalGrid().getGridSize());
            point[2] = float_snapped(point[2], GlobalGrid().getGridSize());
        }
    }

    void dropClipPoint(XYMouseToolEvent& event)
    {
        Vector3 point = event.getWorldPos();
        Vector3 mid = selection::algorithm::getCurrentSelectionCenter();

        GlobalClipper().setViewType(static_cast<EViewType>(event.getViewType()));
        int nDim = (GlobalClipper().getViewType() == YZ) ? 0 : ((GlobalClipper().getViewType() == XZ) ? 1 : 2);
        point[nDim] = mid[nDim];
        point.snap(GlobalGrid().getGridSize());
        GlobalClipper().newClipPoint(point);
    }
};

} // namespace
