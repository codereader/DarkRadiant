#pragma once

#include <map>
#include <set>
#include "imousetoolmanager.h"
#include "wxutil/MouseButton.h"

namespace ui
{

// The mouse state at the time the event occurred, can be used
// to map MouseTools to button/modifier combination.
// Can be used in std::map due to its defined operator<
class MouseState
{
private:
    // Mouse button flags, as in wxutil::MouseButton
    unsigned int _state;

public:
    MouseState() :
        _state(wxutil::MouseButton::NONE)
    {}

    explicit MouseState(unsigned int state) :
        _state(state)
    {}

    MouseState(wxMouseEvent& ev) :
        _state(wxutil::MouseButton::GetStateForMouseEvent(ev))
    {}

    MouseState(const MouseState& other) :
        _state(other._state)
    {}

    bool operator==(const MouseState& other) const
    {
        return _state == other._state;
    }

    bool operator!=(const MouseState& other) const
    {
        return !operator==(other);
    }

    bool operator<(const MouseState& other) const
    {
        return _state < other._state;
    }

    unsigned int getState() const
    {
        return _state;
    }

    void setState(unsigned int state)
    {
        _state = state;
    }
};

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
    typedef std::multimap<MouseState, MouseToolPtr> ToolMapping;
    ToolMapping _toolMapping;

public:
    MouseToolGroup(Type type);

    Type getType();

    // Add/remove mousetools from this host
    void registerMouseTool(const MouseToolPtr& tool);
    void unregisterMouseTool(const MouseToolPtr& tool);

    // Retrieval and iteration methods
    MouseToolPtr getMouseToolByName(const std::string& name);
    void foreachMouseTool(const std::function<void(const MouseToolPtr&)>& func);

    // Mapping
    MouseToolStack getMappedTools(const MouseState& state);
    void addToolMapping(const MouseState& state, const MouseToolPtr& tool);
    void foreachMapping(const std::function<void(const MouseState&, const MouseToolPtr&)>& func);
};

} // namespace
