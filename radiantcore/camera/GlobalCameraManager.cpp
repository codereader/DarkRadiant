#include "GlobalCameraManager.h"

#include "module/StaticModule.h"

namespace camera
{

const std::string& CameraManager::getName() const
{
	static std::string _name(MODULE_CAMERA_MANAGER);
	return _name;
}

const StringSet& CameraManager::getDependencies() const
{
	static StringSet _dependencies;
	return _dependencies;
}

void CameraManager::initialiseModule(const IApplicationContext& ctx)
{
	rMessage() << getName() << "::initialiseModule called." << std::endl;
}

void CameraManager::focusCamera(const Vector3& point, const Vector3& angles)
{
	// TODO
}

ICameraView& CameraManager::getActiveView()
{
	throw std::runtime_error("Not implemented");
}

// disabled for now:  
// module::StaticModule<CameraManager> cameraManagerModule;

}
