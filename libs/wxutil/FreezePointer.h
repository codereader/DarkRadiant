#pragma once

#include <wx/wxprec.h>
#include <wx/event.h>
#include <functional>

namespace wxutil
{

/// Utility class to capture the mouse pointer and freeze it in place, e.g. for camera
/// navigation in 2D and 3D views.
class FreezePointer: public wxEvtHandler
{
public:
    using MotionFunction = std::function<void(int, int, unsigned int state)>;
    using CaptureLostFunction = std::function<void()>;
    using MouseEventFunction = std::function<void(wxMouseEvent&)>;

private:
    // Freeze position relative to captured window
    int _freezePosX = 0;
    int _freezePosY = 0;

    // Whether to lock the cursor in its position
    bool _freezePointer = true;

    // Whether to hide the cursor during capture
    bool _hidePointer = true;

    // Whether the motion callback receives deltas or absolute coords
    bool _motionReceivesDeltas = true;

    MotionFunction _motionFunction;
    CaptureLostFunction _captureLostFunction;

    wxWindow* _capturedWindow = nullptr;

    MouseEventFunction _onMouseUp;
    MouseEventFunction _onMouseDown;

public:
    /**
     * @brief Catch any mouse pointer movements and redirect them to the given window.
     *
     * @param function Any mouse movement will be reported to the given MotionFunction.
     * @param endMove Function invoked as soon as the cursor capture is lost.
     */
    void startCapture(wxWindow* window,
                      const MotionFunction& function,
                      const CaptureLostFunction& endMove,
                      bool freezePointer = true,
                      bool hidePointer = true,
                      bool motionReceivesDeltas = true);

    /// Returns true if the mouse is currently captured by this class.
    bool isCapturing(wxWindow* window) const;

    /**
     * @brief Un-capture the cursor again.
     *
     * If the cursor was frozen, this moves it back to where it was before.
     */
    void endCapture();

    /**
     * During freeze mouse button events might be eaten by the window.
     * Use these to enable event propagation.
     */
    void connectMouseEvents(const MouseEventFunction& onMouseDown,
                            const MouseEventFunction& onMouseUp);
    void disconnectMouseEvents();

private:
    // During capture we might need to propagate the mouseup and
    // mousedown events to the client
    void onMouseUp(wxMouseEvent& ev);
    void onMouseDown(wxMouseEvent& ev);

    // The callback to connect to the motion-notify-event
    void onMouseMotion(wxMouseEvent& ev);
    void onMouseCaptureLost(wxMouseCaptureLostEvent& ev);
};

} // namespace
