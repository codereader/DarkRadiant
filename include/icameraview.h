#pragma once

#include "iinteractiveview.h"

// Abstract class used when handling mouse events
// see also: class IOrthoView in iorthoview.h
class ICameraView :
    public IInteractiveView
{
public:
    virtual ~ICameraView() {}
};
