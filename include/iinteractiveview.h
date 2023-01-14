#pragma once

#include "iselectiontest.h"
#include "ivolumetest.h"
#include "math/Vector2.h"

/**
 * Base class of the interactive views used in DarkRadiant:
 * OrthoView (XYView), Camera (3D View), Texture Tool (UV editor)
 */
class IInteractiveView
{
public:
    virtual ~IInteractiveView() {}

    /**
     * Creates a new SelectionTest instance for the given position, measured in
     * device coordinates in the range [-1..+1]
     */
    virtual SelectionTestPtr createSelectionTestForPoint(const Vector2& point) = 0;

    // Returns true if this view allows drag-selecting multiple items
    virtual bool supportsDragSelections() = 0;

    // Returns the device dimensions in pixels
    virtual int getDeviceWidth() const = 0;
    virtual int getDeviceHeight() const = 0;

    /**
     * Returns the VolumeTest instance associated with this view.
     */
    virtual const VolumeTest& getVolumeTest() const = 0;

    // Schedules a repaint of this view
    virtual void queueDraw() = 0;

    // Paints the view immediately
    virtual void forceRedraw() = 0;
};
