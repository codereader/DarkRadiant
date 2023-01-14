#include "MouseToolManager.h"

#include <stdexcept>
#include "iradiant.h"
#include "iregistry.h"
#include "itextstream.h"
#include "ui/imainframe.h"

#include "xmlutil/Node.h"
#include "string/convert.h"
#include "string/join.h"
#include "wxutil/MouseButton.h"
#include "wxutil/Modifier.h"
#include "module/StaticModule.h"
#include "ModifierHintPopup.h"

#include <fmt/format.h>
#include <wx/frame.h>

namespace ui
{

namespace
{
    constexpr int HINT_POPUP_CLOSE_TIMEOUT_MSECS = 1000;

    inline std::string getToolGroupName(IMouseToolGroup::Type group)
    {
        switch (group)
        {
        case IMouseToolGroup::Type::OrthoView: return "OrthoView";
        case IMouseToolGroup::Type::CameraView: return "CameraView";
        case IMouseToolGroup::Type::TextureTool: return "TextureTool";
        default: 
            throw std::logic_error("Tool group name not resolvable: " + string::to_string(static_cast<int>(group)));
        }
    }

    inline std::string getMappingPath(bool userMappings, MouseToolGroup::Type group)
    {
        return fmt::format("user/ui/input/mouseToolMappings[@name='{0}']//mouseToolMapping[@id={1}]//tool",
            userMappings ? "user" : "default", static_cast<int>(group));
    }
}

MouseToolManager::MouseToolManager() :
    _activeModifierState(0),
    _hintCloseTimer(this),
    _hintPopup(nullptr),
    _shouldClosePopup(false)
{
    Bind(wxEVT_TIMER, &MouseToolManager::onCloseTimerIntervalReached, this);
}

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
        _dependencies.insert(MODULE_MAINFRAME);
    }

    return _dependencies;
}

void MouseToolManager::initialiseModule(const IApplicationContext& ctx)
{
    GlobalMainFrame().signal_MainFrameConstructed().connect(
        sigc::mem_fun(this, &MouseToolManager::onMainFrameConstructed));
}

void MouseToolManager::loadGroupMapping(MouseToolGroup::Type type)
{
	MouseToolGroup& group = getGroup(type);

    // User-defined mapping take precedence, add them first
    auto mappings = GlobalRegistry().findXPath(getMappingPath(true, type));

    // Append the default mappings at the end of the list
    auto defaultMappings = GlobalRegistry().findXPath(getMappingPath(false, type));
    mappings.insert(mappings.end(), defaultMappings.begin(), defaultMappings.end());

	group.clearToolMappings();

	group.foreachMouseTool([&] (const MouseToolPtr& tool)
	{
		for (const auto& node : mappings)
		{
            if (node.getAttributeValue("name") != tool->getName())
            {
                continue;
            }

			// Found it, load the modifier+button combination
			auto state = wxutil::MouseButton::LoadFromNode(node) | wxutil::Modifier::LoadFromNode(node);
			group.addToolMapping(state, tool);

			break; // done here
		}
	});
}

void MouseToolManager::loadToolMappings()
{
    // All modules have registered their stuff, now load the mapping
    loadGroupMapping(MouseToolGroup::Type::CameraView);
	loadGroupMapping(MouseToolGroup::Type::OrthoView);
	loadGroupMapping(MouseToolGroup::Type::TextureTool);
}

void MouseToolManager::resetBindingsToDefault()
{
    // Remove all user settings
    GlobalRegistry().deleteXPath("user/ui/input//mouseToolMappings[@name='user']");

    // Reload the bindings
    loadToolMappings();
}

void MouseToolManager::onMainFrameConstructed()
{
    loadToolMappings();
}

void MouseToolManager::saveToolMappings()
{
    GlobalRegistry().deleteXPath("user/ui/input//mouseToolMappings[@name='user']");

    auto mappingsRoot = GlobalRegistry().createKeyWithName("user/ui/input", "mouseToolMappings", "user");

    foreachGroup([&] (IMouseToolGroup& g)
    {
        auto& group = static_cast<MouseToolGroup&>(g);

        auto mappingNode = mappingsRoot.createChild("mouseToolMapping");
        mappingNode.setAttributeValue("name", getToolGroupName(group.getType()));
        mappingNode.setAttributeValue("id", string::to_string(static_cast<int>(group.getType())));

        // e.g. <tool name="CameraMoveTool" button="MMB" modifiers="CONTROL" />
        group.foreachMapping([&](unsigned int state, const MouseToolPtr& tool)
        {
            auto toolNode = mappingNode.createChild("tool");

            toolNode.setAttributeValue("name", tool->getName());
            wxutil::MouseButton::SaveToNode(state, toolNode);
            wxutil::Modifier::SaveToNode(state, toolNode);
        });
    });
}

void MouseToolManager::shutdownModule()
{
    _hintCloseTimer.Stop();
    closeHintPopup();

    // Save tool mappings
    saveToolMappings();

    _mouseToolGroups.clear();
}

MouseToolGroup& MouseToolManager::getGroup(IMouseToolGroup::Type group)
{
    auto found = _mouseToolGroups.find(group);

    // Insert if not there yet
    if (found == _mouseToolGroups.end())
    {
        found = _mouseToolGroups.emplace(group, std::make_shared<MouseToolGroup>(group)).first;
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

            GlobalMouseToolManager().foreachGroup([&](IMouseToolGroup& group)
            {
                MouseToolStack tools = group.getMappedTools(testFlags);

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

    if (statusText.empty())
    {
        _hintCloseTimer.Stop();
        closeHintPopup();
        return;
    }

    // (Re-)start the timer
    _hintCloseTimer.StartOnce(HINT_POPUP_CLOSE_TIMEOUT_MSECS);
    _shouldClosePopup = false;

    // Ensure the popup exists, but we need to have a valid main frame window
    auto mainParent = GlobalMainFrame().getWxTopLevelWindow();

    if (mainParent == nullptr)
    {
        return;
    }

    if (!_hintPopup)
    {
        _hintPopup = new ModifierHintPopup(mainParent, *this);
    }

    _hintPopup->SetText(statusText);
}

void MouseToolManager::closeHintPopup()
{
    _shouldClosePopup = true;
    requestIdleCallback();
}

void MouseToolManager::onIdle()
{
    if (!_shouldClosePopup) return;

    _shouldClosePopup = false;

    if (_hintPopup)
    {
        _hintPopup->Hide();
        _hintPopup->Destroy();
        _hintPopup = nullptr;
    }
}

void MouseToolManager::onCloseTimerIntervalReached(wxTimerEvent& ev)
{
    // If no modifiers are held anymore, close the popup
    if (wxutil::Modifier::AnyModifierKeyHeldDown())
    {
        // Another round of waiting
        ev.GetTimer().StartOnce(HINT_POPUP_CLOSE_TIMEOUT_MSECS);
        return;
    }

    if (_hintPopup)
    {
        closeHintPopup();
        return;
    }
}

module::StaticModuleRegistration<MouseToolManager> mouseToolManagerModule;

} // namespace
