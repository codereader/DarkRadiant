#pragma once

#include "imodule.h"
#include "icameraview.h"
#include "math/Vector3.h"

namespace camera
{

/**
 * The "global" interface of DarkRadiant's camera module.
 */
class ICameraManager :
	public RegisterableModule
{
public:
};
typedef std::shared_ptr<ICameraManager> ICameraPtr;

} // namespace

const char* const MODULE_CAMERA_WND_MANAGER("CameraWndManager");

// Module accessor
inline camera::ICameraManager& GlobalCameraWndManager()
{
	// Cache the reference locally
    static camera::ICameraManager& _camera(
        *std::static_pointer_cast<camera::ICameraManager>(
			module::GlobalModuleRegistry().getModule(MODULE_CAMERA_WND_MANAGER)
		)
	);
	return _camera;
}
