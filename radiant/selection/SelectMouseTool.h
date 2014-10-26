#pragma once

#include "imousetool.h"
#include "registry/registry.h"
#include "render/View.h"
#include "Device.h"
#include "igl.h"

namespace ui
{

/**
 * greebo: This is the base for classes handling the selection-related mouse operations, 
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
    virtual const std::string& getName()
    {
        static std::string name("DragSelectionMouseTool");
        return name;
    }

    Result onMouseDown(Event& ev)
    {
        _view = render::View(ev.getInteractiveView().getVolumeTest());
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
        ev.getInteractiveView().queueDraw();

        return Result::Finished;
    }

protected:
    virtual bool selectFacesOnly()
    {
        return false;
    }

private:
    void testSelect(const Vector2& position)
    {
        bool isFaceOperation = selectFacesOnly();

        // Get the distance of the mouse pointer from the starting point
        DeviceVector delta(position - _start);

        // If the mouse pointer has moved more than <epsilon>, this is considered a drag operation
        if (fabs(delta.x()) > _epsilon.x() && fabs(delta.y()) > _epsilon.y()) 
        {
            // Call the selectArea command that does the actual selecting
            GlobalSelectionSystem().SelectArea(_view, _start, delta, SelectionSystem::eToggle, isFaceOperation);
        }
        else 
        {
            GlobalSelectionSystem().SelectPoint(_view, position, _epsilon, SelectionSystem::eToggle, isFaceOperation);
        }

        // Reset the mouse position to zero, this mouse operation is finished so far
        _start = _current = DeviceVector(0.0f, 0.0f);
        _dragSelectionRect = selection::Rectangle();
    }
};

class DragSelectionMouseToolFacesOnly :
    public DragSelectionMouseTool
{
public:
    virtual const std::string& getName()
    {
        static std::string name("DragSelectionMouseToolFaceOnly");
        return name;
    }

protected:
    virtual bool selectFacesOnly()
    {
        return true;
    }
};

class CycleSelectionMouseTool :
    public SelectMouseTool
{
private:
    std::size_t _unmovedReplaces;

    Vector2 _lastSelectPos;

public:
    CycleSelectionMouseTool() :
        _unmovedReplaces(0),
        _lastSelectPos(INT_MAX, INT_MAX)
    {}

    const std::string& getName()
    {
        static std::string name("CycleSelectionMouseTool");
        return name;
    }

    Result onMouseDown(Event& ev)
    {
        _view = render::View(ev.getInteractiveView().getVolumeTest());

        // Reset the epsilon
        _epsilon.x() = _selectEpsilon / ev.getInteractiveView().getDeviceWidth();
        _epsilon.y() = _selectEpsilon / ev.getInteractiveView().getDeviceHeight();

        return Result::Activated;
    }

    Result onMouseMove(Event& ev)
    {
        // Reset the counter, mouse has moved
        _unmovedReplaces = 0;

        return Result::Continued;
    }

    Result onMouseUp(Event& ev)
    {
        // Check the result of this (finished) operation, is it a drag or a click?
        testSelect(ev.getDevicePosition());

        // Refresh the view
        ev.getInteractiveView().queueDraw();

        return Result::Finished;
    }

protected:
    virtual bool selectFacesOnly()
    {
        return false;
    }

private:
    void testSelect(const Vector2& position)
    {
        // If the mouse has moved in between selections, reset the depth counter
        if (position != _lastSelectPos)
        {
            _unmovedReplaces = 0;
        }

        // greebo: This is a click operation with a modifier held
        // If Alt-Shift (eReplace) is held, and we already replaced a selection, switch to cycle mode
        // so eReplace is only active during the first click with Alt-Shift
        SelectionSystem::EModifier modifier = _unmovedReplaces++ > 0 ? SelectionSystem::eCycle : SelectionSystem::eReplace;

        GlobalSelectionSystem().SelectPoint(_view, position, _epsilon, modifier, selectFacesOnly());

        // Remember this position
        _lastSelectPos = position;
    }
};

class CycleSelectionMouseToolFaceOnly :
    public CycleSelectionMouseTool
{
public:
    virtual const std::string& getName()
    {
        static std::string name("CycleSelectionMouseToolFaceOnly");
        return name;
    }

protected:
    virtual bool selectFacesOnly()
    {
        return true;
    }
};

}
