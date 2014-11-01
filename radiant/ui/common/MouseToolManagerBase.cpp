#include "MouseToolManagerBase.h"

#include <limits>

namespace ui
{

void MouseToolManagerBase::registerMouseTool(const MouseToolPtr& tool, int priority)
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

void MouseToolManagerBase::unregisterMouseTool(const MouseToolPtr& tool)
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

MouseToolPtr MouseToolManagerBase::getMouseToolByName(const std::string& name)
{
    for (MouseToolMap::const_iterator i = _mouseTools.begin(); i != _mouseTools.end(); ++i)
    {
        if (i->second->getName() == name)
        {
            return i->second;
        }
    }

    return MouseToolPtr();
}

void MouseToolManagerBase::foreachMouseTool(const std::function<void(const MouseToolPtr&)>& func)
{
    std::for_each(_mouseTools.begin(), _mouseTools.end(), [&](const MouseToolMap::value_type& pair)
    {
        func(pair.second);
    });
}

}
