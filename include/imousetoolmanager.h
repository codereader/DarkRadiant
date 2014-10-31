#pragma once

#include "imousetool.h"
#include <functional>

class wxMouseEvent;

namespace ui
{

/**
 * An interface to an object which is able to hold one or more mousetool 
 * instances. It provides methods to register and unregister tools
 * as well as retrieving them when the actual UI event occurs.
 *
 * Implementations of IMouseToolManager are GlobalXYWnd and GlobalCamera, for instance.
 */
class IMouseToolManager
{
public:
    virtual ~IMouseToolManager() {}

    // Add a MouseTool with the given priority
    // If the exact priority is already in use by another tool, 
    // the given tool will be sorted AFTER the existing one.
    virtual void registerMouseTool(const MouseToolPtr& tool, int priority) = 0;

    // Removes the given mousetool from this host.
    virtual void unregisterMouseTool(const MouseToolPtr& tool) = 0;

    // Returns the named mouse tool (case sensitive) or NULL if not found.
    virtual MouseToolPtr getMouseToolByName(const std::string& name) = 0;

    // Visit each mouse tool with the given function object
    virtual void foreachMouseTool(const std::function<void(const MouseToolPtr&)>& func) = 0;

    // Returns a set of mouse tools suitable for the given UI event
    virtual MouseToolStack getMouseToolStackForEvent(wxMouseEvent& ev) = 0;
};

} // namespace
