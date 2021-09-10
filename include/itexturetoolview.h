#pragma once

#include "iorthoview.h"

class SelectionTest;

namespace ui
{

class ITextureToolView :
    public IOrthoViewBase
{
public:
    virtual ~ITextureToolView() {}

    // Perform a selection test using the given test instance
    virtual void testSelect(SelectionTest& test) = 0;
};

}
