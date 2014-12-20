#pragma once

#include "imousetool.h"
#include "render/View.h"
#include "Rectangle.h"

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
    SelectMouseTool();

    virtual Result onMouseDown(Event& ev);

    virtual Result onMouseUp(Event& ev);

    virtual bool allowChaseMouse()
    {
        return false;
    }

    virtual unsigned int getPointerMode()
    {
        return PointerMode::Capture;
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
    virtual const std::string& getName();

    Result onMouseDown(Event& ev);

    Result onMouseMove(Event& ev);

    void onCancel();

    virtual void renderOverlay();

protected:
    virtual bool selectFacesOnly()
    {
        return false;
    }

    // Performs a drag- or point-selection test
    void testSelect(Event& ev);

    // Recalculates the rectangle used to draw the GUI overlay
    void updateDragSelectionRectangle(Event& ev);
};

/**
 * Face-only variant of the DragSelectionMouseTool.
 */
class DragSelectionMouseToolFaceOnly :
    public DragSelectionMouseTool
{
public:
    virtual const std::string& getName();

protected:
    virtual bool selectFacesOnly()
    {
        return true;
    }
};

/**
 * Used to cycle between single selectables while holding a special modifier.
 * The selection candidates are traverse from front to back, moving
 * "deeper" into the scene. Works both in Camera and XY views.
 */
class CycleSelectionMouseTool :
    public SelectMouseTool
{
private:
    // Flag used by the selection logic
    bool _mouseMovedSinceLastSelect;

    // Position of the last testSelect call
    Vector2 _lastSelectPos;

public:
    CycleSelectionMouseTool();

    const std::string& getName();

    Result onMouseMove(Event& ev);

    void onCancel();

protected:
    virtual bool selectFacesOnly()
    {
        return false;
    }

    void testSelect(Event& ev);
};

/**
* Face-only variant of the CycleSelectionMouseTool.
*/
class CycleSelectionMouseToolFaceOnly :
    public CycleSelectionMouseTool
{
public:
    virtual const std::string& getName();

protected:
    virtual bool selectFacesOnly()
    {
        return true;
    }
};

}
