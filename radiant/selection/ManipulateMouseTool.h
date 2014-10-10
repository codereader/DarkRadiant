#pragma once

#include "imousetool.h"

namespace ui
{

/**
 * greebo: This is the tool handling the manipulation mouse operations, it basically just
 * passes all the mouse clicks back to the SelectionSystem, trying to select something 
 * that can be manipulated (patches, lights, drag-resizable objects, vertices,...)
 */
class ManipulateMouseTool :
    public MouseTool
{
private:
    

public:
    const std::string& getName()
    {
        static std::string name("ManipulateMouseTool");
        return name;
    }

    Result onMouseDown(Event& ev)
    {
        try
        {
            const Vector2& devPos = ev.getDevicePosition();
            DeviceVector devicePosition(window_to_normalised_device(position, _width, _height));

            // We only operate on XY view events, so attempt to cast
            XYMouseToolEvent& xyEvent = dynamic_cast<XYMouseToolEvent&>(ev);

            if (GlobalSelectionSystem().countSelected() == 0)
            {
                _brush.reset();
                _startPos = xyEvent.getWorldPos();
                GlobalUndoSystem().start();

                return Result::Activated;
            }
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
            // We only operate on XY view events, so attempt to cast
            XYMouseToolEvent& xyEvent = dynamic_cast<XYMouseToolEvent&>(ev);

            Vector3 startPos = _startPos;
            snapToGrid(startPos, xyEvent.getViewType());

            Vector3 endPos = xyEvent.getWorldPos();
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
                    return Result::Continued; // don't create a degenerate brush
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
            return Result::Ignored;
        }

        return Result::Continued;
    }

    Result onMouseUp(Event& ev)
    {
        try
        {
            // We only operate on XY view events, so attempt to cast
            dynamic_cast<XYMouseToolEvent&>(ev);

            GlobalUndoSystem().finish("brushDragNew");
            return Result::Finished;
        }
        catch (std::bad_cast&)
        {
            return Result::Ignored;
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
};


}
