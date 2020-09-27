#include "CameraManager.h"

#include "module/StaticModule.h"
#include "Camera.h"

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

ICameraView::Ptr CameraManager::createCamera(render::IRenderView& view, const std::function<void(bool)>& requestRedraw)
{
	_cameras.emplace_back(std::make_shared<Camera>(view, requestRedraw));
	return _cameras.back();
}

void CameraManager::destroyCamera(const ICameraView::Ptr& camera)
{
	_cameras.remove(camera);
}

sigc::signal<void>& CameraManager::signal_cameraChanged()
{
	return _sigCameraChanged;
}

void CameraManager::onCameraViewChanged()
{
	_sigCameraChanged.emit();
}

CameraManager& CameraManager::GetInstanceInternal()
{
	return *std::static_pointer_cast<CameraManager>(
		module::GlobalModuleRegistry().getModule(MODULE_CAMERA_MANAGER)
	);
}

module::StaticModule<CameraManager> cameraManagerModule;

}
