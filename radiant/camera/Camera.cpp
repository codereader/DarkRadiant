#include "Camera.h"

#include <functional>
#include "ieventmanager.h"
#include "iregistry.h"
#include "GlobalCameraWndManager.h"
#include "CameraSettings.h"
#include "selection/SelectionTest.h"

namespace ui
{

namespace
{
	const std::string RKEY_SELECT_EPSILON = "user/ui/selectionEpsilon";

	const Matrix4 g_radiant2opengl = Matrix4::byColumns(
		0, -1, 0, 0,
		0, 0, 1, 0,
		-1, 0, 0, 0,
		0, 0, 0, 1
	);

	const Matrix4 g_opengl2radiant = Matrix4::byColumns(
		0, 0, -1, 0,
		-1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 0, 1
	);

	inline Matrix4 projection_for_camera(float near_z, float far_z, float fieldOfView, int width, int height)
	{
		const float half_width = near_z * tan(degrees_to_radians(fieldOfView * 0.5f));
		const float half_height = half_width * (static_cast<float>(height) / static_cast<float>(width));

		return Matrix4::getProjectionForFrustum(
			-half_width,
			half_width,
			-half_height,
			half_height,
			near_z,
			far_z
		);
	}
}

Vector3 Camera::_prevOrigin(0,0,0);
Vector3 Camera::_prevAngles(0,0,0);

Camera::Camera(render::View& view, const Callback& queueDraw, const Callback& forceRedraw) :
	_origin(_prevOrigin), // Use previous origin for camera position
	_angles(_prevAngles),
	_queueDraw(queueDraw),
	_forceRedraw(forceRedraw),
	width(0),
	height(0),
	timing(false),
	color(0, 0, 0),
	projection(Matrix4::getIdentity()),
	modelview(Matrix4::getIdentity()),
	freeMoveEnabled(false),
	movementflags(0),
	fieldOfView(75.0f),
	m_mouseMove(std::bind(&Camera::onMotionDelta, this, std::placeholders::_1, std::placeholders::_2)),
	_view(view)
{
	_moveTimer.Connect(wxEVT_TIMER, wxTimerEventHandler(Camera::camera_keymove), NULL, this);
}

void Camera::keyControl(float dtime) {
	int angleSpeed = getCameraSettings()->angleSpeed();
	int movementSpeed = getCameraSettings()->movementSpeed();

	// Update angles
	if (movementflags & MOVE_ROTLEFT)
		_angles[camera::CAMERA_YAW] += 15 * dtime* angleSpeed;
	if (movementflags & MOVE_ROTRIGHT)
		_angles[camera::CAMERA_YAW] -= 15 * dtime * angleSpeed;
	if (movementflags & MOVE_PITCHUP) {
		_angles[camera::CAMERA_PITCH] += 15 * dtime* angleSpeed;
		if (_angles[camera::CAMERA_PITCH] > 90)
			_angles[camera::CAMERA_PITCH] = 90;
	}
	if (movementflags & MOVE_PITCHDOWN) {
		_angles[camera::CAMERA_PITCH] -= 15 * dtime * angleSpeed;
		if (_angles[camera::CAMERA_PITCH] < -90)
			_angles[camera::CAMERA_PITCH] = -90;
	}

	updateModelview();
	freemoveUpdateAxes();

	// Update position
	if (movementflags & MOVE_FORWARD)
		_origin += forward * (dtime * movementSpeed);
	if (movementflags & MOVE_BACK)
		_origin += forward * (-dtime * movementSpeed);
	if (movementflags & MOVE_STRAFELEFT)
		_origin += right * (-dtime * movementSpeed);
	if (movementflags & MOVE_STRAFERIGHT)
		_origin += right * (dtime * movementSpeed);
	if (movementflags & MOVE_UP)
		_origin += g_vector3_axis_z * (dtime * movementSpeed);
	if (movementflags & MOVE_DOWN)
		_origin += g_vector3_axis_z * (-dtime * movementSpeed);

	updateModelview();
}

void Camera::camera_keymove(wxTimerEvent& ev)
{
	keyMove();
}

void Camera::setMovementFlags(unsigned int mask)
{
	if ((~movementflags & mask) != 0 && movementflags == 0)
	{
		_moveTimer.Start(10);
	}

	movementflags |= mask;
}

void Camera::clearMovementFlags(unsigned int mask)
{
	if ((movementflags & ~mask) == 0 && movementflags != 0)
	{
		_moveTimer.Stop();
	}

	movementflags &= ~mask;
}

void Camera::keyMove() 
{
	m_mouseMove.flush();

	//rMessage() << "keymove... ";
    float time_seconds = _keyControlTimer.Time() / static_cast<float>(1000);

    _keyControlTimer.Start();

	if (time_seconds > 0.05f)
    {
		time_seconds = 0.05f; // 20fps
	}

	keyControl(time_seconds * 5.0f);

	queueDraw();
	GlobalCamera().movedNotify();
}

void Camera::updateModelview()
{
	_prevAngles = _angles;
	_prevOrigin = _origin;

	modelview = Matrix4::getIdentity();

	// roll, pitch, yaw
	Vector3 radiant_eulerXYZ(0, -_angles[camera::CAMERA_PITCH], _angles[camera::CAMERA_YAW]);

	modelview.translateBy(_origin);
	modelview.rotateByEulerXYZDegrees(radiant_eulerXYZ);
	modelview.multiplyBy(g_radiant2opengl);
	modelview.invert();

	updateVectors();

	_view.Construct(projection, modelview, width, height);
}

void Camera::updateVectors() {
	for (int i=0 ; i<3 ; i++) {
		vright[i] = modelview[(i<<2)+0];
		vup[i] = modelview[(i<<2)+1];
		vpn[i] = modelview[(i<<2)+2];
	}
}

void Camera::freemoveUpdateAxes() {
	right = vright;
	forward = -vpn;
}

void Camera::mouseMove(int x, int y) {
	//rMessage() << "mousemove... ";
	freeMove(-x, -y);
	queueDraw();
	GlobalCamera().movedNotify();
}

void Camera::freeMove(int dx, int dy) {
	int angleSpeed = getCameraSettings()->angleSpeed();

	// free strafe mode, toggled by the keyboard modifiers
	if (m_strafe) {
		const float strafespeed = GlobalCamera().getCameraStrafeSpeed();
        const float forwardStrafeFactor = GlobalCamera().getCameraForwardStrafeFactor();

		_origin -= vright * strafespeed * dx;

		if (m_strafe_forward) {
			_origin += vpn * strafespeed * dy * forwardStrafeFactor;
		} else {
			_origin += vup * strafespeed * dy;
		}
	} else { // free rotation
		const float dtime = 0.1f;

		const float zAxisFactor = getCameraSettings()->invertMouseVerticalAxis() ? -1.0f : 1.0f;

		_angles[camera::CAMERA_PITCH] += dy * dtime * angleSpeed * zAxisFactor;

		_angles[camera::CAMERA_YAW] += dx * dtime * angleSpeed;

		if (_angles[camera::CAMERA_PITCH] > 90)
			_angles[camera::CAMERA_PITCH] = 90;
		else if (_angles[camera::CAMERA_PITCH] < -90)
			_angles[camera::CAMERA_PITCH] = -90;

		if (_angles[camera::CAMERA_YAW] >= 360)
			_angles[camera::CAMERA_YAW] -=360;
		else if (_angles[camera::CAMERA_YAW] <= 0)
			_angles[camera::CAMERA_YAW] +=360;
	}

	updateModelview();
	freemoveUpdateAxes();
}

void Camera::mouseControl(int x, int y) {
	int movementSpeed = getCameraSettings()->movementSpeed();

	float xf, yf;

	xf = (float)(x - width/2) / (width/2);
	yf = (float)(y - height/2) / (height/2);

	xf *= 1.0f - fabsf(yf);
	if (xf < 0) {
		xf += 0.1f;
		if (xf > 0)
			xf = 0;
	} else {
		xf -= 0.1f;
		if (xf < 0)
			xf = 0;
	}

	_origin += forward * (yf * 0.1f* movementSpeed);
	_angles[camera::CAMERA_YAW] += xf * -0.1f * movementSpeed;

	updateModelview();
}

const Vector3& Camera::getCameraOrigin() const
{
	return _origin;
}

void Camera::setCameraOrigin(const Vector3& newOrigin)
{
	_origin = newOrigin;
	_prevOrigin = _origin;

	updateModelview();
	queueDraw();
	GlobalCamera().movedNotify();
}

const Vector3& Camera::getCameraAngles() const
{
	return _angles;
}

void Camera::setCameraAngles(const Vector3& newAngles)
{
	_angles = newAngles;
	_prevAngles = _angles;

	updateModelview();
	queueDraw();
	GlobalCamera().movedNotify();
}

const Vector3& Camera::getRightVector() const
{
	return vright;
}

const Vector3& Camera::getUpVector() const
{
	return vup;
}

const Vector3& Camera::getForwardVector() const
{
	return vpn;
}

int Camera::getDeviceWidth() const
{
	return width;
}

int Camera::getDeviceHeight() const
{
	return height;
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

void Camera::queueDraw()
{
	_queueDraw();
}

void Camera::forceRedraw()
{
	_forceRedraw();
}

void Camera::moveUpdateAxes() {
	double ya = degrees_to_radians(_angles[camera::CAMERA_YAW]);

	// the movement matrix is kept 2d
	forward[0] = static_cast<float>(cos(ya));
	forward[1] = static_cast<float>(sin(ya));
	forward[2] = 0;
	right[0] = forward[1];
	right[1] = -forward[0];
}

bool Camera::farClipEnabled() const
{
	return getCameraSettings()->farClipEnabled();
}

float Camera::getFarClipPlane() const
{
	return (farClipEnabled()) ? pow(2.0, (getCameraSettings()->cubicScale() + 7) / 2.0) : 32768.0f;
}

void Camera::updateProjection()
{
	float farClip = getFarClipPlane();
	projection = projection_for_camera(farClip / 4096.0f, farClip, fieldOfView, width, height);

	_view.Construct(projection, modelview, width, height);
}

void Camera::onMotionDelta(int x, int y)
{
	mouseMove(x, y);
}

void Camera::pitchUpDiscrete()
{
	Vector3 angles = getCameraAngles();

	angles[camera::CAMERA_PITCH] += SPEED_TURN;
	if (angles[camera::CAMERA_PITCH] > 90)
		angles[camera::CAMERA_PITCH] = 90;

	setCameraAngles(angles);
}

void Camera::pitchDownDiscrete()
{
	Vector3 angles = getCameraAngles();

	angles[camera::CAMERA_PITCH] -= SPEED_TURN;
	if (angles[camera::CAMERA_PITCH] < -90)
		angles[camera::CAMERA_PITCH] = -90;

	setCameraAngles(angles);
}

void Camera::moveForwardDiscrete(double units)
{
	moveUpdateAxes();
	setCameraOrigin(getCameraOrigin() + forward * fabs(units));
}

void Camera::moveBackDiscrete(double units)
{
	moveUpdateAxes();
	setCameraOrigin(getCameraOrigin() + forward * (-fabs(units)));
}

void Camera::moveUpDiscrete(double units)
{
	Vector3 origin = getCameraOrigin();

	origin[2] += fabs(units);

	setCameraOrigin(origin);
}

void Camera::moveDownDiscrete(double units)
{
	Vector3 origin = getCameraOrigin();
	origin[2] -= fabs(units);
	setCameraOrigin(origin);
}

void Camera::moveLeftDiscrete(double units)
{
	moveUpdateAxes();
	setCameraOrigin(getCameraOrigin() + right * (-fabs(units)));
}

void Camera::moveRightDiscrete(double units)
{
	moveUpdateAxes();
	setCameraOrigin(getCameraOrigin() + right * fabs(units));
}

void Camera::rotateLeftDiscrete()
{
	Vector3 angles = getCameraAngles();
	angles[camera::CAMERA_YAW] += SPEED_TURN;
	setCameraAngles(angles);
}

void Camera::rotateRightDiscrete()
{
	Vector3 angles = getCameraAngles();
	angles[camera::CAMERA_YAW] -= SPEED_TURN;
	setCameraAngles(angles);
}

} // namespace
