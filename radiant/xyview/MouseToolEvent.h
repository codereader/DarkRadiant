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

public:
    MouseToolEvent(const Vector3& worldPos) :
        _worldPos(worldPos)
    {}

    virtual ~MouseToolEvent() {}

    const Vector3& getWorldPos() const
    {
        return _worldPos;
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
    XYMouseToolEvent(const Vector3& worldPos, IOrthoView& view) :
        MouseToolEvent(worldPos),
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
