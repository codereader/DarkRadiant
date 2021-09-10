#pragma once

#include "imousetoolevent.h"

namespace ui
{

// Special event subtype for Texture Tool View events
class TextureToolMouseEvent :
    public MouseToolEvent
{
public:
    TextureToolMouseEvent(IInteractiveView& view, const Vector2& devicePosition) :
        MouseToolEvent(view, devicePosition)
    {}

    TextureToolMouseEvent(IInteractiveView& view, const Vector2& devicePosition, const Vector2& delta) :
        MouseToolEvent(view, devicePosition, delta)
    {}
};

}