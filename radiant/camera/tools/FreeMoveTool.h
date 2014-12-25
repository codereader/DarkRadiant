#pragma once

#include "imousetool.h"
#include "i18n.h"
#include "../GlobalCamera.h"

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

            if (!camEvent.getView().freeMoveEnabled())
            {
                camEvent.getView().enableFreeMove();
            }
            else
            {
                camEvent.getView().disableFreeMove();
            }

            return Result::Finished;
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
        return Result::Ignored;
    }
};

}
