#include "MouseToolGroup.h"

#include <limits>

namespace ui
{

MouseToolGroup::MouseToolGroup(Type type) :
    _type(type)
{}

MouseToolGroup::Type MouseToolGroup::getType()
{
    return _type;
}

void MouseToolGroup::registerMouseTool(const MouseToolPtr& tool)
{
    assert(_mouseTools.find(tool) == _mouseTools.end());

    _mouseTools.insert(tool);
#if 0
    while (priority < std::numeric_limits<int>::max())
    {
        if (_mouseTools.find(priority) == _mouseTools.end())
        {
            _mouseTools[priority] = tool;
            break;
        }

        ++priority;
    }
#endif
}

void MouseToolGroup::unregisterMouseTool(const MouseToolPtr& tool)
{
    _mouseTools.erase(tool);
#if 0
    for (MouseToolMap::const_iterator i = _mouseTools.begin(); i != _mouseTools.end(); ++i)
    {
        if (i->second == tool)
        {
            _mouseTools.erase(i);
            break;
        }
    }
#endif
}

MouseToolPtr MouseToolGroup::getMouseToolByName(const std::string& name)
{
    for (auto tool : _mouseTools)
    {
        if (tool->getName() == name)
        {
            return tool;
        }
    }

    return MouseToolPtr();
}

void MouseToolGroup::foreachMouseTool(const std::function<void(const MouseToolPtr&)>& func)
{
    for (auto tool : _mouseTools)
    {
        func(tool);
    }
}

MouseToolPtr MouseToolGroup::getMappedTool(const MouseState& state)
{
    ToolMapping::const_iterator found = _toolMapping.find(state);

    return found != _toolMapping.end() ? found->second : MouseToolPtr();
}

void MouseToolGroup::setToolMapping(const MouseToolPtr& tool, const MouseState& state)
{
    // Remove any other mapping for the same tool
    for (ToolMapping::iterator i = _toolMapping.begin(); i != _toolMapping.end();)
    {
        if (i->first != state && i->second == tool)
        {
            _toolMapping.erase(i++);
        }
        else
        {
            ++i;
        }
    }

    _toolMapping[state] = tool;
}

}