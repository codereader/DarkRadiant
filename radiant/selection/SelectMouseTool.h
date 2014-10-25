#pragma once

#include "imousetool.h"
#include "registry/registry.h"
#include "render/View.h"
#include "Device.h"
#include "igl.h"

namespace ui
{

/**
 * greebo: This is the class handling the selection-related mouse operations, 
 * like Alt-Shift-Click, Selection toggles and drag selections.
*/
class SelectMouseTool :
    public MouseTool
{
protected:
    float _selectEpsilon;

    // Scaled epsilon vector
    DeviceVector _epsilon;

    render::View _view;

    Vector2 _start;		// Position at mouseDown
    Vector2 _current;	// Position during mouseMove

    selection::Rectangle _dragSelectionRect;

public:
    SelectMouseTool() :
        _selectEpsilon(registry::getValue<float>(RKEY_SELECT_EPSILON))
    {}

    virtual void renderOverlay()
    {
        // Define the blend function for transparency
        glEnable(GL_BLEND);
        glBlendColor(0, 0, 0, 0.2f);
        glBlendFunc(GL_CONSTANT_ALPHA_EXT, GL_ONE_MINUS_CONSTANT_ALPHA_EXT);

        Vector3 dragBoxColour = ColourSchemes().getColour("drag_selection");
        glColor3dv(dragBoxColour);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

        // The transparent fill rectangle
        glBegin(GL_QUADS);
        glVertex2f(_dragSelectionRect.min.x(), _dragSelectionRect.min.y());
        glVertex2f(_dragSelectionRect.max.x(), _dragSelectionRect.min.y());
        glVertex2f(_dragSelectionRect.max.x(), _dragSelectionRect.max.y());
        glVertex2f(_dragSelectionRect.min.x(), _dragSelectionRect.max.y());
        glEnd();

        // The solid borders
        glBlendColor(0, 0, 0, 0.8f);
        glBegin(GL_LINE_LOOP);
        glVertex2f(_dragSelectionRect.min.x(), _dragSelectionRect.min.y());
        glVertex2f(_dragSelectionRect.max.x(), _dragSelectionRect.min.y());
        glVertex2f(_dragSelectionRect.max.x(), _dragSelectionRect.max.y());
        glVertex2f(_dragSelectionRect.min.x(), _dragSelectionRect.max.y());
        glEnd();

        glDisable(GL_BLEND);
}

protected:
#if 0
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
#endif

    void testSelect(const Vector2& position)
    {
#if 0
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
#endif
        // Reset the mouse position to zero, this mouse operation is finished so far
        _start = _current = DeviceVector(0.0f, 0.0f);
        _dragSelectionRect = selection::Rectangle();
    }

    void updateDragSelectionRectangle(Event& ev)
    {
        // get the mouse position relative to the starting point
        DeviceVector delta(_current - _start);

        if (fabs(delta.x()) > _epsilon.x() && fabs(delta.y()) > _epsilon.y())
        {
            _dragSelectionRect = selection::Rectangle::ConstructFromArea(_start, delta);
            _dragSelectionRect.toScreenCoords(ev.getInteractiveView().getDeviceWidth(), 
                                              ev.getInteractiveView().getDeviceHeight());
        }
        else // ...otherwise return an empty area
        {
            _dragSelectionRect = selection::Rectangle();
        }

        ev.getInteractiveView().queueDraw();
    }
};

class DragSelectionMouseTool :
    public SelectMouseTool
{
public:
    DragSelectionMouseTool()
    {}

    const std::string& getName()
    {
        static std::string name("DragSelectionMouseTool");
        return name;
    }

    Result onMouseDown(Event& ev)
    {
        _start = _current = ev.getDevicePosition();

        // Reset the epsilon
        _epsilon.x() = _selectEpsilon / ev.getInteractiveView().getDeviceWidth();
        _epsilon.y() = _selectEpsilon / ev.getInteractiveView().getDeviceHeight();

        return Result::Activated;
    }

    Result onMouseMove(Event& ev)
    {
        _current = ev.getDevicePosition();

        updateDragSelectionRectangle(ev);

        return Result::Continued;
    }

    Result onMouseUp(Event& ev)
    {
        // Check the result of this (finished) operation, is it a drag or a click?
        testSelect(ev.getDevicePosition());

        return Result::Finished;
    }
};

class CycleSelectionMouseTool :
    public SelectMouseTool
{
public:
    CycleSelectionMouseTool()
    {}

    const std::string& getName()
    {
        static std::string name("CycleSelectionMouseTool");
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

        updateDragSelectionRectangle(ev);

        return Result::Continued;
    }

    Result onMouseUp(Event& ev)
    {
        // Check the result of this (finished) operation, is it a drag or a click?
        testSelect(ev.getDevicePosition());
        ev.getInteractiveView().queueDraw();

        return Result::Finished;
    }
};

}
