#pragma once

#include <map>
#include "imousetoolmanager.h"

namespace ui
{

/**
 * Provides a base implementation of the IMouseToolManager interface.
 * This is used by the GlobalXYWnd and GlobalCamera instances.
 */
class MouseToolManagerBase :
    public virtual IMouseToolManager
{
protected:
    // All MouseTools sorted by priority
    typedef std::map<int, MouseToolPtr> MouseToolMap;
    MouseToolMap _mouseTools;

public:
    // Add/remove mousetools from this host
    virtual void registerMouseTool(const MouseToolPtr& tool, int priority);
    virtual void unregisterMouseTool(const MouseToolPtr& tool);

    // Retrieval and iteration methods
    virtual MouseToolPtr getMouseToolByName(const std::string& name);
    virtual void foreachMouseTool(const std::function<void(const MouseToolPtr&)>& func);
};

}
