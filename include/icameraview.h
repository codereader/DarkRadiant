#pragma once

#include "iinteractiveview.h"

// Abstract class used when handling mouse events
// see also: class IOrthoView in iorthoview.h
class ICameraView :
    public IInteractiveView
{
public:
    virtual ~ICameraView() {}

    // Freemove mode
    virtual void enableFreeMove() = 0;
    virtual void disableFreeMove() = 0;
    virtual bool freeMoveEnabled() const = 0;
};
