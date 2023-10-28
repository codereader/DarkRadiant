#pragma once

#include "imodule.h"
#include "iinteractiveview.h"

template<typename Element>
class BasicVector3;
typedef BasicVector3<double> Vector3;

constexpr const char* const RKEY_HIGHER_ENTITY_PRIORITY = "user/ui/xyview/higherEntitySelectionPriority";

// Possible orientations of the orthogonal view window
enum class OrthoOrientation
{
    YZ = 0,
    XZ = 1,
    XY = 2
};

namespace ui
{

// Common interface used by all orthographic/2D views, i.e. XY/YZ/XZ and Texture Tool
class IOrthoViewBase: public IInteractiveView
{
public:
    virtual ~IOrthoViewBase() {}

    virtual int getWidth() const = 0;
    virtual int getHeight() const = 0;

    // Scrolls the view by the specified amount of screen pixels
    virtual void scrollByPixels(int x, int y) = 0;

    // Increase / decrease zoom factor
    virtual void zoomIn() = 0;
    virtual void zoomOut() = 0;
};

/// Interface for a single orthoview window
class IOrthoView: public IOrthoViewBase
{
public:
    virtual ~IOrthoView() {}

    // The cursor types available on orthoviews
    enum class CursorType
    {
        Pointer,
        Crosshair,
        Default = Pointer,
    };

    virtual const Vector3& getOrigin() const = 0;
    virtual void setOrigin(const Vector3& origin) = 0;

    // Returns the scale factor of this view
    virtual float getScale() const = 0;

    // Snaps the given Vector to the XY view's grid
    // Note that one component of the given vector stays untouched.
    virtual void snapToGrid(Vector3& point) const = 0;

    /// Get the orientation of this view
    virtual OrthoOrientation getOrientation() const = 0;

    // Sets the mouse cursor type of this view
    virtual void setCursorType(CursorType type) = 0;
};

/// Interface for the module which manages all of the ortho/XY views
class IOrthoViewManager: public RegisterableModule
{
public:
    // Passes a draw call to each allocated view, set force to true
    // to redraw immediately instead of queueing the draw.
    virtual void updateAllViews(bool force = false) = 0;

    // Sets the origin of all available views
    virtual void setOrigin(const Vector3& origin) = 0;

    // Returns the origin of the currently active ortho view
    // Will throw std::runtime_error if no view is found
    virtual Vector3 getActiveViewOrigin() = 0;

    // Returns a reference to the currently active view
    // Will throw std::runtime_error if no view is found
    virtual IOrthoView& getActiveView() = 0;

    // Return the first view matching the given viewType
    // Will throw std::runtime_error if no view is found
    virtual IOrthoView& getViewByType(OrthoOrientation viewType) = 0;

    // Sets the scale of all available views
    virtual void setScale(float scale) = 0;

    // Positions all available views
    virtual void positionAllViews(const Vector3& origin) = 0;

    // Positions the active views
    virtual void positionActiveView(const Vector3& origin) = 0;

    // Returns the view type of the currently active view
    virtual OrthoOrientation getActiveViewType() const = 0;

    // Sets the viewtype of the active view
    virtual void setActiveViewType(OrthoOrientation viewType) = 0;
};

} // namespace

constexpr const char* const MODULE_ORTHOVIEWMANAGER = "OrthoviewManager";

/// Accessor for the OrthoViewManager module
inline ui::IOrthoViewManager& GlobalOrthoViewManager()
{
    static module::InstanceReference<ui::IOrthoViewManager> _reference(MODULE_ORTHOVIEWMANAGER);
    return _reference;
}
