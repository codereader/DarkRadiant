#pragma once

#include "icamera.h"
#include "icameraview.h"

namespace camera
{

class CameraManager :
	public ICameraViewManager
{
private:
	sigc::signal<void> _sigCameraChanged;

public:
	// RegisterableModule
	const std::string& getName() const override;
	const StringSet& getDependencies() const override;
	void initialiseModule(const IApplicationContext& ctx) override;

	// ICameraManager
	ICameraView::Ptr createCamera(render::IRenderView& view,
		const std::function<void()>& queueDraw, const std::function<void()>& forceRedraw) override;

	sigc::signal<void>& signal_cameraChanged() override;

	void onCameraViewChanged();

	// Module-internal accessor
	static CameraManager& GetInstanceInternal();
};

}
