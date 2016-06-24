#pragma once

#include "imousetool.h"
#include "i18n.h"
#include "../GlobalCamera.h"
#include "../CameraSettings.h"

namespace ui
{

class FreeMoveTool :
    public MouseTool
{
public:
    const std::string& getName() override
    {
        static std::string name("FreeMoveTool");
        return name;
    }

    const std::string& getDisplayName() override
    {
        static std::string displayName(_("Freemove Mode"));
        return displayName;
    }

    unsigned int getPointerMode() override
    {
        return PointerMode::Capture | PointerMode::Freeze | 
               PointerMode::Hidden | PointerMode::MotionDeltas;
    }

    Result onMouseDown(Event& ev) override
    {
        try
        {
            CameraMouseToolEvent& camEvent = dynamic_cast<CameraMouseToolEvent&>(ev);

            if (getCameraSettings()->toggleFreelook())
            {
                // Invert the current freelook status in toggle mode
                if (!camEvent.getView().freeMoveEnabled())
                {
                    camEvent.getView().enableFreeMove();
                    return Result::Activated; // stay active
                }
                else
                {
                    camEvent.getView().disableFreeMove();
                    return Result::Finished;
                }
            }
            
            // Non-toggle mode, just check if we can activate freelook
            if (!camEvent.getView().freeMoveEnabled())
            {
                camEvent.getView().enableFreeMove();
            }

            return Result::Activated; // we might already be in freelook mode, so let's report activated in all cases
        }
        catch (std::bad_cast&)
        {
        }

        return Result::Ignored; // not handled
    }

    Result onMouseMove(Event& ev) override
    {
        return Result::Ignored;
    }

    Result onMouseUp(Event& ev) override
    {
        try
        {
            CameraMouseToolEvent& camEvent = dynamic_cast<CameraMouseToolEvent&>(ev);

            // MouseUp events are ignored when in toggle mode
            // In non-toggle mode, we just reset the freelook status to disabled
            if (!getCameraSettings()->toggleFreelook() && camEvent.getView().freeMoveEnabled())
            {
                camEvent.getView().disableFreeMove();
                return Result::Finished;
            }

            return Result::Ignored; // all other cases ignore the event
        }
        catch (std::bad_cast&)
        {
        }

        return Result::Ignored;
    }

    // End the freemove mode if the capture is lost
    void onMouseCaptureLost(IInteractiveView& view) override
    {
        try
        {
            ICameraView& camView = dynamic_cast<ICameraView&>(view);

            if (camView.freeMoveEnabled())
            {
                camView.disableFreeMove();
            }
        }
        catch (std::bad_cast&)
        {
        }
    }

    // The FreeMove tool can not be canceled by ESC
    // More likely users are intending to de-select objects
    Result onCancel(IInteractiveView& view) override
    {
        return Result::Ignored;
    }
};

}
