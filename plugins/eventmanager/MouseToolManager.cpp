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
        unsigned int state = wxutil::MouseButton::LoadFromNode(node);
        std::string name = node.getAttributeValue("name");
        MouseToolPtr tool = group.getMouseToolByName(name);

        if (!tool)
        {
            rWarning() << "Unregistered MouseTool name in XML for group " << 
                static_cast<int>(group.getType()) << ": " << name << std::endl;
            continue;
        }

        group.addToolMapping(state, tool);
    }
}

void MouseToolManager::loadToolMappings()
{
    // All modules have registered their stuff, now load the mapping
    // Try the user-defined mapping first
    xml::NodeList mappings = GlobalRegistry().findXPath("user/ui/input/mouseToolMappings[@name='user']//mouseToolMapping");

    if (mappings.empty())
    {
        // Fall back to the default mapping
        mappings = GlobalRegistry().findXPath("user/ui/input/mouseToolMappings[@name='default']//mouseToolMapping");
    }

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

void MouseToolManager::saveToolMappings()
{
    GlobalRegistry().deleteXPath("user/ui/input//mouseToolMappings[@name='user']");

    xml::Node mappingsRoot = GlobalRegistry().createKeyWithName("user/ui/input", "mouseToolMappings", "user");

    foreachGroup([&] (IMouseToolGroup& g)
    {
        MouseToolGroup& group = static_cast<MouseToolGroup&>(g);

        std::string groupName = group.getType() == IMouseToolGroup::Type::OrthoView ? "OrthoView" : "CameraView";

        xml::Node mappingNode = mappingsRoot.createChild("mouseToolMapping");
        mappingNode.setAttributeValue("name", groupName);
        mappingNode.setAttributeValue("id", string::to_string(static_cast<int>(group.getType())));

        // e.g. <tool name="CameraMoveTool" button="MMB" modifiers="CONTROL" />
        group.foreachMapping([&](unsigned int state, const MouseToolPtr& tool)
        {
            xml::Node toolNode = mappingNode.createChild("tool");

            toolNode.setAttributeValue("name", tool->getName());
            wxutil::MouseButton::SaveToNode(state, toolNode);
        });
    });
}

void MouseToolManager::shutdownModule()
{
    // Save tool mappings
    saveToolMappings();

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

MouseToolStack MouseToolManager::getMouseToolsForEvent(IMouseToolGroup::Type group, wxMouseEvent& ev)
{
    // Translate the wxMouseEvent to MouseState
    unsigned int state = wxutil::MouseButton::GetStateForMouseEvent(ev);

    return static_cast<MouseToolGroup&>(getGroup(group)).getMappedTools(state);
}

}
