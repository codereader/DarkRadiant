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
    // Base epsilon value as read from the registry
    float _selectEpsilon;

    // Epsilon vector (scaled by device dimensions)
    Vector2 _epsilon;

    render::View _view;

public:
    SelectMouseTool() :
        _selectEpsilon(registry::getValue<float>(RKEY_SELECT_EPSILON))
    {}

    virtual Result onMouseDown(Event& ev)
    {
        _view = render::View(ev.getInteractiveView().getVolumeTest());

        // Reset the epsilon
        _epsilon.x() = _selectEpsilon / ev.getInteractiveView().getDeviceWidth();
        _epsilon.y() = _selectEpsilon / ev.getInteractiveView().getDeviceHeight();

        return Result::Activated;
    }

    virtual Result onMouseUp(Event& ev)
    {
        // Invoke the testselect virtual
        testSelect(ev);

        // Refresh the view now that we're done
        ev.getInteractiveView().queueDraw();

        return Result::Finished;
    }

protected:
    // Test select method to be implemented by subclasses
    // testSelect will be called onMouseUp()
    virtual void testSelect(Event& ev) = 0;
};

/**
 * Drag-selection tool class. Renders an overlay rectangle on the device
 * during the active selection phase.
 */
class DragSelectionMouseTool :
    public SelectMouseTool
{
private:

    Vector2 _start;		// Position at mouseDown
    Vector2 _current;	// Position during mouseMove

    selection::Rectangle _dragSelectionRect;

public:
    virtual const std::string& getName()
    {
        static std::string name("DragSelectionMouseTool");
        return name;
    }

    Result onMouseDown(Event& ev)
    {
        _start = _current = ev.getDevicePosition();

        return SelectMouseTool::onMouseDown(ev);
    }

    Result onMouseMove(Event& ev)
    {
        _current = ev.getDevicePosition();

        updateDragSelectionRectangle(ev);

        return Result::Continued;
    }

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
    virtual bool selectFacesOnly()
    {
        return false;
    }

    void testSelect(Event& ev)
    {
        bool isFaceOperation = selectFacesOnly();

        // Get the distance of the mouse pointer from the starting point
        Vector2 delta(ev.getDevicePosition() - _start);

        // If the mouse pointer has moved more than <epsilon>, this is considered a drag operation
        if (fabs(delta.x()) > _epsilon.x() && fabs(delta.y()) > _epsilon.y()) 
        {
            // Call the selectArea command that does the actual selecting
            GlobalSelectionSystem().SelectArea(_view, _start, delta, SelectionSystem::eToggle, isFaceOperation);
        }
        else 
        {
            // Mouse has barely moved, call the point selection routine
            GlobalSelectionSystem().SelectPoint(_view, ev.getDevicePosition(), 
                _epsilon, SelectionSystem::eToggle, isFaceOperation);
        }

        // Reset the mouse position to zero, this mouse operation is finished so far
        _start = _current = Vector2(0.0f, 0.0f);

        _dragSelectionRect = selection::Rectangle();
    }

    void updateDragSelectionRectangle(Event& ev)
    {
        // get the mouse position relative to the starting point
        Vector2 delta(_current - _start);

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
    // Flag used by the selection logic
    bool _mouseMovedSinceLastSelect;

    // Position of the last testSelect call
    Vector2 _lastSelectPos;

public:
    CycleSelectionMouseTool() :
        _mouseMovedSinceLastSelect(true),
        _lastSelectPos(INT_MAX, INT_MAX)
    {}

    const std::string& getName()
    {
        static std::string name("CycleSelectionMouseTool");
        return name;
    }

    Result onMouseMove(Event& ev)
    {
        // Reset the counter, mouse has moved
        _mouseMovedSinceLastSelect = true;

        return Result::Continued;
    }

protected:
    virtual bool selectFacesOnly()
    {
        return false;
    }

    void testSelect(Event& ev)
    {
        const Vector2& curPos = ev.getDevicePosition();

        // If the mouse has moved in between selections, reset the depth counter
        if (curPos != _lastSelectPos)
        {
            _mouseMovedSinceLastSelect = true;
        }

        // If we already replaced a selection, switch to cycle mode
        // eReplace should only be active during the first call without mouse movement
        SelectionSystem::EModifier modifier = _mouseMovedSinceLastSelect ? SelectionSystem::eReplace : SelectionSystem::eCycle;

        GlobalSelectionSystem().SelectPoint(_view, curPos, _epsilon, modifier, selectFacesOnly());

        // Remember this position
        _lastSelectPos = curPos;
        _mouseMovedSinceLastSelect = false;
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
