#include "BrushCreatorTool.h"

#include "i18n.h"
#include "igrid.h"
#include "iclipper.h"
#include "scenelib.h"
#include "selectionlib.h"
#include "command/ExecutionNotPossible.h"
#include "XYMouseToolEvent.h"
#include "ui/texturebrowser/TextureBrowserPanel.h"
#include "ui/texturebrowser/TextureBrowserManager.h"

namespace ui
{

const std::string& BrushCreatorTool::getName()
{
    static std::string name("BrushCreatorTool");
    return name;
}

const std::string& BrushCreatorTool::getDisplayName()
{
    static std::string displayName(_("Drag-create Brush"));
    return displayName;
}

MouseTool::Result BrushCreatorTool::onMouseDown(Event& ev)
{
    try
    {
        if (GlobalClipper().clipMode() || GlobalMapModule().getEditMode() == IMap::EditMode::Merge)
        {
            return Result::Ignored; // no brush creation in clip or merge mode
        }

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

MouseTool::Result BrushCreatorTool::onMouseMove(Event& ev)
{
    try
    {
        // We only operate on XY view events, so attempt to cast
        XYMouseToolEvent& xyEvent = dynamic_cast<XYMouseToolEvent&>(ev);

        Vector3 startPos = _startPos;
        xyEvent.getView().snapToGrid(startPos);

        Vector3 endPos = xyEvent.getWorldPos();
        xyEvent.getView().snapToGrid(endPos);

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
                // Brush was created
                // Insert the brush into worldspawn
                auto worldspawn = GlobalMapModule().findOrInsertWorldspawn();
                scene::addNodeToContainer(_brush, worldspawn);
            }

            // Make sure the brush is selected
            Node_setSelected(_brush, true);
        }

        // It's possible that the brush we created has been deleted in the meantime
        // check the brush object for validity
        if (!_brush->getParent() || GlobalSelectionSystem().countSelected() == 0)
        {
            _brush.reset();
            return Result::Finished;
        }

        // Dispatch the command
        GlobalCommandSystem().executeCommand(
            "ResizeSelectedBrushesToBounds",
            {startPos, endPos, GlobalTextureBrowser().getSelectedShader()}
        );
    }
    catch (cmd::ExecutionNotPossible&)
    {
        return Result::Ignored;
    }
    catch (std::bad_cast&)
    {
        return Result::Ignored;
    }

    return Result::Continued;
}

MouseTool::Result BrushCreatorTool::onMouseUp(Event& ev)
{
    try
    {
        // We only operate on XY view events, so attempt to cast
        dynamic_cast<XYMouseToolEvent&>(ev).getScale();

        GlobalUndoSystem().finish("brushDragNew");
        _brush.reset();

        return Result::Finished;
    }
    catch (std::bad_cast&)
    {
        return Result::Ignored;
    }
}

BrushCreatorTool::Result BrushCreatorTool::onCancel(IInteractiveView& view)
{
    if (_brush)
    {
        // We have a WIP brush object, kill it
        scene::removeNodeFromParent(_brush);
        GlobalUndoSystem().cancel();

        _brush.reset();
    }

    return Result::Finished;
}

void BrushCreatorTool::onMouseCaptureLost(IInteractiveView& view)
{
    onCancel(view);
}

unsigned int BrushCreatorTool::getPointerMode()
{
    return PointerMode::Capture;
}

unsigned int BrushCreatorTool::getRefreshMode()
{
    return RefreshMode::Force | RefreshMode::AllViews; // update cam view too
}

} // namespace
