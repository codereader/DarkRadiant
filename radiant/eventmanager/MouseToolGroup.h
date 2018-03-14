#pragma once

#include <map>
#include <set>
#include "imousetoolmanager.h"
#include "wxutil/MouseButton.h"

namespace ui
{

/**
* Defines a categorised set of mousetools.
*/
class MouseToolGroup :
    public IMouseToolGroup
{
protected:
    // All MouseTools in this group
    typedef std::set<MouseToolPtr> MouseTools;
    MouseTools _mouseTools;

    // Group category (xy or cam)
    Type _type;

    // Maps Mousebutton/Modifier combinations to Tools
    typedef std::multimap<unsigned int, MouseToolPtr> ToolMapping;
    ToolMapping _toolMapping;

public:
    MouseToolGroup(Type type);

    Type getType();
    std::string getDisplayName();

    // Add/remove mousetools from this host
    void registerMouseTool(const MouseToolPtr& tool);
    void unregisterMouseTool(const MouseToolPtr& tool);

    // Retrieval and iteration methods
    MouseToolPtr getMouseToolByName(const std::string& name);
    void foreachMouseTool(const std::function<void(const MouseToolPtr&)>& func);

    // Mapping (the state variable is defined by the same flags as in wxutil::MouseButton)
    MouseToolStack getMappedTools(unsigned int state);
    unsigned int getMappingForTool(const MouseToolPtr& tool);
    void addToolMapping(unsigned int state, const MouseToolPtr& tool);
    void foreachMapping(const std::function<void(unsigned int, const MouseToolPtr&)>& func);
    void clearToolMappings();
    void clearToolMapping(MouseToolPtr& tool);
};
typedef std::shared_ptr<MouseToolGroup> MouseToolGroupPtr;

} // namespace
