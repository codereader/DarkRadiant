#pragma once

#include "imodule.h"
#include "iinteractiveview.h"

template<typename Element>
class BasicVector3;
typedef BasicVector3<double> Vector3;

// Possible types of the orthogonal view window
enum EViewType
{
    YZ = 0,
    XZ = 1,
    XY = 2
};

namespace ui
{

class IOrthoView :
    public IInteractiveView
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

    // Returns the scale factor of this view
    virtual float getScale() const = 0;

    // Snaps the given Vector to the XY view's grid
    // Note that one component of the given vector stays untouched.
    virtual void snapToGrid(Vector3& point) = 0;

    // Returns the projection type (XY, XZ, YZ) of this view
    virtual EViewType getViewType() const = 0;

    // Sets the mouse cursor type of this view
    virtual void setCursorType(CursorType type) = 0;

    // Increase / decrease zoom factor
    virtual void zoomIn() = 0;
    virtual void zoomOut() = 0;

    // Scrolls the view by the specified amount of screen pixels
    virtual void scroll(int x, int y) = 0;
};

class IXWndManager :
	public RegisterableModule
{
public:
	// Passes a draw call to each allocated view, set force to true 
    // to redraw immediately instead of queueing the draw.
	virtual void updateAllViews(bool force = false) = 0;

	// Free all allocated views
	virtual void destroyViews() = 0;

	// Sets the origin of all available views
	virtual void setOrigin(const Vector3& origin) = 0;

	// Sets the scale of all available views
	virtual void setScale(float scale) = 0;

	// Positions all available views
	virtual void positionAllViews(const Vector3& origin) = 0;

	// Positions the active views
	virtual void positionActiveView(const Vector3& origin) = 0;

	// Returns the view type of the currently active view
	virtual EViewType getActiveViewType() const = 0;

	// Sets the viewtype of the active view
	virtual void setActiveViewType(EViewType viewType) = 0;
};

} // namespace

const char* const MODULE_ORTHOVIEWMANAGER = "OrthoviewManager";

// This is the accessor for the xy window manager module
inline ui::IXWndManager& GlobalXYWndManager()
{
	// Cache the reference locally
	static ui::IXWndManager& _xyWndManager(
		*std::static_pointer_cast<ui::IXWndManager>(
			module::GlobalModuleRegistry().getModule(MODULE_ORTHOVIEWMANAGER)
		)
	);
	return _xyWndManager;
}
