#include "SelectionMouseTools.h"

#include "iscenegraph.h"
#include "i18n.h"
#include "icolourscheme.h"
#include "registry/registry.h"
#include "selection/Device.h"
#include "Rectangle.h"
#include "selection/SelectionVolume.h"
#include "igl.h"

#include <climits>

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

const std::string& BasicSelectionTool::getName()
{
    static std::string name("BasicSelectionTool");
    return name;
}

const std::string& BasicSelectionTool::getDisplayName()
{
    static std::string displayName(_("Select"));
    return displayName;
}

MouseTool::Result BasicSelectionTool::onMouseDown(Event& ev)
{
    _start = _current = ev.getDevicePosition();

    return SelectMouseTool::onMouseDown(ev);
}

MouseTool::Result BasicSelectionTool::onMouseMove(Event& ev)
{
    _current = ev.getDevicePosition();

    if (ev.getInteractiveView().supportsDragSelections())
    {
        updateDragSelectionRectangle(ev);
    }

    return Result::Continued;
}

void BasicSelectionTool::onMouseCaptureLost(IInteractiveView& view)
{
    onCancel(view); // same behaviour as cancel
}

BasicSelectionTool::Result BasicSelectionTool::onCancel(IInteractiveView& view)
{
    // Reset the mouse position to zero
    _start = _current = Vector2(0.0f, 0.0f);

    _dragSelectionRect = selection::Rectangle();

    // Update the views
	SceneChangeNotify();

    return Result::Finished;
}

void BasicSelectionTool::renderOverlay()
{
    // Define the blend function for transparency
    glEnable(GL_BLEND);
    glBlendColor(0, 0, 0, 0.2f);
    glBlendFunc(GL_CONSTANT_ALPHA_EXT, GL_ONE_MINUS_CONSTANT_ALPHA_EXT);

    Vector3 dragBoxColour = GlobalColourSchemeManager().getColour("drag_selection");
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

void BasicSelectionTool::performSelectionTest(SelectionVolume& volume, SelectionType type, MouseTool::Event& ev)
{
    if (type == SelectionType::Area)
    {
        GlobalSelectionSystem().selectArea(volume, selection::SelectionSystem::eToggle, selectFacesOnly());
    }
    else
    {
        GlobalSelectionSystem().selectPoint(volume, selection::SelectionSystem::eToggle, selectFacesOnly());
    }
}

void BasicSelectionTool::testSelect(MouseTool::Event& ev)
{
    // Get the distance of the mouse pointer from the starting point
    Vector2 delta(ev.getDevicePosition() - _start);

    // If the mouse pointer has moved more than <epsilon>, this is considered a drag operation
    if (ev.getInteractiveView().supportsDragSelections() && fabs(delta.x()) > _epsilon.x() && fabs(delta.y()) > _epsilon.y())
    {
        // Construct the selection test according to the area the user covered with his drag
        render::View scissored(_view);
        ConstructSelectionTest(scissored, selection::Rectangle::ConstructFromArea(_start, delta));

        SelectionVolume volume(scissored);
        performSelectionTest(volume, SelectionType::Area, ev);
    }
    else
    {
        // Mouse has barely moved, call the point selection routine
        // 
        // Copy the view to create a scissored volume
        render::View scissored(_view);
        // Create a volume out of a small box with 2*epsilon edge length
        ConstructSelectionTest(scissored, 
            selection::Rectangle::ConstructFromPoint(ev.getDevicePosition(), _epsilon));

        // Create a selection test using that volume
        SelectionVolume volume(scissored);
        performSelectionTest(volume, SelectionType::Point, ev);
    }

    // Reset the mouse position to zero, this mouse operation is finished so far
    _start = _current = Vector2(0.0f, 0.0f);

    _dragSelectionRect = selection::Rectangle();
}

void BasicSelectionTool::updateDragSelectionRectangle(Event& ev)
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

void CycleSelectionMouseTool::performPointSelection(SelectionVolume& volume, selection::SelectionSystem::EModifier modifier)
{
    GlobalSelectionSystem().selectPoint(volume, modifier, selectFacesOnly());
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
    auto modifier = _mouseMovedSinceLastSelect ? selection::SelectionSystem::eReplace : selection::SelectionSystem::eCycle;
    
    // Copy the view to create a scissored volume
    render::View scissored(_view);
    // Create a volume out of a small box with 2*epsilon edge length
    ConstructSelectionTest(scissored, selection::Rectangle::ConstructFromPoint(curPos, _epsilon));

    // Create a selection test using that volume
    SelectionVolume volume(scissored);

    // Invoke the virtual function to dispatch the selection request
    performPointSelection(volume, modifier);

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
