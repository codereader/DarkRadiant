#pragma once

#include "iscript.h"
#include "iscriptinterface.h"
#include "icameraview.h"

#include "MathInterface.h"

namespace script
{

class ScriptCameraView
{
private:
	camera::ICameraView& _cameraView;

public:
	ScriptCameraView(camera::ICameraView& cameraView);

	const Vector3& getCameraOrigin() const;
	void setCameraOrigin(const Vector3& newOrigin);

	const Vector3& getCameraAngles() const;
	void setCameraAngles(const Vector3& newAngles);

	void setOriginAndAngles(const Vector3& newOrigin, const Vector3& newAngles);

	const Vector3& getRightVector() const;
	const Vector3& getUpVector() const;
	const Vector3& getForwardVector() const;

	float getFarClipPlaneDistance() const;
	void setFarClipPlaneDistance(float distance);

	void refresh();
};

class CameraInterface :
	public IScriptInterface
{
public:
	ScriptCameraView getActiveView();

	// IScriptInterface implementation
	void registerInterface(py::module& scope, py::dict& globals) override;
};

} // namespace script
