#pragma once

#include "imodule.h"
#include "icameraview.h"
#include "math/Vector3.h"

namespace camera
{

enum 
{
    CAMERA_PITCH = 0, // up / down
    CAMERA_YAW = 1, // left / right
    CAMERA_ROLL = 2, // fall over
};

/**
 * The "global" interface of DarkRadiant's camera module.
 */
class ICameraManager :
	public RegisterableModule
{
public:
	/**
	 * greebo: Sets the camera origin to the given <point> using the given <angles>.
	 */
	virtual void focusCamera(const Vector3& point, const Vector3& angles) = 0;

	// A reference to the currently active view. Will throw a std::runtime_error if no camera is present
	virtual ICameraView& getActiveView() = 0;
};
typedef std::shared_ptr<ICameraManager> ICameraPtr;

} // namespace

const char* const MODULE_CAMERA_MANAGER("CameraManager");

// Module accessor
inline camera::ICameraManager& GlobalCameraManager()
{
	// Cache the reference locally
    static camera::ICameraManager& _camera(
        *std::static_pointer_cast<camera::ICameraManager>(
			module::GlobalModuleRegistry().getModule(MODULE_CAMERA_MANAGER)
		)
	);
	return _camera;
}
