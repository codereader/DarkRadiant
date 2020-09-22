#pragma once

#include "imodule.h"
#include "icameraview.h"
#include "math/Vector3.h"
#include <list>

namespace ui
{

enum 
{
    CAMERA_PITCH = 0, // up / down
    CAMERA_YAW = 1, // left / right
    CAMERA_ROLL = 2, // fall over
};

/// Observer interface for ICamera
class CameraObserver
{
public:
    // destructor
	virtual ~CameraObserver() {}
	// This gets called as soon as the camera is moved
	virtual void cameraMoved() = 0;

}; // class CameraObserver

typedef std::list<CameraObserver*> CameraObserverList;

/**
 * The "global" interface of DarkRadiant's camera module.
 */
class ICamera :
	public RegisterableModule
{
public:
	/**
	 * greebo: Sets the camera origin to the given <point> using the given <angles>.
	 */
	virtual void focusCamera(const Vector3& point, const Vector3& angles) = 0;

    /// Get the active camera view
	virtual ICameraView::Ptr getActiveCameraView() = 0;

    /// Add an observer for camera events
	virtual void addCameraObserver(CameraObserver* observer) = 0;

    /// Remove a camera observer
	virtual void removeCameraObserver(CameraObserver* observer) = 0;
};
typedef std::shared_ptr<ICamera> ICameraPtr;

} // namespace

const std::string MODULE_CAMERA("Camera");

// Accessor
// (this is named CameraView to avoid name conflicts with the existing GlobalCamera() accessor)
inline ui::ICamera& GlobalCameraView() {
	// Cache the reference locally
    static ui::ICamera& _camera(
        *std::static_pointer_cast<ui::ICamera>(
			module::GlobalModuleRegistry().getModule(MODULE_CAMERA)
		)
	);
	return _camera;
}

class Matrix4;

class CameraView
{
public:
  virtual ~CameraView() {}
  virtual void setModelview(const Matrix4& modelview) = 0;
  virtual void setFieldOfView(float fieldOfView) = 0;
};
