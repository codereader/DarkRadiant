#pragma once

#include "iselectiontest.h"
#include "ivolumetest.h"
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

    // Returns the device dimensions in pixels
    virtual int getDeviceWidth() const = 0;
    virtual int getDeviceHeight() const = 0;

    /**
     * Returns the VolumeTest instance associated with this view.
     */
    virtual const VolumeTest& getVolumeTest() const = 0;
};
