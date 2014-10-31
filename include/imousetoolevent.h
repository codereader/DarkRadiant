#pragma once

#include "icameraview.h"
#include "math/Vector2.h"

namespace ui
{

// Base class for a generic event to be handled by the MouseTool implementation
class MouseToolEvent
{
private:
    // The device position, normalised within [-1..+1]
    Vector2 _devicePosition;

    // The device delta for tools using a frozen mouse pointer
    Vector2 _deviceDelta;

    IInteractiveView& _view;

public:
    MouseToolEvent(IInteractiveView& view, const Vector2& devicePosition) :
        _devicePosition(devicePosition),
        _deviceDelta(0, 0),
        _view(view)
    {}

    MouseToolEvent(IInteractiveView& view, const Vector2& devicePosition, const Vector2& delta) :
        _devicePosition(devicePosition),
        _deviceDelta(delta),
        _view(view)
    {}

    virtual ~MouseToolEvent() {}

    // Returns the mouse position in normalised device coordinates (x,y in [-1..+1])
    const Vector2& getDevicePosition() const
    {
        return _devicePosition;
    }

    // Used by MouseMove events, this contains the delta
    // in device coordinates since the last mousemove event
    const Vector2& getDeviceDelta() const
    {
        return _deviceDelta;
    }

    IInteractiveView& getInteractiveView()
    {
        return _view;
    }
};

} // namespace
