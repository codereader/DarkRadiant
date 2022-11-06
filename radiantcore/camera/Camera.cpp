#include "Camera.h"

#include <functional>
#include "registry/registry.h"
#include "CameraManager.h"
#include "render/View.h"
#include "render/CameraView.h"
#include "selection/SelectionVolume.h"
#include "Rectangle.h"

namespace camera
{

namespace
{
	const std::string RKEY_SELECT_EPSILON = "user/ui/selectionEpsilon";
}

Vector3 Camera::_prevOrigin(0,0,0);
Vector3 Camera::_prevAngles(0,0,0);

Camera::Camera(render::IRenderView& view, const std::function<void(bool)>& requestRedraw) :
	_origin(_prevOrigin), // Use previous origin for camera position
	_angles(_prevAngles),
	_requestRedraw(requestRedraw),
	_fieldOfView(90.0f),
	_farClipPlane(32768),
	_farClipPlaneEnabled(true),
	_width(0),
	_height(0),
	_projection(Matrix4::getIdentity()),
	_modelview(Matrix4::getIdentity()),
	_view(view),
    _dragSelectionEnabled(RKEY_CAMERA_DRAG_SELECTION_ENABLED)
{}

void Camera::updateModelview()
{
	_prevAngles = _angles;
	_prevOrigin = _origin;

    _modelview = calculateModelViewMatrix(_origin, _angles);

	updateVectors();

	_view.construct(_projection, _modelview, _width, _height);
}

void Camera::updateVectors()
{
	for (int i = 0; i < 3; i++)
	{
		_vright[i] = _modelview[(i<<2)+0];
		_vup[i] = _modelview[(i<<2)+1];
		_vpn[i] = _modelview[(i<<2)+2];
	}
}

void Camera::freemoveUpdateAxes() 
{
	_right = _vright;
	_forward = -_vpn;
}

const Vector3& Camera::getCameraOrigin() const
{
	return _origin;
}

void Camera::setCameraOrigin(const Vector3& newOrigin)
{
	doSetOrigin(newOrigin, true);
	
	queueDraw();
	CameraManager::GetInstanceInternal().onCameraViewChanged();
}

const Vector3& Camera::getCameraAngles() const
{
	return _angles;
}

void Camera::setCameraAngles(const Vector3& newAngles)
{
	doSetAngles(newAngles, true);
	
	queueDraw();
	CameraManager::GetInstanceInternal().onCameraViewChanged();
}

void Camera::doSetOrigin(const Vector3& origin, bool updateModelView)
{
	_origin = origin;
	_prevOrigin = _origin;

	if (updateModelView)
	{
		updateModelview();
		queueDraw();
	}
}

void Camera::doSetAngles(const Vector3& angles, bool updateModelView)
{
	_angles = angles;
	_prevAngles = _angles;

	if (updateModelView)
	{
		updateModelview();
		freemoveUpdateAxes();
	}
}

void Camera::setOriginAndAngles(const Vector3& newOrigin, const Vector3& newAngles)
{
	doSetOrigin(newOrigin, false); // hold back matrix recalculation
	doSetAngles(newAngles, false); // hold back matrix recalculation

	updateModelview();
	freemoveUpdateAxes();

	queueDraw();
	CameraManager::GetInstanceInternal().onCameraViewChanged();
}

const Vector3& Camera::getRightVector() const
{
	return _vright;
}

const Vector3& Camera::getUpVector() const
{
	return _vup;
}

const Vector3& Camera::getForwardVector() const
{
	return _vpn;
}

const Matrix4& Camera::getModelView() const
{
	return _modelview;
}

const Matrix4& Camera::getProjection() const
{
	return _projection;
}

int Camera::getDeviceWidth() const
{
	return _width;
}

int Camera::getDeviceHeight() const
{
	return _height;
}

void Camera::setDeviceDimensions(int width, int height)
{
	_width = width;
	_height = height;
	updateProjection();
}

SelectionTestPtr Camera::createSelectionTestForPoint(const Vector2& point)
{
	float selectEpsilon = registry::getValue<float>(RKEY_SELECT_EPSILON);

	// Get the mouse position
	Vector2 deviceEpsilon(selectEpsilon / getDeviceWidth(), selectEpsilon / getDeviceHeight());

	// Copy the current view and constrain it to a small rectangle
	render::View scissored(_view);

	auto rect = selection::Rectangle::ConstructFromPoint(point, deviceEpsilon);
	scissored.EnableScissor(rect.min[0], rect.max[0], rect.min[1], rect.max[1]);

	return SelectionTestPtr(new SelectionVolume(scissored));
}

const VolumeTest& Camera::getVolumeTest() const
{
	return _view;
}

bool Camera::supportsDragSelections()
{
    return _dragSelectionEnabled.get();
}

void Camera::queueDraw()
{
	_requestRedraw(false);
}

void Camera::forceRedraw()
{
	_requestRedraw(true);
}

void Camera::moveUpdateAxes()
{
	double ya = degrees_to_radians(_angles[camera::CAMERA_YAW]);

	// the movement matrix is kept 2d
	_forward[0] = static_cast<float>(cos(ya));
	_forward[1] = static_cast<float>(sin(ya));
	_forward[2] = 0;
	_right[0] = _forward[1];
	_right[1] = -_forward[0];
}

float Camera::getFarClipPlaneDistance() const
{
	return _farClipPlane;
}

void Camera::setFarClipPlaneDistance(float distance)
{
	_farClipPlane = distance;
	updateProjection();
}

bool Camera::getFarClipPlaneEnabled() const
{
    return _farClipPlaneEnabled;
}

void Camera::setFarClipPlaneEnabled(bool enabled)
{
    _farClipPlaneEnabled = enabled;
    updateProjection();
}

void Camera::updateProjection()
{
    auto farClip = _farClipPlaneEnabled ? getFarClipPlaneDistance() : 32768.0f;
	_projection = calculateProjectionMatrix(farClip / 4096.0f, farClip, _fieldOfView, _width, _height);

	_view.construct(_projection, _modelview, _width, _height);
}

} // namespace
