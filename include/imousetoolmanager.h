#pragma once

#include "imousetool.h"
#include "imodule.h"
#include <list>
#include <functional>
#include <memory>

namespace ui
{

// A list of mousetools
class MouseToolStack :
    public std::list<MouseToolPtr>
{
public:
    // Tries to handle the given event, returning the first tool that responded positively
    MouseToolPtr handleMouseDownEvent(MouseTool::Event& mouseEvent)
    {
        for (const_iterator i = begin(); i != end(); ++i)
        {
            // Ask each tool to handle the event
            MouseTool::Result result = (*i)->onMouseDown(mouseEvent);

            if (result != MouseTool::Result::Ignored && result != MouseTool::Result::Finished)
            {
                // This tool is now activated
                return *i;
            }
        }

        return MouseToolPtr();
    }
};

// A set of MouseTools, use the GlobalMouseToolManager() to get access
class IMouseToolGroup
{
public:
    virtual ~IMouseToolGroup() {}

    // MouseTools categories
    enum class Type
    {
        OrthoView   = 0,
        CameraView  = 1,
    };

    // Returns the type of this group
    virtual Type getType() = 0;

    // Returns the display name of this group
    virtual std::string getDisplayName() = 0;

    // Add a MouseTool to the given group
    virtual void registerMouseTool(const MouseToolPtr& tool) = 0;

    // Removes the given mousetool from all categories.
    virtual void unregisterMouseTool(const MouseToolPtr& tool) = 0;

    // Returns the named mouse tool (case sensitive) or NULL if not found.
    virtual MouseToolPtr getMouseToolByName(const std::string& name) = 0;

    // Visit each mouse tool with the given function object
    virtual void foreachMouseTool(const std::function<void(const MouseToolPtr&)>& func) = 0;

    // Returns the set of mapped tools for this mouse event. The unsigned int is a bitmap 
    // using the same flags as used in wxutil::MouseButton
    virtual MouseToolStack getMappedTools(unsigned int state) = 0;

    // Returns the mapping for the given mousetool
    virtual unsigned int getMappingForTool(const MouseToolPtr& tool) = 0;

    // Map a tool to a defined mouse state (as used in wxutil::MouseButton)
    // It's possible to map the same tool to multiple mouse states
    virtual void addToolMapping(unsigned int state, const MouseToolPtr& tool) = 0;

    // Iterate over each tool mapping
    virtual void foreachMapping(const std::function<void(unsigned int, const MouseToolPtr&)>& func) = 0;

    // Removes all mappings for all tools
    virtual void clearToolMappings() = 0;

    // Removes all mappings for the given tool
    virtual void clearToolMapping(MouseToolPtr& tool) = 0;
};
typedef std::shared_ptr<IMouseToolGroup> IMouseToolGroupPtr;

/**
 * An interface to an object which is able to hold one or more mousetool 
 * groups. 
 *
 * Clients of IMouseToolManager are GlobalXYWnd and GlobalCamera, for instance.
 */
class IMouseToolManager :
    public RegisterableModule
{
public:
    virtual ~IMouseToolManager() {}

    // Get the group defined by the given enum. This always succeeds, if the group
    // is not existing yet, a new one will be created internally.
    virtual IMouseToolGroup& getGroup(IMouseToolGroup::Type group) = 0;

    // Iterate over each group using the given visitor function
    virtual void foreachGroup(const std::function<void(IMouseToolGroup&)>& functor) = 0;

    // Returns matching MouseTools for the given event and group type. To convert a wxMouseEvent to
    // the unsigned int mouseState, use wxutil::MouseButton
    virtual MouseToolStack getMouseToolsForEvent(IMouseToolGroup::Type group, unsigned int mouseState) = 0;

    // Prints the list of possible mouse actions to the statusbar
    virtual void updateStatusbar(unsigned int mouseState) = 0;

    // Reverts all mappings to the defaults as defined in the stock input.xml
    virtual void resetBindingsToDefault() = 0;
};

} // namespace

const char* const MODULE_MOUSETOOLMANAGER = "MouseToolManager";

inline ui::IMouseToolManager& GlobalMouseToolManager()
{
    // Cache the reference locally
    static ui::IMouseToolManager& _mtManager(
        *std::static_pointer_cast<ui::IMouseToolManager>(
            module::GlobalModuleRegistry().getModule(MODULE_MOUSETOOLMANAGER)
        )
    );
    return _mtManager;
}
