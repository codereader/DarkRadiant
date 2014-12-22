#include "MouseToolManager.h"

#include "MouseToolGroup.h"
#include "ieventmanager.h"

namespace ui
{

// RegisterableModule implementation
const std::string& MouseToolManager::getName() const
{
    static std::string _name(MODULE_MOUSETOOLMANAGER);
    return _name;
}

const StringSet& MouseToolManager::getDependencies() const
{
    static StringSet _dependencies;
    return _dependencies;
}

void MouseToolManager::initialiseModule(const ApplicationContext& ctx)
{
}

void MouseToolManager::shutdownModule()
{
    _mouseToolGroups.clear();
}

IMouseToolGroup& MouseToolManager::getGroup(IMouseToolGroup::Type group)
{
    GroupMap::iterator found = _mouseToolGroups.find(group);

    // Insert if not there yet
    if (found == _mouseToolGroups.end())
    {
        found = _mouseToolGroups.insert(std::make_pair(group, std::make_shared<MouseToolGroup>(group))).first;
    }

    return *found->second;
}

void MouseToolManager::foreachGroup(const std::function<void(IMouseToolGroup&)>& functor)
{
    for (auto i : _mouseToolGroups)
    {
        functor(*i.second);
    }
}

MouseToolStack MouseToolManager::getMouseToolStackForEvent(IMouseToolGroup::Type group, wxMouseEvent& ev)
{
    MouseToolStack stack;

    IMouseEvents& mouseEvents = GlobalEventManager().MouseEvents();

    if (group == IMouseToolGroup::Type::OrthoView)
    {
        IMouseToolGroup& toolGroup = getGroup(group);

        if (mouseEvents.stateMatchesObserverEvent(ui::obsSelect, ev) ||
            mouseEvents.stateMatchesObserverEvent(ui::obsToggle, ev) ||
            mouseEvents.stateMatchesObserverEvent(ui::obsToggleGroupPart, ev))
        {
            stack.push_back(toolGroup.getMouseToolByName("DragSelectionMouseTool"));
        }

        if (mouseEvents.stateMatchesObserverEvent(ui::obsToggleFace, ev))
        {
            stack.push_back(toolGroup.getMouseToolByName("DragSelectionMouseToolFaceOnly"));
        }

        if (mouseEvents.stateMatchesObserverEvent(ui::obsReplace, ev))
        {
            stack.push_back(toolGroup.getMouseToolByName("CycleSelectionMouseTool"));
        }

        if (mouseEvents.stateMatchesObserverEvent(ui::obsReplaceFace, ev))
        {
            stack.push_back(toolGroup.getMouseToolByName("CycleSelectionMouseToolFaceOnly"));
        }

        if (mouseEvents.stateMatchesXYViewEvent(xyNewBrushDrag, ev))
        {
            stack.push_back(toolGroup.getMouseToolByName("BrushCreatorTool"));
        }

        if (mouseEvents.stateMatchesXYViewEvent(xySelect, ev))
        {
            stack.push_back(toolGroup.getMouseToolByName("ClipperTool"));
        }

        if (mouseEvents.stateMatchesXYViewEvent(xyZoom, ev))
        {
            stack.push_back(toolGroup.getMouseToolByName("ZoomTool"));
        }

        if (mouseEvents.stateMatchesXYViewEvent(xyCameraAngle, ev))
        {
            stack.push_back(toolGroup.getMouseToolByName("CameraAngleTool"));
        }

        if (mouseEvents.stateMatchesXYViewEvent(xyCameraMove, ev))
        {
            stack.push_back(toolGroup.getMouseToolByName("CameraMoveTool"));
        }

        if (mouseEvents.stateMatchesXYViewEvent(xyMoveView, ev))
        {
            stack.push_back(toolGroup.getMouseToolByName("MoveViewTool"));
        }

        if (mouseEvents.stateMatchesObserverEvent(obsManipulate, ev))
        {
            stack.push_back(toolGroup.getMouseToolByName("ManipulateMouseTool"));
        }
    }
    else if (group == IMouseToolGroup::Type::CameraView)
    {
        IMouseToolGroup& toolGroup = getGroup(group);

        if (mouseEvents.stateMatchesObserverEvent(ui::obsManipulate, ev))
        {
            stack.push_back(toolGroup.getMouseToolByName("ManipulateMouseTool"));
        }

        if (mouseEvents.stateMatchesObserverEvent(ui::obsSelect, ev) ||
            mouseEvents.stateMatchesObserverEvent(ui::obsToggle, ev) ||
            mouseEvents.stateMatchesObserverEvent(ui::obsToggleGroupPart, ev))
        {
            stack.push_back(toolGroup.getMouseToolByName("DragSelectionMouseTool"));
        }

        if (mouseEvents.stateMatchesObserverEvent(ui::obsToggleFace, ev))
        {
            stack.push_back(toolGroup.getMouseToolByName("DragSelectionMouseToolFaceOnly"));
        }

        if (mouseEvents.stateMatchesObserverEvent(ui::obsReplace, ev))
        {
            stack.push_back(toolGroup.getMouseToolByName("CycleSelectionMouseTool"));
        }

        if (mouseEvents.stateMatchesObserverEvent(ui::obsReplaceFace, ev))
        {
            stack.push_back(toolGroup.getMouseToolByName("CycleSelectionMouseToolFaceOnly"));
        }

        if (mouseEvents.stateMatchesObserverEvent(ui::obsCopyTexture, ev))
        {
            stack.push_back(toolGroup.getMouseToolByName("PickShaderTool"));
        }

        if (mouseEvents.stateMatchesObserverEvent(ui::obsPasteTextureProjected, ev))
        {
            stack.push_back(toolGroup.getMouseToolByName("PasteShaderProjectedTool"));
        }

        if (mouseEvents.stateMatchesObserverEvent(ui::obsPasteTextureNatural, ev))
        {
            stack.push_back(toolGroup.getMouseToolByName("PasteShaderNaturalTool"));
        }

        if (mouseEvents.stateMatchesObserverEvent(ui::obsPasteTextureCoordinates, ev))
        {
            stack.push_back(toolGroup.getMouseToolByName("PasteShaderCoordsTool"));
        }

        if (mouseEvents.stateMatchesObserverEvent(ui::obsPasteTextureToBrush, ev))
        {
            stack.push_back(toolGroup.getMouseToolByName("PasteShaderToBrushTool"));
        }

        if (mouseEvents.stateMatchesObserverEvent(ui::obsPasteTextureNameOnly, ev))
        {
            stack.push_back(toolGroup.getMouseToolByName("PasteShaderNameTool"));
        }

        if (mouseEvents.stateMatchesObserverEvent(ui::obsJumpToObject, ev))
        {
            stack.push_back(toolGroup.getMouseToolByName("JumpToObjectTool"));
        }
    }

    return stack;
}

}
