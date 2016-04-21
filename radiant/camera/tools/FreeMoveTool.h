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
        return getCameraSettings()->toggleFreelook() ? PointerMode::Normal : PointerMode::Capture;
    }

    Result onMouseDown(Event& ev) override
    {
        try
        {
            CameraMouseToolEvent& camEvent = dynamic_cast<CameraMouseToolEvent&>(ev);

            bool toggleMode = getCameraSettings()->toggleFreelook();

            if (!camEvent.getView().freeMoveEnabled())
            {
                camEvent.getView().enableFreeMove();
            }
            else if (toggleMode)
            {
                camEvent.getView().disableFreeMove();
            }

            // In non-toggle mode, we need to stay active
            return toggleMode ? Result::Finished : Result::Activated;
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

            bool toggleMode = getCameraSettings()->toggleFreelook();

            if (!toggleMode && camEvent.getView().freeMoveEnabled())
            {
                camEvent.getView().disableFreeMove();
            }

            return Result::Finished;
        }
        catch (std::bad_cast&)
        {
        }

        return Result::Ignored;
    }

    // End the freemove mode if the user hits cancel
    void onCancel(IInteractiveView& view) override
    {
        try
        {
            bool toggleMode = getCameraSettings()->toggleFreelook();

            ICameraView& camView = dynamic_cast<ICameraView&>(view);

            if (!toggleMode && camView.freeMoveEnabled())
            {
                camView.disableFreeMove();
            }
        }
        catch (std::bad_cast&)
        {
        }
    }
};

}
