#include "MouseToolManager.h"

#include "MouseToolGroup.h"
#include "iradiant.h"
#include "iregistry.h"
#include "itextstream.h"
#include "string/convert.h"

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

    if (_dependencies.empty())
    {
        _dependencies.insert(MODULE_RADIANT);
    }

    return _dependencies;
}

void MouseToolManager::initialiseModule(const ApplicationContext& ctx)
{
    GlobalRadiant().signal_radiantStarted().connect(
        sigc::mem_fun(this, &MouseToolManager::onRadiantStartup));
}

void MouseToolManager::loadGroupMapping(MouseToolGroup& group, const xml::Node& mappingNode)
{
    for (const xml::Node& node : mappingNode.getNamedChildren("tool"))
    {
        // Load the condition
        MouseState state = MouseState(wxutil::MouseButton::LoadFromNode(node));
        std::string name = node.getAttributeValue("name");
        MouseToolPtr tool = group.getMouseToolByName(name);

        if (!tool)
        {
            rWarning() << "Unregistered MouseTool name in XML for group " << 
                static_cast<int>(group.getType()) << ": " << name << std::endl;
            continue;
        }

        group.setToolMapping(tool, state);
    }
}

void MouseToolManager::loadToolMappings()
{
    // All modules have registered their stuff, now load the mapping
    xml::NodeList mappings = GlobalRegistry().findXPath("user/ui/input/mouseToolMappings//mouseToolMapping");

    for (const xml::Node& node : mappings)
    {
        std::string mappingName = node.getAttributeValue("name");

        int mappingId = string::convert<int>(node.getAttributeValue("id"), -1);

        if (mappingId == -1)
        {
            rMessage() << "Skipping invalid view id in mouse tool mapping " << mappingName << std::endl;
            continue;
        }

        rMessage() << "Loading mouse tool mapping for " << mappingName << std::endl;

        MouseToolGroup::Type type = static_cast<MouseToolGroup::Type>(mappingId);

        MouseToolGroup& group = static_cast<MouseToolGroup&>(getGroup(type));

        loadGroupMapping(group, node);
    }
}

void MouseToolManager::onRadiantStartup()
{
    loadToolMappings();
}

void MouseToolManager::shutdownModule()
{
    // TODO: Save tool mappings

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

MouseToolPtr MouseToolManager::getMouseToolForEvent(IMouseToolGroup::Type group, wxMouseEvent& ev)
{
    // Translate the wxMouseEvent to MouseState
    MouseState state(ev);

    //IMouseEvents& mouseEvents = GlobalEventManager().MouseEvents();

    return static_cast<MouseToolGroup&>(getGroup(group)).getMappedTool(state);

#if 0
    if (group == IMouseToolGroup::Type::OrthoView)
    {
        IMouseToolGroup& toolGroup = getGroup(group);

        return toolGroup.getMappedTool(state);

        if (mouseEvents.stateMatchesObserverEvent(obsSelect, ev) ||
            mouseEvents.stateMatchesObserverEvent(obsToggle, ev) ||
            mouseEvents.stateMatchesObserverEvent(obsToggleGroupPart, ev))
        {
            return toolGroup.getMouseToolByName("DragSelectionMouseTool");
        }

        if (mouseEvents.stateMatchesObserverEvent(obsToggleFace, ev))
        {
            return toolGroup.getMouseToolByName("DragSelectionMouseToolFaceOnly");
        }

        if (mouseEvents.stateMatchesObserverEvent(obsReplace, ev))
        {
            return toolGroup.getMouseToolByName("CycleSelectionMouseTool");
        }

        if (mouseEvents.stateMatchesObserverEvent(obsReplaceFace, ev))
        {
            return toolGroup.getMouseToolByName("CycleSelectionMouseToolFaceOnly");
        }

        if (mouseEvents.stateMatchesXYViewEvent(xyNewBrushDrag, ev))
        {
            return toolGroup.getMouseToolByName("BrushCreatorTool");
        }

        if (mouseEvents.stateMatchesXYViewEvent(xySelect, ev))
        {
            return toolGroup.getMouseToolByName("ClipperTool");
        }

        if (mouseEvents.stateMatchesXYViewEvent(xyZoom, ev))
        {
            return toolGroup.getMouseToolByName("ZoomTool");
        }

        if (mouseEvents.stateMatchesXYViewEvent(xyCameraAngle, ev))
        {
            return toolGroup.getMouseToolByName("CameraAngleTool");
        }

        if (mouseEvents.stateMatchesXYViewEvent(xyCameraMove, ev))
        {
            return toolGroup.getMouseToolByName("CameraMoveTool");
        }

        if (mouseEvents.stateMatchesXYViewEvent(xyMoveView, ev))
        {
            return toolGroup.getMouseToolByName("MoveViewTool");
        }

        if (mouseEvents.stateMatchesObserverEvent(obsManipulate, ev))
        {
            return toolGroup.getMouseToolByName("ManipulateMouseTool");
        }
    }
    else if (group == IMouseToolGroup::Type::CameraView)
    {
        IMouseToolGroup& toolGroup = getGroup(group);

        if (mouseEvents.stateMatchesObserverEvent(obsManipulate, ev))
        {
            return toolGroup.getMouseToolByName("ManipulateMouseTool");
        }

        if (mouseEvents.stateMatchesObserverEvent(obsSelect, ev) ||
            mouseEvents.stateMatchesObserverEvent(obsToggle, ev) ||
            mouseEvents.stateMatchesObserverEvent(obsToggleGroupPart, ev))
        {
            return toolGroup.getMouseToolByName("DragSelectionMouseTool");
        }

        if (mouseEvents.stateMatchesObserverEvent(obsToggleFace, ev))
        {
            return toolGroup.getMouseToolByName("DragSelectionMouseToolFaceOnly");
        }

        if (mouseEvents.stateMatchesObserverEvent(obsReplace, ev))
        {
            return toolGroup.getMouseToolByName("CycleSelectionMouseTool");
        }

        if (mouseEvents.stateMatchesObserverEvent(obsReplaceFace, ev))
        {
            return toolGroup.getMouseToolByName("CycleSelectionMouseToolFaceOnly");
        }

        if (mouseEvents.stateMatchesObserverEvent(obsCopyTexture, ev))
        {
            return toolGroup.getMouseToolByName("PickShaderTool");
        }

        if (mouseEvents.stateMatchesObserverEvent(obsPasteTextureProjected, ev))
        {
            return toolGroup.getMouseToolByName("PasteShaderProjectedTool");
        }

        if (mouseEvents.stateMatchesObserverEvent(obsPasteTextureNatural, ev))
        {
            return toolGroup.getMouseToolByName("PasteShaderNaturalTool");
        }

        if (mouseEvents.stateMatchesObserverEvent(obsPasteTextureCoordinates, ev))
        {
            return toolGroup.getMouseToolByName("PasteShaderCoordsTool");
        }

        if (mouseEvents.stateMatchesObserverEvent(obsPasteTextureToBrush, ev))
        {
            return toolGroup.getMouseToolByName("PasteShaderToBrushTool");
        }

        if (mouseEvents.stateMatchesObserverEvent(obsPasteTextureNameOnly, ev))
        {
            return toolGroup.getMouseToolByName("PasteShaderNameTool");
        }

        if (mouseEvents.stateMatchesObserverEvent(obsJumpToObject, ev))
        {
            return toolGroup.getMouseToolByName("JumpToObjectTool");
        }
    }

    return MouseToolPtr();
#endif
}

}
