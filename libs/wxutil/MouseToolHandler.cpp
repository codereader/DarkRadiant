#include "MouseToolHandler.h"

#include "itextstream.h"
#include "MouseButton.h"
#include "imainframe.h"

namespace wxutil
{

MouseToolHandler::MouseToolHandler(ui::IMouseToolGroup::Type type) :
    _type(type)
{}

void MouseToolHandler::onGLMouseButtonPress(wxMouseEvent& ev)
{
    // Filter out the button that was triggering this event
    unsigned int state = wxutil::MouseButton::GetButtonStateChangeForMouseEvent(ev);

    // For the active tool mapping, we need just the mouse button without modifiers
    unsigned int button = wxutil::MouseButton::GetButtonStateChangeForMouseEvent(ev) & wxutil::MouseButton::ALL_BUTTON_MASK;

    ui::MouseToolPtr activeToolToBeCleared;

    if (_activeMouseTools.find(button) != _activeMouseTools.end())
    {
        // Send the click event to the currently active tool. Some tools stay active after
        // mouse up and might choose to end their action along with the mouse down event
        // The FreeMoveTool with toggle mode activated is such an example.
        ui::MouseToolPtr tool = _activeMouseTools[button];

        switch (processMouseDownEvent(tool, Vector2(ev.GetX(), ev.GetY())))
        {
        case ui::MouseTool::Result::Finished:
            // Tool is done
            activeToolToBeCleared = tool;
            break;

        case ui::MouseTool::Result::Activated:
        case ui::MouseTool::Result::Continued:
            handleViewRefresh(tool->getRefreshMode());
            break;

        case ui::MouseTool::Result::Ignored:
            break;
        };
    }

    // Now consider all the available tools and send the event
    // Currently active tools are handled above, so don't send the event again

    // Get all mouse tools mapped to this button/modifier combination
    ui::MouseToolStack toolStack = GlobalMouseToolManager().getMouseToolsForEvent(_type, state);

    // Remove all active mouse tools from this stack
    toolStack.remove_if(std::bind(&MouseToolHandler::toolIsActive, this, std::placeholders::_1));

    // The candidates have been trimmed, so let's clear out any pending tools
    if (activeToolToBeCleared)
    {
        clearActiveMouseTool(activeToolToBeCleared);
        activeToolToBeCleared.reset();
    }

    // Check which one of the candidates responds to the mousedown event
    ui::MouseToolPtr activeTool;

    for (ui::MouseToolPtr tool : toolStack)
    {
        // Ask each tool to handle the event
        ui::MouseTool::Result result = processMouseDownEvent(tool, Vector2(ev.GetX(), ev.GetY()));

        if (result != ui::MouseTool::Result::Ignored && result != ui::MouseTool::Result::Finished)
        {
            // This tool is now activated
            activeTool = tool;
            break;
        }
    }

    if (!activeTool)
    {
        return;
    }

    // Store this tool in our map
    _activeMouseTools[button] = activeTool;

    unsigned int pointerMode = activeTool->getPointerMode();

    // Check if the mousetool requires pointer freeze
    if ((pointerMode & ui::MouseTool::PointerMode::Capture) != 0)
    {
        startCapture(activeTool);
    }

    if (!_escapeListener)
    {
        // Register a hook to capture the ESC key during the active phase
        _escapeListener.reset(new KeyEventFilter(WXK_ESCAPE,
            std::bind(&MouseToolHandler::handleEscapeKeyPress, this)));
    }
}

void MouseToolHandler::onGLMouseMove(wxMouseEvent& ev)
{
    // Skip this event if any of the active mouse tools is in capture mode
    // the call here still arrives on OSX even during capture
    for (const ActiveMouseTools::value_type& pair : _activeMouseTools)
    {
        if (pair.second->getPointerMode() & ui::MouseTool::PointerMode::Capture)
        {
            return; // skip
        }
    }
    
    Vector2 position(ev.GetX(), ev.GetY());

    sendMoveEventToInactiveTools(ev.GetX(), ev.GetY());

    // Pass the move event to all active tools and clear the ones that are done
    for (ActiveMouseTools::const_iterator i = _activeMouseTools.begin();
         i != _activeMouseTools.end();)
    {
        ui::MouseToolPtr tool = (i++)->second;

        // Ask the active mousetool to handle this event
        switch (processMouseMoveEvent(tool, ev.GetX(), ev.GetY()))
        {
        case ui::MouseTool::Result::Finished:
            // Tool is done
            clearActiveMouseTool(tool);
            handleViewRefresh(tool->getRefreshMode());
            break;

        case ui::MouseTool::Result::Activated:
        case ui::MouseTool::Result::Continued:
            handleViewRefresh(tool->getRefreshMode());
            break;

        case ui::MouseTool::Result::Ignored:
            break;
        };
    }
}

void MouseToolHandler::onGLCapturedMouseMove(int x, int y, unsigned int mouseState)
{
    sendMoveEventToInactiveTools(x, y);

    for (ActiveMouseTools::const_iterator i = _activeMouseTools.begin(); i != _activeMouseTools.end();)
    {
        ui::MouseToolPtr tool = (i++)->second;

        switch (processMouseMoveEvent(tool, x, y))
        {
        case ui::MouseTool::Result::Finished:
            // Tool is done
            clearActiveMouseTool(tool);
            handleViewRefresh(tool->getRefreshMode());
            break;

        case ui::MouseTool::Result::Activated:
        case ui::MouseTool::Result::Continued:
            handleViewRefresh(tool->getRefreshMode());
            break;

        case ui::MouseTool::Result::Ignored:
            break;
        };
    }
}

bool MouseToolHandler::toolIsActive(const ui::MouseToolPtr& tool)
{
    // The active tools don't count
    for (const ActiveMouseTools::value_type& i : _activeMouseTools)
    {
        if (i.second == tool) return true;
    }

    return false;
}

void MouseToolHandler::sendMoveEventToInactiveTools(int x, int y)
{
    // Send mouse move events to all tools that want them
    GlobalMouseToolManager().getGroup(_type).foreachMouseTool([&] (const ui::MouseToolPtr& tool)
    {
        if (!tool->alwaysReceivesMoveEvents()) return;
        
        // The active tools don't receive this event a second time
        if (toolIsActive(tool)) return;

        processMouseMoveEvent(tool, x, y);
    });
}

void MouseToolHandler::onGLMouseButtonRelease(wxMouseEvent& ev)
{
    if (_activeMouseTools.empty()) return;

    // Determine the button that has been released
    unsigned int state = wxutil::MouseButton::GetButtonStateChangeForMouseEvent(ev) & wxutil::MouseButton::ALL_BUTTON_MASK;

    ActiveMouseTools::const_iterator i = _activeMouseTools.find(state);

    if (i != _activeMouseTools.end())
    {
        // Ask the active mousetool to handle this event
        ui::MouseTool::Result result = processMouseUpEvent(i->second, Vector2(ev.GetX(), ev.GetY()));

        if (result == ui::MouseTool::Result::Finished)
        {
            clearActiveMouseTool(i->second);
        }
    }
}

void MouseToolHandler::handleCaptureLost(const ui::MouseToolPtr& tool)
{
    if (!tool) return;

    if (tool->getPointerMode() & ui::MouseTool::PointerMode::Capture)
    {
        // Send the capture lost event, which should make the tool cancel the operation
        tool->onMouseCaptureLost(getInteractiveView());

		handleViewRefresh(tool->getRefreshMode());

        // Clear the tool when the capture is lost
        clearActiveMouseTool(tool);
    }
}

void MouseToolHandler::clearActiveMouseTool(const ui::MouseToolPtr& tool)
{
    unsigned int previousPointerMode = tool->getPointerMode();

    for (ActiveMouseTools::const_iterator i = _activeMouseTools.begin(); i != _activeMouseTools.end(); ++i)
    {
        if (i->second == tool)
        {
            _activeMouseTools.erase(i);
            break;
        }
    }

    // Check if any mouse tools still require capture mode 
    unsigned int remainingPointerMode = ui::MouseTool::PointerMode::Normal;

    for (const ActiveMouseTools::value_type& pair : _activeMouseTools)
    {
        remainingPointerMode |= pair.second->getPointerMode();
    }

    // If no more freezing mouse tools: release the mouse cursor again
    if (previousPointerMode & ui::MouseTool::PointerMode::Capture && 
        !(remainingPointerMode & ui::MouseTool::PointerMode::Capture))
    {
        endCapture();
    }

    // Reset the escape listener if this was the last active tool
    if (_activeMouseTools.empty())
    {
        _escapeListener.reset();
    }
}

void MouseToolHandler::clearActiveMouseTool(unsigned int button)
{
    if (_activeMouseTools.find(button) != _activeMouseTools.end())
    {
        clearActiveMouseTool(_activeMouseTools[button]);
    }
}

void MouseToolHandler::clearActiveMouseTools()
{
    // Reset the escape listener
    _escapeListener.reset();

    if (_activeMouseTools.empty())
    {
        return;
    }

    // Check the capture mode
    unsigned int pointerMode = ui::MouseTool::PointerMode::Normal;

    // Freezing mouse tools: release the mouse cursor again
    for (ActiveMouseTools::const_iterator i = _activeMouseTools.begin(); i != _activeMouseTools.end();)
    {
        pointerMode |= i->second->getPointerMode();

        // Tool is done
        _activeMouseTools.erase(i++);
    }

    // If any of the active tools was capturing, end this now
    if (pointerMode & ui::MouseTool::PointerMode::Capture)
    {
        endCapture();
    }
}

KeyEventFilter::Result MouseToolHandler::handleEscapeKeyPress()
{
    // Key will slip through unless one tool reports having it processed
    KeyEventFilter::Result result = KeyEventFilter::Result::KeyIgnored;

    for (ActiveMouseTools::const_iterator i = _activeMouseTools.begin(); i != _activeMouseTools.end();)
    {
        ui::MouseToolPtr tool = (i++)->second;

        switch (tool->onCancel(getInteractiveView()))
        {
        // Any MouseTool returning the Finished signal will be removed
        case ui::MouseTool::Result::Finished:
            // Tool is done
            clearActiveMouseTool(tool);
			handleViewRefresh(tool->getRefreshMode());
            result = KeyEventFilter::Result::KeyProcessed;
            break;

        case ui::MouseTool::Result::Activated:
        case ui::MouseTool::Result::Continued:
        case ui::MouseTool::Result::Ignored:
            break;
        };
    }

    return result;
}

void MouseToolHandler::handleViewRefresh(unsigned int flags)
{
    if (flags & ui::MouseTool::RefreshMode::AllViews)
    {
        // Pass the signal to the mainframe
        GlobalMainFrame().updateAllWindows((flags & ui::MouseTool::RefreshMode::Force) != 0);
    }
    else if (flags & ui::MouseTool::RefreshMode::ActiveView)
    {
        if (flags & ui::MouseTool::RefreshMode::Force)
        {
            getInteractiveView().forceRedraw();
        }
        else
        {
            getInteractiveView().queueDraw();
        }
    }
}

}
