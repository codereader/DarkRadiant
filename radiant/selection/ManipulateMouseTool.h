#pragma once

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

    Result onMouseDown(Event& ev)
    {
        _view = render::View(ev.getInteractiveView().getVolumeTest());

        Vector2 epsilon(_selectEpsilon / ev.getInteractiveView().getDeviceWidth(), 
                        _selectEpsilon / ev.getInteractiveView().getDeviceHeight());

        Vector2 devicePos = window_to_normalised_device(ev.getDevicePosition(),
                                                        ev.getInteractiveView().getDeviceWidth(),
                                                        ev.getInteractiveView().getDeviceHeight());

        if (GlobalSelectionSystem().SelectManipulator(_view, devicePos, epsilon))
        {
            return Result::Activated;
        }

        return Result::Ignored; // not handled
    }

    Result onMouseMove(Event& ev)
    {
        Vector2 devicePos = window_to_normalised_device(ev.getDevicePosition(),
                                                        ev.getInteractiveView().getDeviceWidth(),
                                                        ev.getInteractiveView().getDeviceHeight());

        GlobalSelectionSystem().MoveSelected(_view, devicePos);
        
        return Result::Continued;
    }

    Result onMouseUp(Event& ev)
    {
        // Notify the selectionsystem about the finished operation
        GlobalSelectionSystem().endMove();
        return Result::Finished;
    }
};

}
