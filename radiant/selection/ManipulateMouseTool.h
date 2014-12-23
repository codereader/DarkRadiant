#pragma once

#include "i18n.h"
#include "imousetool.h"
#include "registry/registry.h"
#include "render/View.h"
#include "Device.h"

namespace ui
{

namespace
{
    const char* const RKEY_SELECT_EPSILON = "user/ui/selectionEpsilon";
}

/**
 * greebo: This is the tool handling the manipulation mouse operations, it basically just
 * passes all the mouse clicks back to the SelectionSystem, trying to select something 
 * that can be manipulated (patches, lights, drag-resizable objects, vertices,...)
 */
class ManipulateMouseTool :
    public MouseTool
{
private:
    float _selectEpsilon;

    render::View _view;

public:
    ManipulateMouseTool() :
        _selectEpsilon(registry::getValue<float>(RKEY_SELECT_EPSILON))
    {}

    const std::string& getName()
    {
        static std::string name("ManipulateMouseTool");
        return name;
    }

    const std::string& getDisplayName()
    {
        static std::string displayName(_("Manipulate"));
        return displayName;
    }

    Result onMouseDown(Event& ev)
    {
        _view = render::View(ev.getInteractiveView().getVolumeTest());

        Vector2 epsilon(_selectEpsilon / ev.getInteractiveView().getDeviceWidth(), 
                        _selectEpsilon / ev.getInteractiveView().getDeviceHeight());

        if (GlobalSelectionSystem().SelectManipulator(_view, ev.getDevicePosition(), epsilon))
        {
            return Result::Activated;
        }

        return Result::Ignored; // not handled
    }

    Result onMouseMove(Event& ev)
    {
        // Get the view afresh each time, chasemouse might have changed the view since onMouseDown
        _view = render::View(ev.getInteractiveView().getVolumeTest());

        GlobalSelectionSystem().MoveSelected(_view, ev.getDevicePosition());
        
        return Result::Continued;
    }

    Result onMouseUp(Event& ev)
    {
        // Notify the selectionsystem about the finished operation
        GlobalSelectionSystem().endMove();
        return Result::Finished;
    }

    void onCancel()
    {
        // Update the views
        GlobalSelectionSystem().cancelMove();
    }

    virtual bool allowChaseMouse()
    {
        return true;
    }

    virtual unsigned int getPointerMode()
    {
        return PointerMode::Capture;
    }
};

}
