#pragma once

#include "imousetool.h"
#include "imousetoolmanager.h"
#include <map>
#include <wx/event.h>
#include "event/KeyEventFilter.h"

namespace wxutil
{

/**
 * A helper class containing the mousetool handling logic.
 * Interactive Views can have several active mouse tools 
 * at the same time, need support for ESC/cancelling the operations
 * as well as capturing the mouse pointer.
 */
class MouseToolHandler
{
private:
    ui::IMouseToolGroup::Type _type;

protected:
    // One active tool is possible for each button
    typedef std::map<unsigned int, ui::MouseToolPtr> ActiveMouseTools;
    ActiveMouseTools _activeMouseTools;

private:
    // During active phases we listen for ESC keys to cancel the operation
    KeyEventFilterPtr _escapeListener;

public:
    MouseToolHandler(ui::IMouseToolGroup::Type type);

    void onGLMouseButtonPress(wxMouseEvent& ev);
    void onGLMouseButtonRelease(wxMouseEvent& ev);
    void onGLMouseMove(wxMouseEvent& ev);
    void onGLCapturedMouseMove(int x, int y, unsigned int mouseState);

    void handleCaptureLost(const ui::MouseToolPtr& tool);

protected:
    virtual ui::MouseTool::Result processMouseDownEvent(const ui::MouseToolPtr& tool, const Vector2& point) = 0;
    virtual ui::MouseTool::Result processMouseUpEvent(const ui::MouseToolPtr& tool, const Vector2& point) = 0;
    virtual ui::MouseTool::Result processMouseMoveEvent(const ui::MouseToolPtr& tool, int x, int y) = 0;

    virtual void startCapture(const ui::MouseToolPtr& tool) = 0;
    virtual void endCapture() = 0;

    virtual IInteractiveView& getInteractiveView() = 0;

    void clearActiveMouseTool(const ui::MouseToolPtr& tool);
    void clearActiveMouseTool(unsigned int button);
    void clearActiveMouseTools();

private:
    void sendMoveEventToInactiveTools(int x, int y);

    void handleViewRefresh(unsigned int flags);

    bool toolIsActive(const ui::MouseToolPtr& tool);

    KeyEventFilter::Result handleEscapeKeyPress();
};

}
