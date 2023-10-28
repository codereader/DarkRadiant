#pragma once

#include "imousetoolevent.h"
#include "math/Vector3.h"

namespace ui
{

// Special event subtype for XY View events. Provides
// information about view type, for example.
class XYMouseToolEvent: public OrthoViewMouseToolEvent
{
    IOrthoView& _view;

    // Current mouse coordinates, relative to 0,0,0 world origin
    Vector3 _worldPos;

public:
    XYMouseToolEvent(IOrthoView& view, const Vector3& worldPos, const Vector2& devicePos) :
        OrthoViewMouseToolEvent(view, devicePos),
        _view(view),
        _worldPos(worldPos)
    {}

    XYMouseToolEvent(IOrthoView& view, const Vector3& worldPos, const Vector2& devicePos, const Vector2& delta) :
        OrthoViewMouseToolEvent(view, devicePos, delta),
        _view(view),
        _worldPos(worldPos)
    {}

    IOrthoView& getView()
    {
        return _view;
    }

    const Vector3& getWorldPos() const
    {
        return _worldPos;
    }

    OrthoOrientation getViewType() const
    {
        return _view.getOrientation();
    }

    float getScale() const
    {
        return _view.getScale();
    }
};

}
