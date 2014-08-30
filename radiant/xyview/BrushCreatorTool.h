#pragma once

#include "MouseTool.h"
#include "iundo.h"
#include "iselection.h"
#include "igrid.h"
#include "scenelib.h"
#include "itextstream.h"
#include "selection/algorithm/Primitives.h"
#include "ui/texturebrowser/TextureBrowser.h"

namespace ui
{

class BrushCreatorTool :
    public MouseTool
{
private:
    scene::INodePtr _brush;
    Vector3 _startPos;

public:
    const std::string& getName()
    {
        static std::string name("BrushCreatorTool");
        return name;
    }

    bool onMouseDown(Event& ev)
    {
        if (GlobalSelectionSystem().countSelected() == 0)
        {
            _brush.reset();
            _startPos = ev.getWorldPos();
            GlobalUndoSystem().start();

            return true;
        }

        return false; // not handled
    }

    bool onMouseMove(Event& ev)
    {
        Vector3 startPos = _startPos;
        snapToGrid(startPos, ev.getViewType());

        Vector3 endPos = ev.getWorldPos();
        snapToGrid(endPos, ev.getViewType());
        
        int nDim = (ev.getViewType() == Event::ViewType::XY) ? 2 : (ev.getViewType() == Event::ViewType::YZ) ? 0 : 1;

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

        return true;
    }

    bool onMouseUp(Event& ev)
    {
        GlobalUndoSystem().finish("brushDragNew");
        return true; // handled anyway
    }

private:
    void snapToGrid(Vector3& point, Event::ViewType viewType)
    {
        if (viewType == Event::ViewType::XY)
        {
            point[0] = float_snapped(point[0], GlobalGrid().getGridSize());
            point[1] = float_snapped(point[1], GlobalGrid().getGridSize());
        }
        else if (viewType == Event::ViewType::YZ)
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

} // namespace
