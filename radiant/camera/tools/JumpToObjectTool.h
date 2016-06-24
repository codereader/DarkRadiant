#pragma once

#include "imousetool.h"
#include "i18n.h"
#include "../GlobalCamera.h"

namespace ui
{

class JumpToObjectTool :
    public MouseTool
{
public:
    const std::string& getName() override
    {
        static std::string name("JumpToObjectTool");
        return name;
    }

    const std::string& getDisplayName() override
    {
        static std::string displayName(_("Jump to Object"));
        return displayName;
    }

    Result onMouseDown(Event& ev) override
    {
        try
        {
            CameraMouseToolEvent& camEvent = dynamic_cast<CameraMouseToolEvent&>(ev);

            SelectionTestPtr selectionTest = camEvent.getView().createSelectionTestForPoint(camEvent.getDevicePosition());

            CamWndPtr cam = GlobalCamera().getActiveCamWnd();

            if (cam != NULL)
            {
                cam->jumpToObject(*selectionTest);
            }

            return Result::Finished;
        }
        catch (std::bad_cast&)
        {
        }

        return Result::Ignored; // not handled

    }

    Result onMouseMove(Event& ev) override
    {
        return Result::Finished;
    }

    Result onMouseUp(Event& ev) override
    {
        return Result::Finished;
    }
};

} // namespace
