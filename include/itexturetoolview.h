#pragma once

#include "iinteractiveview.h"

namespace ui
{

class ITextureToolView :
    public IInteractiveView
{
public:
    virtual ~ITextureToolView() {}

    // Scrolls the view by the specified amount of screen pixels
    virtual void scrollByPixels(int x, int y) = 0;
};

}
