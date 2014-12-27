#include "MouseToolGroup.h"

#include <limits>
#include "i18n.h"

namespace ui
{

MouseToolGroup::MouseToolGroup(Type type) :
    _type(type)
{}

MouseToolGroup::Type MouseToolGroup::getType()
{
    return _type;
}

std::string MouseToolGroup::getDisplayName()
{
    switch (_type)
    {
    case Type::OrthoView:
        return _("XY View");
    case Type::CameraView:
        return _("Camera View");
    default:
        return _("Unknown");
    };
}

void MouseToolGroup::registerMouseTool(const MouseToolPtr& tool)
{
    assert(_mouseTools.find(tool) == _mouseTools.end());

    _mouseTools.insert(tool);
}

void MouseToolGroup::unregisterMouseTool(const MouseToolPtr& tool)
{
    assert(_mouseTools.find(tool) != _mouseTools.end());
    _mouseTools.erase(tool);
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

MouseToolStack MouseToolGroup::getMappedTools(unsigned int state)
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

unsigned int MouseToolGroup::getMappingForTool(const MouseToolPtr& tool)
{
    for (auto i : _toolMapping)
    {
        if (i.second == tool)
        {
            return i.first;
        }
    }

    return wxutil::MouseButton::NONE | wxutil::Modifier::NONE;
}

void MouseToolGroup::addToolMapping(unsigned int state, const MouseToolPtr& tool)
{
    _toolMapping.insert(std::make_pair(state, tool));
}

void MouseToolGroup::foreachMapping(const std::function<void(unsigned int, const MouseToolPtr&)>& func)
{
    for (auto mapping : _toolMapping)
    {
        func(mapping.first, mapping.second);
    }
}

void MouseToolGroup::clearToolMappings()
{
    _toolMapping.clear();
}

void MouseToolGroup::clearToolMapping(MouseToolPtr& tool)
{
    for (ToolMapping::iterator i = _toolMapping.begin(); i != _toolMapping.end();)
    {
        if (i->second == tool)
        {
            _toolMapping.erase(i++);
        }
        else
        {
            ++i;
        }
    }
}

}