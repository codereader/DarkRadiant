#pragma once

#include "imodule.h"
#include "iinteractiveview.h"
#include "irenderview.h"

template<typename Element>class BasicVector3;
typedef BasicVector3<double> Vector3;
class Matrix4;

namespace camera
{

enum
{
	CAMERA_PITCH = 0, // up / down
	CAMERA_YAW = 1, // left / right
	CAMERA_ROLL = 2, // fall over
};

// Abstract class used when handling mouse events
// see also: class IOrthoView in iorthoview.h
class ICameraView :
	public virtual IInteractiveView
{
public:
	typedef std::shared_ptr<ICameraView> Ptr;

	virtual ~ICameraView() {}

	// Sets the device width and height, updates the projection
	virtual void setDeviceDimensions(int width, int height) = 0;

	// Move the camera's origin
	virtual const Vector3& getCameraOrigin() const = 0;
	virtual void setCameraOrigin(const Vector3& newOrigin) = 0;

	virtual const Vector3& getCameraAngles() const = 0;
	virtual void setCameraAngles(const Vector3& newAngles) = 0;

	// Combined setter for position and angles - triggers only one callback to potential observers
	virtual void setOriginAndAngles(const Vector3& newOrigin, const Vector3& newAngles) = 0;

	// Returns the vector pointing to the "right"
	virtual const Vector3& getRightVector() const = 0;
	// Returns the vector pointing "up"
	virtual const Vector3& getUpVector() const = 0;
	// Returns the vector pointing "forward"
	virtual const Vector3& getForwardVector() const = 0;

	virtual const Matrix4& getModelView() const = 0;
	virtual const Matrix4& getProjection() const = 0;

	// Cubic clipping
	virtual float getFarClipPlaneDistance() const = 0;
	virtual void setFarClipPlaneDistance(float distance) = 0;
	virtual bool getFarClipPlaneEnabled() const = 0;
	virtual void setFarClipPlaneEnabled(bool enabled) = 0;
};

class IFreeMoveView :
	public virtual IInteractiveView
{
public:
	virtual ~IFreeMoveView() {}

    // Freemove mode
    virtual void enableFreeMove() = 0;
    virtual void disableFreeMove() = 0;
    virtual bool freeMoveEnabled() const = 0;
};

class ICameraViewManager :
	public RegisterableModule
{
public:
	virtual ~ICameraViewManager() {}

	// Create a new camera instance. The ICameraViewManager is keeping a reference to this
	// object for broadcasting the focusCamera() calls, so be sure to notify the manager
	// if this camera is no longer in use by invoking destroyCamera().
	// The requestRedraw takes a bool indicating whether the redraw should be queued (false)
	// or a redraw should be forced (true)
	virtual ICameraView::Ptr createCamera(render::IRenderView& view, const std::function<void(bool)>& requestRedraw) = 0;

	// Releases this camera instance, clearing any internal references to it
	virtual void destroyCamera(const ICameraView::Ptr& camera) = 0;

	// Sets the position and angles of all active cameras to the given values
	virtual void focusAllCameras(const Vector3& position, const Vector3& angles) = 0;

	// A reference to the currently active view. Will throw a std::runtime_error if no camera is present
	virtual ICameraView& getActiveView() = 0;

	// Signal emitted when any camera position or angles changed
	virtual sigc::signal<void>& signal_cameraChanged() = 0;
};

}

constexpr const char* const MODULE_CAMERA_MANAGER("CameraManager");

constexpr const char* const RKEY_CAMERA_DRAG_SELECTION_ENABLED = "user/ui/camera/dragSelectionEnabled";

// Module accessor
inline camera::ICameraViewManager& GlobalCameraManager()
{
	static module::InstanceReference<camera::ICameraViewManager> _reference(MODULE_CAMERA_MANAGER);
	return _reference;
}
