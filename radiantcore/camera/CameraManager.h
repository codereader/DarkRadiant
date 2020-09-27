#pragma once

#include "icamera.h"
#include "icameraview.h"

namespace camera
{

class CameraManager :
	public ICameraViewManager
{
public:
	// RegisterableModule
	const std::string& getName() const override;
	const StringSet& getDependencies() const override;
	void initialiseModule(const IApplicationContext& ctx) override;

	// ICameraManager
	ICameraView::Ptr createCamera(render::IRenderView& view,
		const std::function<void()>& queueDraw, const std::function<void()>& forceRedraw) override;

	void onCameraViewChanged();

	// Module-internal accessor
	static CameraManager& GetInstanceInternal();
};

}
