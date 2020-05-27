#include "SelectionMouseTools.h"

#include "iscenegraph.h"
#include "i18n.h"
#include "iuimanager.h"
#include "registry/registry.h"
#include "Device.h"
#include "igl.h"

namespace ui
{

namespace
{
    const char* const RKEY_SELECT_EPSILON = "user/ui/selectionEpsilon";
}

SelectMouseTool::SelectMouseTool() :
    _selectEpsilon(registry::getValue<float>(RKEY_SELECT_EPSILON))
{}

MouseTool::Result SelectMouseTool::onMouseDown(Event& ev)
{
    _view = render::View(ev.getInteractiveView().getVolumeTest());

    // Reset the epsilon
    _epsilon.x() = _selectEpsilon / ev.getInteractiveView().getDeviceWidth();
    _epsilon.y() = _selectEpsilon / ev.getInteractiveView().getDeviceHeight();

    return Result::Activated;
}

MouseTool::Result SelectMouseTool::onMouseUp(Event& ev)
{
    // Invoke the testselect virtual
    testSelect(ev);

    // Refresh the view now that we're done
    ev.getInteractiveView().queueDraw();

    return Result::Finished;
}

// DragSelection

const std::string& DragSelectionMouseTool::getName()
{
    static std::string name("DragSelectionMouseTool");
    return name;
}

const std::string& DragSelectionMouseTool::getDisplayName()
{
    static std::string displayName(_("Select"));
    return displayName;
}

MouseTool::Result DragSelectionMouseTool::onMouseDown(Event& ev)
{
    _start = _current = ev.getDevicePosition();

    return SelectMouseTool::onMouseDown(ev);
}

MouseTool::Result DragSelectionMouseTool::onMouseMove(Event& ev)
{
    _current = ev.getDevicePosition();

    updateDragSelectionRectangle(ev);

    return Result::Continued;
}

void DragSelectionMouseTool::onMouseCaptureLost(IInteractiveView& view)
{
    onCancel(view); // same behaviour as cancel
}

DragSelectionMouseTool::Result DragSelectionMouseTool::onCancel(IInteractiveView& view)
{
    // Reset the mouse position to zero
    _start = _current = Vector2(0.0f, 0.0f);

    _dragSelectionRect = selection::Rectangle();

    // Update the views
	SceneChangeNotify();

    return Result::Finished;
}

void DragSelectionMouseTool::renderOverlay()
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

void DragSelectionMouseTool::testSelect(MouseTool::Event& ev)
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

void DragSelectionMouseTool::updateDragSelectionRectangle(Event& ev)
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

const std::string& DragSelectionMouseToolFaceOnly::getName()
{
    static std::string name("DragSelectionMouseToolFaceOnly");
    return name;
}

const std::string& DragSelectionMouseToolFaceOnly::getDisplayName()
{
    static std::string displayName(_("Select Faces"));
    return displayName;
}

// Cycle Selection

CycleSelectionMouseTool::CycleSelectionMouseTool() :
    _mouseMovedSinceLastSelect(true),
    _lastSelectPos(INT_MAX, INT_MAX)
{}

const std::string& CycleSelectionMouseTool::getName()
{
    static std::string name("CycleSelectionMouseTool");
    return name;
}

const std::string& CycleSelectionMouseTool::getDisplayName()
{
    static std::string displayName(_("Cycle Selection"));
    return displayName;
}

MouseTool::Result CycleSelectionMouseTool::onMouseMove(Event& ev)
{
    // Reset the counter, mouse has moved
    _mouseMovedSinceLastSelect = true;

    return Result::Continued;
}

void CycleSelectionMouseTool::onMouseCaptureLost(IInteractiveView& view)
{
    onCancel(view); // same as cancel
}

CycleSelectionMouseTool::Result CycleSelectionMouseTool::onCancel(IInteractiveView& view)
{
    _mouseMovedSinceLastSelect = true;

    return Result::Finished;
}

void CycleSelectionMouseTool::testSelect(MouseTool::Event& ev)
{
    const Vector2& curPos = ev.getDevicePosition();

    // If the mouse has moved in between selections, reset the depth counter
    if (!_mouseMovedSinceLastSelect && curPos != _lastSelectPos)
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

const std::string& CycleSelectionMouseToolFaceOnly::getName()
{
    static std::string name("CycleSelectionMouseToolFaceOnly");
    return name;
}

const std::string& CycleSelectionMouseToolFaceOnly::getDisplayName()
{
    static std::string displayName(_("Cycle Face Selection"));
    return displayName;
}

}
