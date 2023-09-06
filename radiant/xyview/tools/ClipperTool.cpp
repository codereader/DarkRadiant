#include "ClipperTool.h"

#include "i18n.h"
#include "igrid.h"
#include "iclipper.h"
#include "iselection.h"
#include "ui/imainframe.h"
#include "XYMouseToolEvent.h"

namespace ui
{

ClipperTool::ClipperTool() :
    _crossHairEnabled(false)
{}

const std::string& ClipperTool::getName()
{
    static std::string name("ClipperTool");
    return name;
}

const std::string& ClipperTool::getDisplayName()
{
    static std::string displayName(_("Clipper"));
    return displayName;
}

MouseTool::Result ClipperTool::onMouseDown(Event& ev)
{
    try
    {
        // We only operate on XY view events, so attempt to cast
        XYMouseToolEvent& xyEvent = dynamic_cast<XYMouseToolEvent&>(ev);

        // There are two possibilites for the "select" click: Clip or Select
        if (GlobalClipper().clipMode())
        {
            ClipPoint* foundClipPoint = GlobalClipper().find(
                xyEvent.getWorldPos(), xyEvent.getViewType(), xyEvent.getScale());

            GlobalClipper().setMovingClip(foundClipPoint);

            if (foundClipPoint == nullptr)
            {
                dropClipPoint(xyEvent);
            }

            return Result::Activated;
        }
    }
    catch (std::bad_cast&)
    {
    }

    return Result::Ignored; // not handled
}

MouseTool::Result ClipperTool::onMouseMove(Event& ev)
{
    try
    {
        // We only operate on XY view events, so attempt to cast
        XYMouseToolEvent& xyEvent = dynamic_cast<XYMouseToolEvent&>(ev);

        if (!GlobalClipper().clipMode())
        {
            // Check if we need to clean up the XY cursor state
            if (_crossHairEnabled)
            {
                xyEvent.getView().setCursorType(IOrthoView::CursorType::Default);
                _crossHairEnabled = false;
            }

            return Result::Ignored;
        }

        // Check, if we have a clip point operation running
        if (GlobalClipper().getMovingClip() != nullptr)
        {
            // Leave the third coordinate of the clip point untouched (#5356)
            auto viewType = xyEvent.getViewType();
            int missingDim = viewType == OrthoOrientation::XY ? 2 : viewType == OrthoOrientation::YZ ? 0 : 1;
            auto& clipCoords = GlobalClipper().getMovingClipCoords();

            Vector3 newWorldPos = xyEvent.getWorldPos();
            newWorldPos[missingDim] = clipCoords[missingDim];

            xyEvent.getView().snapToGrid(newWorldPos);

            GlobalClipper().getMovingClipCoords() = newWorldPos;
            GlobalClipper().update();

            GlobalMainFrame().updateAllWindows();

            return Result::Continued;
        }

        // Check the point below the cursor to detect manipulatable clip points
        if (GlobalClipper().find(xyEvent.getWorldPos(), xyEvent.getViewType(), xyEvent.getScale()) != NULL)
        {
            xyEvent.getView().setCursorType(IOrthoView::CursorType::Crosshair);
            // Remember to clean up the cursor when clip mode is deactivated
            _crossHairEnabled = true;
            return Result::Continued;
        }
        else
        {
            xyEvent.getView().setCursorType(IOrthoView::CursorType::Default);
            _crossHairEnabled = false;
            return Result::Continued;
        }
    }
    catch (std::bad_cast&)
    {
    }

    return Result::Ignored;
}

MouseTool::Result ClipperTool::onMouseUp(Event& ev)
{
    try
    {
        // We only operate on XY view events, so attempt to cast
        dynamic_cast<XYMouseToolEvent&>(ev).getScale();

        if (GlobalClipper().clipMode())
        {
            GlobalClipper().setMovingClip(NULL);
            return Result::Finished;
        }
    }
    catch (std::bad_cast&)
    {
    }

    return Result::Ignored;
}

bool ClipperTool::alwaysReceivesMoveEvents()
{
    return true;
}

void ClipperTool::dropClipPoint(XYMouseToolEvent& event)
{
    Vector3 point = event.getWorldPos();
    Vector3 mid = GlobalSelectionSystem().getCurrentSelectionCenter();

    GlobalClipper().setViewType(static_cast<OrthoOrientation>(event.getViewType()));
    int nDim = (GlobalClipper().getViewType() == OrthoOrientation::YZ) ? 0 : ((GlobalClipper().getViewType() == OrthoOrientation::XZ) ? 1 : 2);
    point[nDim] = mid[nDim];
    point.snap(GlobalGrid().getGridSize());
    GlobalClipper().newClipPoint(point);
}

} // namespace
