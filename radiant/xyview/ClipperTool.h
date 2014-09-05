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

    bool onMouseDown(Event& ev)
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
                    dropClipPoint(ev.getWorldPos());
                }

                return true;
            }
        }
        catch (std::bad_cast&)
        {}

        return false; // not handled
    }

    bool onMouseMove(Event& ev)
    {
        try
        {
            // We only operate on XY view events, so attempt to cast
            XYMouseToolEvent& xyEvent = dynamic_cast<XYMouseToolEvent&>(ev);

            Vector3 startPos = _startPos;
            snapToGrid(startPos, xyEvent.getViewType());

            Vector3 endPos = ev.getWorldPos();
            snapToGrid(endPos, xyEvent.getViewType());

            int nDim = (xyEvent.getViewType() == XY) ? 2 : (xyEvent.getViewType() == YZ) ? 0 : 1;

            const selection::WorkZone& wz = GlobalSelectionSystem().getWorkZone();

            startPos[nDim] = float_snapped(wz.min[nDim], GlobalGrid().getGridSize());
            endPos[nDim] = float_snapped(wz.max[nDim], GlobalGrid().getGridSize());

            if (endPos[nDim] <= startPos[nDim])
            {
                endPos[nDim] = startPos[nDim] + GlobalGrid().getGridSize();
            }

            for (int i = 0; i < 3; i++)
            {
                if (startPos[i] == endPos[i])
                {
                    return true; // don't create a degenerate brush
                }

                if (startPos[i] > endPos[i])
                {
                    std::swap(startPos[i], endPos[i]);
                }
            }

            if (!_brush)
            {
                // greebo: Create a new brush
                _brush = GlobalBrushCreator().createBrush();

                if (_brush)
                {
                    // Brush could be created

                    // Insert the brush into worldspawn
                    scene::INodePtr worldspawn = GlobalMap().findOrInsertWorldspawn();
                    scene::addNodeToContainer(_brush, worldspawn);
                }
            }

            // Make sure the brush is selected
            Node_setSelected(_brush, true);

            selection::algorithm::resizeBrushesToBounds(
                AABB::createFromMinMax(startPos, endPos),
                GlobalTextureBrowser().getSelectedShader()
                );
        }
        catch (std::bad_cast&)
        {
            return false;
        }

        return true;
    }

    bool onMouseUp(Event& ev)
    {
        try
        {
            // We only operate on XY view events, so attempt to cast
            dynamic_cast<XYMouseToolEvent&>(ev);

            GlobalUndoSystem().finish("brushDragNew");
            return true;
        }
        catch (std::bad_cast&)
        {
            return false;
        }
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

    void dropClipPoint(const Vector3& point)
    {
        Vector3 mid = selection::algorithm::getCurrentSelectionCenter();

        GlobalClipper().setViewType(static_cast<EViewType>(getViewType()));
        int nDim = (GlobalClipper().getViewType() == YZ) ? 0 : ((GlobalClipper().getViewType() == XZ) ? 1 : 2);
        point[nDim] = mid[nDim];
        point.snap(GlobalGrid().getGridSize());
        GlobalClipper().newClipPoint(point);
    }
};

} // namespace
