#pragma once

#include "imousetool.h"
#include "imodule.h"
#include <functional>
#include <boost/shared_ptr.hpp>

class wxMouseEvent;

namespace ui
{

// A set of MouseTools, use the GlobalMouseToolManager() to get access
class IMouseToolGroup
{
public:
    virtual ~IMouseToolGroup() {}

    // MouseTools categories
    enum class Type
    {
        OrthoView,
        CameraView,
    };

    // Returns the type of this group
    virtual Type getType() = 0;

    // Add a MouseTool with the given priority to the given group
    // If the exact priority is already in use by another tool, 
    // the given tool will be sorted AFTER the existing one.
    virtual void registerMouseTool(const MouseToolPtr& tool, int priority) = 0;

    // Removes the given mousetool from all categories.
    virtual void unregisterMouseTool(const MouseToolPtr& tool) = 0;

    // Returns the named mouse tool (case sensitive) or NULL if not found.
    virtual MouseToolPtr getMouseToolByName(const std::string& name) = 0;

    // Visit each mouse tool with the given function object
    virtual void foreachMouseTool(const std::function<void(const MouseToolPtr&)>& func) = 0;
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

    // Returns matching MouseTools for the given event and group type
    virtual MouseToolStack getMouseToolStackForEvent(IMouseToolGroup::Type group, wxMouseEvent& ev) = 0;
};

} // namespace

const char* const MODULE_MOUSETOOLMANAGER = "MouseToolManager";

inline ui::IMouseToolManager& GlobalMouseToolManager()
{
    // Cache the reference locally
    static ui::IMouseToolManager& _mtManager(
        *boost::static_pointer_cast<ui::IMouseToolManager>(
            module::GlobalModuleRegistry().getModule(MODULE_MOUSETOOLMANAGER)
        )
    );
    return _mtManager;
}
