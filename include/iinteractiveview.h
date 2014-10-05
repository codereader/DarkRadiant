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
     * Creates a new SelectionTest instance for the given position in
     * pixel coordinates. The upper left corner of the view is 0,0.
     */
    virtual SelectionTestPtr createSelectionTestForPoint(const Vector2& point) = 0;
};
