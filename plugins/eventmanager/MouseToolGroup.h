#pragma once

#include <map>
#include "imousetoolmanager.h"

namespace ui
{

/**
* Defines a categorised set of mousetools.
*/
class MouseToolGroup :
    public IMouseToolGroup
{
protected:
    // All MouseTools sorted by priority
    typedef std::map<int, MouseToolPtr> MouseToolMap;
    MouseToolMap _mouseTools;

    Type _type;

public:
    MouseToolGroup(Type type);

    Type getType();

    // Add/remove mousetools from this host
    void registerMouseTool(const MouseToolPtr& tool, int priority);
    void unregisterMouseTool(const MouseToolPtr& tool);

    // Retrieval and iteration methods
    MouseToolPtr getMouseToolByName(const std::string& name);
    void foreachMouseTool(const std::function<void(const MouseToolPtr&)>& func);
};

} // namespace
