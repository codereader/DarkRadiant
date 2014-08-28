#pragma once

#include <string>
#include <memory>

namespace ui
{

/** 
 * A Tool class represents an operator which can be "used" 
 * in DarkRadiant's Ortho- and Camera views by using the mouse.
 */
class MouseTool
{
public:
    struct Event
    {
        enum class Type
        {
            MouseDown,
            MouseUp,
            MouseMove,
        };

        // Current mouse coordinates, relative to 0,0,0 world origin
        Vector3 worldPos;
        Type type;

        Event(Type type_, const Vector3& worldPos_) :
            type(type),
            worldPos(worldPos_)
        {}
    };

    // Returns the name of this operation. This name is only used
    // internally and should be unique.
    virtual const std::string& getName() = 0;

    virtual void onMouseEvent(Event& ev) = 0;
};
typedef std::shared_ptr<MouseTool> MouseToolPtr;

} // namespace
