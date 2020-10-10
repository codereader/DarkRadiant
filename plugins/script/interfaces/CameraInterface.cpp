#include "CameraInterface.h"

#include "math/Vector3.h"
#include <pybind11/pybind11.h>

namespace script
{

ScriptCameraView::ScriptCameraView(camera::ICameraView& cameraView) :
	_cameraView(cameraView)
{}

const Vector3& ScriptCameraView::getCameraOrigin() const
{
	return _cameraView.getCameraOrigin();
}

void ScriptCameraView::setCameraOrigin(const Vector3& newOrigin)
{
	_cameraView.setCameraOrigin(newOrigin);
}

const Vector3& ScriptCameraView::getCameraAngles() const
{
	return _cameraView.getCameraAngles();
}

void ScriptCameraView::setCameraAngles(const Vector3& newAngles)
{
	_cameraView.setCameraAngles(newAngles);
}

void ScriptCameraView::setOriginAndAngles(const Vector3& newOrigin, const Vector3& newAngles)
{
	_cameraView.setOriginAndAngles(newOrigin, newAngles);
}

const Vector3& ScriptCameraView::getRightVector() const
{
	return _cameraView.getRightVector();
}

const Vector3& ScriptCameraView::getUpVector() const
{
	return _cameraView.getUpVector();
}

const Vector3& ScriptCameraView::getForwardVector() const
{
	return _cameraView.getForwardVector();
}

float ScriptCameraView::getFarClipPlaneDistance() const
{
	return _cameraView.getFarClipPlaneDistance();
}

void ScriptCameraView::setFarClipPlaneDistance(float distance)
{
	_cameraView.setFarClipPlaneDistance(distance);
}

ScriptCameraView CameraInterface::getActiveView()
{
	return ScriptCameraView(GlobalCameraManager().getActiveView());
}

void CameraInterface::registerInterface(py::module& scope, py::dict& globals)
{
	// Define a CameraView structure
	py::class_<ScriptCameraView> camera(scope, "CameraView");
	camera.def(py::init<camera::ICameraView&>());
	camera.def("getCameraOrigin", &ScriptCameraView::getCameraOrigin, py::return_value_policy::reference);
	camera.def("setCameraOrigin", &ScriptCameraView::setCameraOrigin);
	camera.def("getCameraAngles", &ScriptCameraView::getCameraAngles, py::return_value_policy::reference);
	camera.def("setCameraAngles", &ScriptCameraView::setCameraAngles);
	camera.def("setOriginAndAngles", &ScriptCameraView::setOriginAndAngles);
	camera.def("getRightVector", &ScriptCameraView::getRightVector, py::return_value_policy::reference);
	camera.def("getUpVector", &ScriptCameraView::getUpVector, py::return_value_policy::reference);
	camera.def("getForwardVector", &ScriptCameraView::getForwardVector, py::return_value_policy::reference);
	camera.def("getFarClipPlaneDistance", &ScriptCameraView::getFarClipPlaneDistance);
	camera.def("setFarClipPlaneDistance", &ScriptCameraView::setFarClipPlaneDistance);

	// Define the BrushCreator interface
	py::class_<CameraInterface> cameraManager(scope, "Camera");
	cameraManager.def("getActiveView", &CameraInterface::getActiveView);

	// Now point the Python variable "CameraInterface" to this instance
	globals["GlobalCameraManager"] = this;
}

}
