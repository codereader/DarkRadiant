#pragma once

#include "iorthoview.h"

namespace ui
{

// Base class for a generic event to be handled by the MouseTool implementation
class MouseToolEvent
{
private:
    // Current mouse coordinates, relative to 0,0,0 world origin
    Vector3 _worldPos;

    // The device delta for tools using a frozen mouse pointer
    Vector2 _delta;

public:
    MouseToolEvent(const Vector3& worldPos) :
        _worldPos(worldPos),
        _delta(0, 0)
    {}

    MouseToolEvent(const Vector3& worldPos, const Vector2& delta) :
        _worldPos(worldPos),
        _delta(delta)
    {}

    virtual ~MouseToolEvent() {}

    const Vector3& getWorldPos() const
    {
        return _worldPos;
    }

    // Used by MouseMove events, this contains the delta
    // in device coordinates since the last mousemove event
    const Vector2& getDeviceDelta() const
    {
        return _delta;
    }
};

// Special event subtype for XY View events. Provides
// information about view type, for example.
class XYMouseToolEvent :
    public MouseToolEvent
{
private:
    IOrthoView& _view;

public:
    XYMouseToolEvent(IOrthoView& view, const Vector3& worldPos) :
        MouseToolEvent(worldPos),
        _view(view)
    {}

    XYMouseToolEvent(IOrthoView& view, const Vector3& worldPos, const Vector2& delta) :
        MouseToolEvent(worldPos, delta),
        _view(view)
    {}

    IOrthoView& getView()
    {
        return _view;
    }

    EViewType getViewType() const
    {
        return _view.getViewType();
    }

    float getScale() const
    {
        return _view.getScale();
    }
};

} // namespace
