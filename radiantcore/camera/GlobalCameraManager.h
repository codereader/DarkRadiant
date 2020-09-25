#pragma once

#include "icamera.h"
#include "icameraview.h"

namespace camera
{

class CameraManager :
	public ICameraManager
{
public:
	// RegisterableModule
	const std::string& getName() const override;
	const StringSet& getDependencies() const override;
	void initialiseModule(const IApplicationContext& ctx) override;

	// ICameraManager
	void focusCamera(const Vector3& point, const Vector3& angles) override;
	ICameraView& getActiveView() override;
};

}
