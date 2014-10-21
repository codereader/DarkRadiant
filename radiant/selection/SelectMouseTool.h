#pragma once

#include "imousetool.h"
#include "registry/registry.h"
#include "render/View.h"
#include "Device.h"

namespace ui
{

/**
 * greebo: This is the class handling the selection-related mouse operations, 
 * like Alt-Shift-Click, Selection toggles and drag selections.
*/
class SelectMouseTool :
    public MouseTool
{
private:
    float _selectEpsilon;

    render::View _view;

    Vector2 _start;		// Position at mouseDown
    Vector2 _current;	// Position during mouseMove

public:
    SelectMouseTool() :
        _selectEpsilon(registry::getValue<float>(RKEY_SELECT_EPSILON))
    {}

    const std::string& getName()
    {
        static std::string name("SelectMouseTool");
        return name;
    }

    Result onMouseDown(Event& ev)
    {
        _start = _current = ev.getDevicePosition();

        return Result::Activated;
    }

    Result onMouseMove(Event& ev)
    {
        _current = ev.getDevicePosition();

        draw_area();

        return Result::Continued;
    }

    Result onMouseUp(Event& ev)
    {
        // Check the result of this (finished) operation, is it a drag or a click?
        testSelect(ev.getDevicePosition());

        return Result::Finished;
    }

private:
    SelectionSystem::EModifier getModifier()
    {
        IMouseEvents& mouseEvents = GlobalEventManager().MouseEvents();

        // Retrieve the according ObserverEvent for the 
        ui::ObserverEvent observerEvent = mouseEvents.getObserverEvent(*_wxEvent);

        if (observerEvent == ui::obsSelect || observerEvent == ui::obsToggle ||
            observerEvent == ui::obsToggleFace || observerEvent == ui::obsToggleGroupPart)
        {
            return SelectionSystem::eToggle;
        }

        if (observerEvent == ui::obsReplace || observerEvent == ui::obsReplaceFace)
        {
            return SelectionSystem::eReplace;
        }

        // greebo: Return the standard case: eManipulator mode, if none of the above apply
        return SelectionSystem::eManipulator;
    }

    void testSelect(const Vector2& position)
    {
        // Get the MouseEvents class from the EventManager
        IMouseEvents& mouseEvents = GlobalEventManager().MouseEvents();

        // Obtain the current modifier status (eManipulator, etc.)
        SelectionSystem::EModifier modifier = getModifier();

        // Determine, if we have a face operation
        // Retrieve the according ObserverEvent for the GdkEventButton
        wxutil::MouseButton::GetStateForMouseEvent
        ui::ObserverEvent observerEvent = mouseEvents.getObserverEventForMouseButtonState();

        bool isFaceOperation = (observerEvent == ui::obsToggleFace || observerEvent == ui::obsReplaceFace);

        // If the user pressed some of the modifiers (Shift, Alt, Ctrl) the mode is NOT eManipulator
        // so the if clause is true if there are some modifiers active
        if (modifier != SelectionSystem::eManipulator) {
            // Get the distance of the mouse pointer from the starting point
            DeviceVector delta(position - _start);

            // If the mouse pointer has moved more than <epsilon>, this is considered a drag operation
            if (fabs(delta.x()) > _epsilon.x() && fabs(delta.y()) > _epsilon.y()) {
                // This is a drag operation with a modifier held
                DeviceVector delta(position - _start);	// << superfluous?

                // Call the selectArea command that does the actual selecting
                GlobalSelectionSystem().SelectArea(*_view, &_start[0], &delta[0], modifier, isFaceOperation);
            }
            else {
                // greebo: This is a click operation with a modifier held
                // If Alt-Shift (eReplace) is held, and we already replaced a selection, switch to cycle mode
                // so eReplace is only active during the first click with Alt-Shift
                if (modifier == SelectionSystem::eReplace && _unmovedReplaces++ > 0) {
                    modifier = SelectionSystem::eCycle;
                }
                // Call the selectPoint command in RadiantSelectionSystem that does the actual selecting
                GlobalSelectionSystem().SelectPoint(*_view, &position[0], &_epsilon[0], modifier, isFaceOperation);
            }
        }

        // Reset the mouse position to zero, this mouse operation is finished so far
        _start = _current = DeviceVector(0.0f, 0.0f);
        draw_area();
    }
};

}
