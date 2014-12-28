#include "ManipulateMouseTool.h"

#include "i18n.h"
#include "registry/registry.h"
#include "Device.h"

namespace ui
{

namespace
{
    const char* const RKEY_SELECT_EPSILON = "user/ui/selectionEpsilon";
}

ManipulateMouseTool::ManipulateMouseTool(SelectionSystem& selectionSystem) :
    _selectEpsilon(registry::getValue<float>(RKEY_SELECT_EPSILON)),
    _selectionSystem(selectionSystem)
{}

const std::string& ManipulateMouseTool::getName()
{
    static std::string name("ManipulateMouseTool");
    return name;
}

const std::string& ManipulateMouseTool::getDisplayName()
{
    static std::string displayName(_("Manipulate"));
    return displayName;
}

ManipulateMouseTool::Result ManipulateMouseTool::onMouseDown(Event& ev)
{
    _view = render::View(ev.getInteractiveView().getVolumeTest());

    Vector2 epsilon(_selectEpsilon / ev.getInteractiveView().getDeviceWidth(),
                    _selectEpsilon / ev.getInteractiveView().getDeviceHeight());

    if (_selectionSystem.SelectManipulator(_view, ev.getDevicePosition(), epsilon))
    {
        return Result::Activated;
    }

    return Result::Ignored; // not handled
}

ManipulateMouseTool::Result ManipulateMouseTool::onMouseMove(Event& ev)
{
    // Get the view afresh each time, chasemouse might have changed the view since onMouseDown
    _view = render::View(ev.getInteractiveView().getVolumeTest());

    _selectionSystem.MoveSelected(_view, ev.getDevicePosition());

    return Result::Continued;
}

ManipulateMouseTool::Result ManipulateMouseTool::onMouseUp(Event& ev)
{
    // Notify the selectionsystem about the finished operation
    _selectionSystem.endMove();
    return Result::Finished;
}

void ManipulateMouseTool::onCancel()
{
    // Update the views
    _selectionSystem.cancelMove();
}

unsigned int ManipulateMouseTool::getPointerMode()
{
    return PointerMode::Capture;
}

}
