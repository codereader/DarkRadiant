#pragma once

#include <string>
#include <memory>
#include <list>
#include "imousetoolevent.h"

class RenderSystem;
class IRenderableCollector;
class VolumeTest;

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

    // The display name, which is also used in the status bar
    virtual const std::string& getDisplayName() = 0;

    virtual Result onMouseDown(Event& ev) = 0;
    virtual Result onMouseMove(Event& ev) = 0;
    virtual Result onMouseUp(Event& ev) = 0;

    // During an active operation the user may hit ESC,
    // in which case the onCancel event will be fired.
    // This should not be ignored by the tool, which should
    // seek to shut down any ongoing operation safely, unless it actually
    // wants to pass the key through and stay active (e.g. FreeMoveTool).
    // Classes returning Finished to this call will have the tool cleared.
    virtual Result onCancel(IInteractiveView&)
    {
        // Default behaviour is to remove this tool once ESC is encountered
        return Result::Finished;
    }

    // A tool using pointer mode Capture might want to get notified
    // when the mouse capture of the window has been lost due to 
    // the user alt-tabbing out of the app or something else.
    // Any tools using PointerMode::Capture must watch out for this event.
    virtual void onMouseCaptureLost(IInteractiveView& view)
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

    // A tool can request special mouse capture modes during its active phase
    // All flags can be combined, use Normal to indicate that no capturing is needed.
    struct PointerMode
    {
        enum Flags
        {
            Normal       = 0,   // no capturing, absolute coordinates are sent to onMouseMove, pointer will be shown
            Capture      = 1,   // capture mouse (tools should implement onMouseCaptureLost), see also MotionDeltas
            Freeze       = 2,   // pointer will be frozen and kept at the same position
            Hidden       = 4,   // pointer will be hidden
            MotionDeltas = 8,   // onMouseMove will receive delta values relative to the capture position
        };
    };

    // Some tools might want to capture the pointer after onMouseDown
    // Override this method to return "Capture" instead of "Normal".
    virtual unsigned int getPointerMode()
    {
        return PointerMode::Normal;
    }

    // Bitmask determining which view are refreshed in which way
    // after each click and mouse pointer movement event
    struct RefreshMode
    {
        enum Flags
        {
            NoRefresh       = 0,            // don't refresh anything
            Queue           = 1 << 0,       // queue a redraw (will be painted as soon as the app is idle)
            Force           = 1 << 1,       // force a redraw
            ActiveView      = 1 << 2,       // refresh the active view only (the one the mouse tool has been activated on)
            AllViews        = 1 << 3,       // refresh all available views
        };
    };

    virtual unsigned int getRefreshMode()
    {
        // By default, force a refresh of the view the tool is active on
        return RefreshMode::Force | RefreshMode::ActiveView;
    }

    // Optional render routine that is invoked after the scene
    // has been drawn on the interactive window. The projection
    // and modelview matrix have already been set up for  
    // overlay rendering (glOrtho).
    virtual void renderOverlay()
    {}

	// For in-scene rendering of active mousetools they need implement this method.
	// Any needed shaders should be acquired on-demand from the attached rendersystem.
	// Renderable objects need to be submitted to the given RenderableCollector.
	virtual void render(RenderSystem& renderSystem, IRenderableCollector& collector, const VolumeTest& volume)
	{}
};
typedef std::shared_ptr<MouseTool> MouseToolPtr;

} // namespace
