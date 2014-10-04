#pragma once

#include "iselectiontest.h"
#include "math/Vector2.h"

/**
 * Base class of the two interactive views used in DarkRadiant:
 * OrthoView (XYView) and the Camera (3D View).
 */
class IInteractiveView
{
public:
    virtual ~IInteractiveView() {}

    /**
     * Creates a new SelectionTest instance for the given rectangle defined by
     * minimum and maximum pixel coordinates. The upper left corner of the view is 0,0.
     */
    virtual SelectionTestPtr createSelectionTest(const Vector2& min, const Vector2& max) = 0;
};
