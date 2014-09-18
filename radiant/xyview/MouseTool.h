#pragma once

#include <string>
#include <memory>
#include <list>
#include "MouseToolEvent.h"

namespace ui
{

/** 
 * A Tool class represents an operator which can be "used" 
 * in DarkRadiant's Ortho- and Camera views by using the mouse.
 */
class MouseTool
{
public:
    typedef MouseToolEvent Event;

    enum class Result
    {
        Ignored,    // event does not apply for this tool
        Activated,  // event handled, tool is now active
        Continued,  // event handled, tool continues to be active
        Finished,   // event handled, tool no longer active
    };

    // Returns the name of this operation. This name is only used
    // internally and should be unique.
    virtual const std::string& getName() = 0;

    virtual Result onMouseDown(Event& ev) = 0;
    virtual Result onMouseMove(Event& ev) = 0;
    virtual Result onMouseUp(Event& ev) = 0;

    // Some tools might want to receive mouseMove events even when they
    // are not active, to send feedback to the user before the buttons
    // are pressed. The Clipper tool uses this to change the mouse cursor
    // to a crosshair when moved over a manipulatable clip point.
    virtual bool alwaysReceivesMoveEvents()
    {
        return false;
    }
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
            MouseTool::Result result = (*i)->onMouseDown(mouseEvent);

            if (result != MouseTool::Result::Ignored && result != MouseTool::Result::Finished)
            {
                // This tool is now activated
                return *i;
            }
        }

        return MouseToolPtr();
    }
};

} // namespace
