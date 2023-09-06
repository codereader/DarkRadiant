#pragma once

#include "XYMouseToolEvent.h"
#include "i18n.h"
#include "imousetool.h"
#include "iorthoview.h"
#include "icameraview.h"
#include "selectionlib.h"

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
    const std::string& getName() override
    {
        static std::string name("CameraMoveTool");
        return name;
    }

    const std::string& getDisplayName() override
    {
        static std::string displayName(_("Drag Camera"));
        return displayName;
    }

    Result onMouseDown(Event& ev) override
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

    Result onMouseMove(Event& ev) override
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

    Result onMouseUp(Event& ev) override
    {
        try
        {
            // We only operate on XY view events, so attempt to cast
            dynamic_cast<XYMouseToolEvent&>(ev).getScale();
            return Result::Finished;
        }
        catch (std::bad_cast&)
        {
        }

        return Result::Ignored;
    }

    unsigned int getRefreshMode() override
    {
        return RefreshMode::Force | RefreshMode::AllViews; // update cam view too
    }

private:
    void positionCamera(XYMouseToolEvent& xyEvent)
    {
        try
        {
            auto& camera = GlobalCameraManager().getActiveView();

            Vector3 origin = xyEvent.getWorldPos();

            // Substitute the third component from the selection work zone
            auto workzoneOrigin = GlobalSelectionSystem().getWorkZone().bounds.getOrigin();

            switch (xyEvent.getViewType())
            {
            case OrthoOrientation::XY:
                origin[2] = workzoneOrigin[2];
                break;
            case OrthoOrientation::YZ:
                origin[0] = workzoneOrigin[0];
                break;
            case OrthoOrientation::XZ:
                origin[1] = workzoneOrigin[1];
                break;
            };

            xyEvent.getView().snapToGrid(origin);
            camera.setCameraOrigin(origin);

            xyEvent.getView().queueDraw();
        }
        catch (const std::runtime_error&)
        {
            // no camera present
        }
    }
};

}
