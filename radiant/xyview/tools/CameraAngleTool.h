#pragma once

#include "XYMouseToolEvent.h"
#include "i18n.h"
#include "imousetool.h"
#include "math/Vector3.h"
#include "iorthoview.h"
#include "icameraview.h"

namespace ui
{

/**
* The CameraAngleTool re-orients the camera such that
* the clicked location is in its view.
*/
class CameraAngleTool :
    public MouseTool
{
public:
    const std::string& getName() override
    {
        static std::string name("CameraAngleTool");
        return name;
    }

    const std::string& getDisplayName() override
    {
        static std::string displayName(_("Point Camera"));
        return displayName;
    }

    Result onMouseDown(Event& ev) override
    {
        try
        {
            // Set the camera angle
            orientCamera(dynamic_cast<XYMouseToolEvent&>(ev));

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
            orientCamera(dynamic_cast<XYMouseToolEvent&>(ev));

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
    void orientCamera(XYMouseToolEvent& xyEvent)
    {
        try
        {
            auto& camView = GlobalCameraManager().getActiveView();

            Vector3 point = xyEvent.getWorldPos();
            xyEvent.getView().snapToGrid(point);

            point -= camView.getCameraOrigin();

            int n1 = (xyEvent.getViewType() == OrthoOrientation::XY) ? 1 : 2;
            int n2 = (xyEvent.getViewType() == OrthoOrientation::YZ) ? 1 : 0;

            int nAngle = (xyEvent.getViewType() == OrthoOrientation::XY) ? camera::CAMERA_YAW : camera::CAMERA_PITCH;

            if (point[n1] || point[n2])
            {
                Vector3 angles(camView.getCameraAngles());

                angles[nAngle] = radians_to_degrees(atan2(point[n1], point[n2]));

                camView.setCameraAngles(angles);
            }

            xyEvent.getView().queueDraw();
        }
        catch (const std::runtime_error&)
        {
            // no camera present
        }
    }
};

} // namespace
