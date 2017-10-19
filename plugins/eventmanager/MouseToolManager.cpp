#include "MouseToolManager.h"

#include "iradiant.h"
#include "iuimanager.h"
#include "iregistry.h"
#include "itextstream.h"
#include "string/convert.h"
#include "string/join.h"
#include "wxutil/MouseButton.h"
#include "wxutil/Modifier.h"

namespace ui
{

MouseToolManager::MouseToolManager() :
    _activeModifierState(0)
{}

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

void MouseToolManager::loadGroupMapping(MouseToolGroup::Type type, const xml::NodeList& userMappings, const xml::NodeList& defaultMappings)
{
	MouseToolGroup& group = getGroup(type);

	group.clearToolMappings();

	group.foreachMouseTool([&] (const MouseToolPtr& tool)
	{
		// First, look in the userMappings if we have a user-defined setting
		for (const xml::Node& node : userMappings)
		{
			if (node.getAttributeValue("name") == tool->getName())
			{
				// Load the condition
				unsigned int state = wxutil::MouseButton::LoadFromNode(node) | wxutil::Modifier::LoadFromNode(node);
				group.addToolMapping(state, tool);

				return; // done here
			}
		}

		// nothing found in the user mapping, fall back to default
		for (const xml::Node& node : defaultMappings)
		{
			if (node.getAttributeValue("name") == tool->getName())
			{
				// Load the condition
				unsigned int state = wxutil::MouseButton::LoadFromNode(node) | wxutil::Modifier::LoadFromNode(node);
				group.addToolMapping(state, tool);

				return; // done here
			}
		}

		// No mapping for this tool
	});
}

void MouseToolManager::loadToolMappings()
{
    // All modules have registered their stuff, now load the mapping
    // Try the user-defined mapping first
    xml::NodeList userMappings = GlobalRegistry().findXPath("user/ui/input/mouseToolMappings[@name='user']//mouseToolMapping//tool");
	xml::NodeList defaultMappings = GlobalRegistry().findXPath("user/ui/input/mouseToolMappings[@name='default']//mouseToolMapping//tool");

	loadGroupMapping(MouseToolGroup::Type::CameraView, userMappings, defaultMappings);
	loadGroupMapping(MouseToolGroup::Type::OrthoView, userMappings, defaultMappings);
}

void MouseToolManager::resetBindingsToDefault()
{
    // Remove all user settings
    GlobalRegistry().deleteXPath("user/ui/input//mouseToolMappings[@name='user']");

    // Reload the bindings
    loadToolMappings();
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
            wxutil::Modifier::SaveToNode(state, toolNode);
        });
    });
}

void MouseToolManager::shutdownModule()
{
    // Save tool mappings
    saveToolMappings();

    _mouseToolGroups.clear();
}

MouseToolGroup& MouseToolManager::getGroup(IMouseToolGroup::Type group)
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

MouseToolStack MouseToolManager::getMouseToolsForEvent(IMouseToolGroup::Type group, unsigned int mouseState)
{
    return getGroup(group).getMappedTools(mouseState);
}

void MouseToolManager::updateStatusbar(unsigned int newState)
{
    // Only do this if the flags actually changed
    if (newState == _activeModifierState)
    {
        return;
    }

    _activeModifierState = newState;

    std::string statusText("");

    if (_activeModifierState != 0)
    {
        wxutil::MouseButton::ForeachButton([&](unsigned int button)
        {
            unsigned int testFlags = _activeModifierState | button;

            std::set<std::string> toolNames;

            GlobalMouseToolManager().foreachGroup([&](ui::IMouseToolGroup& group)
            {
                ui::MouseToolStack tools = group.getMappedTools(testFlags);

                for (auto i : tools)
                {
                    toolNames.insert(i->getDisplayName());
                }
            });

            if (!toolNames.empty())
            {
                statusText += wxutil::Modifier::GetModifierString(_activeModifierState) + "-";
                statusText += wxutil::MouseButton::GetButtonString(testFlags) + ": ";
                statusText += string::join(toolNames, ", ");
                statusText += " ";
            }
        });
    }

    // Pass the call
    GlobalUIManager().getStatusBarManager().setText(STATUSBAR_COMMAND, statusText);
}

} // namespace
