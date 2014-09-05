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
    EViewType _viewType;
    float _scale;

public:
    XYMouseToolEvent(const Vector3& worldPos, EViewType viewType, float scale) :
        MouseToolEvent(worldPos),
        _viewType(viewType),
        _scale(scale)
    {}

    EViewType getViewType() const
    {
        return _viewType;
    }

    float getScale() const
    {
        return _scale;
    }
};

} // namespace
