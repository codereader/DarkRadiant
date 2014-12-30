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
    const std::string& getName()
    {
        static std::string name("FreeMoveTool");
        return name;
    }

    const std::string& getDisplayName()
    {
        static std::string displayName(_("Freemove Mode"));
        return displayName;
    }

    Result onMouseDown(Event& ev)
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

    Result onMouseMove(Event& ev)
    {
        return Result::Ignored;
    }

    Result onMouseUp(Event& ev)
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
};

}
