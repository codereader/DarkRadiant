#pragma once

#include <list>
#include "icamera.h"
#include "icameraview.h"

namespace camera
{

class CameraManager :
	public ICameraViewManager
{
private:
	sigc::signal<void> _sigCameraChanged;

	// We keep track of created cameras to be able to
	// issue focusCamera calls to them
	std::list<ICameraView::Ptr> _cameras;

public:
	// RegisterableModule
	const std::string& getName() const override;
	const StringSet& getDependencies() const override;
	void initialiseModule(const IApplicationContext& ctx) override;

	// ICameraManager
	ICameraView::Ptr createCamera(render::IRenderView& view, const std::function<void(bool)>& requestRedraw) override;

	void destroyCamera(const ICameraView::Ptr& camera) override;

	sigc::signal<void>& signal_cameraChanged() override;

	void onCameraViewChanged();

	// Module-internal accessor
	static CameraManager& GetInstanceInternal();
};

}
