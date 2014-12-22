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

void MouseToolGroup::registerMouseTool(const MouseToolPtr& tool, int priority)
{
    while (priority < std::numeric_limits<int>::max())
    {
        if (_mouseTools.find(priority) == _mouseTools.end())
        {
            _mouseTools[priority] = tool;
            break;
        }

        ++priority;
    }
}

void MouseToolGroup::unregisterMouseTool(const MouseToolPtr& tool)
{
    for (MouseToolMap::const_iterator i = _mouseTools.begin(); i != _mouseTools.end(); ++i)
    {
        if (i->second == tool)
        {
            _mouseTools.erase(i);
            break;
        }
    }
}

MouseToolPtr MouseToolGroup::getMouseToolByName(const std::string& name)
{
    for (auto i : _mouseTools)
    {
        if (i.second->getName() == name)
        {
            return i.second;
        }
    }

    return MouseToolPtr();
}

void MouseToolGroup::foreachMouseTool(const std::function<void(const MouseToolPtr&)>& func)
{
    for (auto i : _mouseTools)
    {
        func(i.second);
    }
}

}