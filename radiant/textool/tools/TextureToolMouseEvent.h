#pragma once

#include "iorthoview.h"
#include "imousetoolevent.h"

namespace ui
{

// Special event subtype for Texture Tool View events
class TextureToolMouseEvent :
    public OrthoViewMouseToolEvent
{
private:
    IOrthoViewBase& _view;

public:
    TextureToolMouseEvent(IOrthoViewBase& view, const Vector2& devicePosition) :
        OrthoViewMouseToolEvent(view, devicePosition),
        _view(view)
    {}

    TextureToolMouseEvent(IOrthoViewBase& view, const Vector2& devicePosition, const Vector2& delta) :
        OrthoViewMouseToolEvent(view, devicePosition, delta),
        _view(view)
    {}

    IOrthoViewBase& getView()
    {
        return _view;
    }
};

}