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

MouseToolStack MouseToolGroup::getMappedTools(const MouseState& state)
{
    MouseToolStack stack;

    for (ToolMapping::const_iterator i = _toolMapping.find(state);
         i != _toolMapping.upper_bound(state) && i != _toolMapping.end();
         ++i)
    {
        stack.push_back(i->second);
    }

    return stack;
}

void MouseToolGroup::addToolMapping(const MouseState& state, const MouseToolPtr& tool)
{
    _toolMapping.insert(std::make_pair(state, tool));
}

void MouseToolGroup::foreachMapping(const std::function<void(const MouseState&, const MouseToolPtr&)>& func)
{
    for (auto mapping : _toolMapping)
    {
        func(mapping.first, mapping.second);
    }
}

}