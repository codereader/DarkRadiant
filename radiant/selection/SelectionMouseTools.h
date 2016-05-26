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

    virtual Result onMouseDown(Event& ev) override;
    virtual Result onMouseUp(Event& ev) override;

    virtual bool allowChaseMouse() override
    {
        return false;
    }

    virtual unsigned int getPointerMode() override
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
    virtual const std::string& getName() override;
    virtual const std::string& getDisplayName() override;

    Result onMouseDown(Event& ev) override;
    Result onMouseMove(Event& ev) override;

    void onMouseCaptureLost(IInteractiveView& view) override;
    Result onCancel(IInteractiveView& view) override;

    virtual void renderOverlay() override;

protected:
    virtual bool selectFacesOnly()
    {
        return false;
    }

    // Performs a drag- or point-selection test
    void testSelect(Event& ev) override;

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
    const std::string& getName() override;
    const std::string& getDisplayName() override;

protected:
    bool selectFacesOnly() override
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

    virtual const std::string& getName() override;
    virtual const std::string& getDisplayName() override;

    Result onMouseMove(Event& ev) override;

    void onMouseCaptureLost(IInteractiveView& view) override;
    Result onCancel(IInteractiveView& view) override;

protected:
    virtual bool selectFacesOnly() 
    {
        return false;
    }

    void testSelect(Event& ev) override;
};

/**
* Face-only variant of the CycleSelectionMouseTool.
*/
class CycleSelectionMouseToolFaceOnly :
    public CycleSelectionMouseTool
{
public:
    const std::string& getName() override;
    const std::string& getDisplayName() override;

protected:
    bool selectFacesOnly() override
    {
        return true;
    }
};

}
