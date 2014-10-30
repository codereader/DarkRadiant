#pragma once

#include <string>
#include <memory>
#include <list>
#include "imousetoolevent.h"

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

    // During an active operation the user may hit ESC,
    // in which case the cancel event will be fired.
    // This should not be ignored by the tool, which should
    // seek to shut down any ongoing operation safely.
    virtual void onCancel()
    {}

    // A tool using pointer mode Capture might want to get notified
    // when the mouse capture of the window has been lost due to 
    // the user alt-tabbing out of the app or something else.
    virtual void onMouseCaptureLost()
    {}

    // Some tools might want to receive mouseMove events even when they
    // are not active, to send feedback to the user before the buttons
    // are pressed. The Clipper tool uses this to change the mouse cursor
    // to a crosshair when moved over a manipulatable clip point.
    virtual bool alwaysReceivesMoveEvents()
    {
        return false;
    }

    // By default, when the user is dragging the mouse to the borders of
    // the view, the viewport will be moved along. For some tools this might
    // not be desirable, in which case they need to override this method to
    // return false.
    virtual bool allowChaseMouse()
    {
        return true;
    }

    // A tool using "Capture" pointer mode will cause the
    // mouse pointer to be frozen after onMouseDown and device deltas will
    // be sent to the onMouseMove() event instead of world coordinates.
    enum class PointerMode
    {
        Normal,
        Capture,
    };

    // Some tools might want to capture the pointer after onMouseDown
    // Override this method to return "Capture" instead of "Normal".
    virtual PointerMode getPointerMode()
    {
        return PointerMode::Normal;
    }

    // Optional render routine that is invoked after the scene
    // has been drawn on the interactive window. The projection
    // and modelview matrix have already been set up for  
    // overlay rendering (glOrtho).
    virtual void renderOverlay()
    {}
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
