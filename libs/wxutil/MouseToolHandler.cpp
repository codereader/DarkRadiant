#include "MouseToolHandler.h"

#include "itextstream.h"
#include "MouseButton.h"

namespace wxutil
{

MouseToolHandler::MouseToolHandler(ui::IMouseToolGroup::Type type) :
    _type(type)
{}

void MouseToolHandler::onGLMouseButtonPress(wxMouseEvent& ev)
{
    // Filter out the button that was triggering this event
    unsigned int state = wxutil::MouseButton::GetButtonStateChangeForMouseEvent(ev);

    // Get all mouse tools mapped to this button/modifier combination
    ui::MouseToolStack toolStack = GlobalMouseToolManager().getMouseToolsForEvent(_type, state);

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

    // Get the actual mouse button we pressed here
    unsigned int button = wxutil::MouseButton::GetButtonStateChangeForMouseEvent(ev) & wxutil::MouseButton::ALL_BUTTON_MASK;

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
        _escapeListener.reset(new KeyEventFilter(WXK_ESCAPE, [&]()
        {
            for (ActiveMouseTools::value_type& i : _activeMouseTools)
            {
                try
                {
                    i.second->onCancel(dynamic_cast<IInteractiveView&>(*this));
                }
                catch (std::bad_cast&)
                {
                    rError() << "Cannot cancel mouse tool, unable to cast to interactive view!" << std::endl;
                }
            }

            // This also removes the active escape listener
            clearActiveMouseTools();
        }));
    }
}

void MouseToolHandler::onGLMouseMove(wxMouseEvent& ev)
{
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
            break;

        case ui::MouseTool::Result::Activated:
        case ui::MouseTool::Result::Continued:
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

        if (processMouseMoveEvent(tool, x, y) == ui::MouseTool::Result::Finished)
        {
            clearActiveMouseTool(tool);
        }
    }
}

void MouseToolHandler::sendMoveEventToInactiveTools(int x, int y)
{
    // Send mouse move events to all tools that want them
    GlobalMouseToolManager().getGroup(_type).foreachMouseTool([&] (const ui::MouseToolPtr& tool)
    {
        if (!tool->alwaysReceivesMoveEvents()) return;
        
        // The active tools don't count
        for (const ActiveMouseTools::value_type& i : _activeMouseTools)
        {
            if (i.second == tool) return;
        }

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

}
