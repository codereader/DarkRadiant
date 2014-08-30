#pragma once

#include <string>
#include <memory>
#include <list>

namespace ui
{

/** 
 * A Tool class represents an operator which can be "used" 
 * in DarkRadiant's Ortho- and Camera views by using the mouse.
 */
class MouseTool
{
public:
    class Event
    {
    public:
        // The view type this event is called on
        enum class ViewType
        {
            XY,
            XZ,
            YZ,
            CAMERA,
        };

    private:
        // Current mouse coordinates, relative to 0,0,0 world origin
        Vector3 _worldPos;

        ViewType _viewType;

    public:
        Event(const Vector3& worldPos, ViewType viewType) :
            _worldPos(worldPos),
            _viewType(viewType)
        {}

        const Vector3& getWorldPos() const
        {
            return _worldPos;
        }

        ViewType getViewType() const
        {
            return _viewType;
        }
    };

    // Returns the name of this operation. This name is only used
    // internally and should be unique.
    virtual const std::string& getName() = 0;

    virtual bool onMouseDown(Event& ev) = 0;
    virtual bool onMouseMove(Event& ev) = 0;
    virtual bool onMouseUp(Event& ev) = 0;
};
typedef std::shared_ptr<MouseTool> MouseToolPtr;

// A list of mousetools
class MouseToolStack :
    public std::list<MouseToolPtr>
{
public:
    // Tries to handle the given event, returning the first tool that responded positively
    MouseToolPtr handleMouseDownEvent(MouseTool::Event& mouseEvent)
    {
        for (const_iterator i = begin(); i != end(); ++i)
        {
            // Ask each tool to handle the event
            if ((*i)->onMouseDown(mouseEvent))
            {
                // This tool handled the request, set it as active tool
                return *i;
            }
        }

        return MouseToolPtr();
    }
};

} // namespace
