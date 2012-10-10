#include "Camera.h"

#include "ieventmanager.h"
#include "iregistry.h"
#include "GlobalCamera.h"
#include "CameraSettings.h"
#include <boost/bind.hpp>

Vector3 Camera::_prevOrigin(0,0,0);
Vector3 Camera::_prevAngles(0,0,0);

Camera::Camera(render::View* view, const Callback& update) :
	_origin(_prevOrigin), // Use previous origin for camera position
	_angles(_prevAngles),
	width(0),
	height(0),
	timing(false),
	color(0, 0, 0),
	movementflags(0),
	m_keymove_handler(0),
	fieldOfView(75.0f),
	m_mouseMove(boost::bind(&Camera::onMotionDelta, this, _1, _2)),
	m_view(view),
	m_update(update)
{}

void Camera::keyControl(float dtime) {
	int angleSpeed = getCameraSettings()->angleSpeed();
	int movementSpeed = getCameraSettings()->movementSpeed();

	// Update angles
	if (movementflags & MOVE_ROTLEFT)
		_angles[CAMERA_YAW] += 15 * dtime* angleSpeed;
	if (movementflags & MOVE_ROTRIGHT)
		_angles[CAMERA_YAW] -= 15 * dtime * angleSpeed;
	if (movementflags & MOVE_PITCHUP) {
		_angles[CAMERA_PITCH] += 15 * dtime* angleSpeed;
		if (_angles[CAMERA_PITCH] > 90)
			_angles[CAMERA_PITCH] = 90;
	}
	if (movementflags & MOVE_PITCHDOWN) {
		_angles[CAMERA_PITCH] -= 15 * dtime * angleSpeed;
		if (_angles[CAMERA_PITCH] < -90)
			_angles[CAMERA_PITCH] = -90;
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

gboolean Camera::camera_keymove(gpointer data) {
	Camera* self = reinterpret_cast<Camera*>(data);
	self->keyMove();
	return TRUE;
}

void Camera::setMovementFlags(unsigned int mask) {
	if ((~movementflags & mask) != 0 && movementflags == 0) {
		m_keymove_handler = g_idle_add(camera_keymove, this);
	}
	movementflags |= mask;
}

void Camera::clearMovementFlags(unsigned int mask) {
	if ((movementflags & ~mask) == 0 && movementflags != 0) {
		g_source_remove(m_keymove_handler);
		m_keymove_handler = 0;
	}
	movementflags &= ~mask;
}

void Camera::keyMove() {
	m_mouseMove.flush();

	//rMessage() << "keymove... ";
	float time_seconds = m_keycontrol_timer.elapsed_msec() / static_cast<float>(msec_per_sec);
	m_keycontrol_timer.start();
	if (time_seconds > 0.05f) {
		time_seconds = 0.05f; // 20fps
	}
	keyControl(time_seconds * 5.0f);

	m_update();
	GlobalCamera().movedNotify();
}

void Camera::updateModelview()
{
	_prevAngles = _angles;
	_prevOrigin = _origin;

	modelview = Matrix4::getIdentity();

	// roll, pitch, yaw
	Vector3 radiant_eulerXYZ(0, -_angles[CAMERA_PITCH], _angles[CAMERA_YAW]);

	modelview.translateBy(_origin);
	modelview.rotateByEulerXYZDegrees(radiant_eulerXYZ);
	modelview.multiplyBy(g_radiant2opengl);
	modelview.invert();

	updateVectors();

	m_view->Construct(projection, modelview, width, height);
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
	m_update();
	GlobalCamera().movedNotify();
}

void Camera::freeMove(int dx, int dy) {
	int angleSpeed = getCameraSettings()->angleSpeed();

	// free strafe mode, toggled by the keyboard modifiers
	if (m_strafe) {
		const float strafespeed = GlobalEventManager().MouseEvents().getCameraStrafeSpeed();
		const float forwardStrafeFactor = GlobalEventManager().MouseEvents().getCameraForwardStrafeFactor();

		_origin -= vright * strafespeed * dx;

		if (m_strafe_forward) {
			_origin += vpn * strafespeed * dy * forwardStrafeFactor;
		} else {
			_origin += vup * strafespeed * dy;
		}
	} else { // free rotation
		const float dtime = 0.1f;

		const float zAxisFactor = getCameraSettings()->invertMouseVerticalAxis() ? -1.0f : 1.0f;

		_angles[CAMERA_PITCH] += dy * dtime * angleSpeed * zAxisFactor;

		_angles[CAMERA_YAW] += dx * dtime * angleSpeed;

		if (_angles[CAMERA_PITCH] > 90)
			_angles[CAMERA_PITCH] = 90;
		else if (_angles[CAMERA_PITCH] < -90)
			_angles[CAMERA_PITCH] = -90;

		if (_angles[CAMERA_YAW] >= 360)
			_angles[CAMERA_YAW] -=360;
		else if (_angles[CAMERA_YAW] <= 0)
			_angles[CAMERA_YAW] +=360;
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
	_angles[CAMERA_YAW] += xf * -0.1f * movementSpeed;

	updateModelview();
}

void Camera::setAngles(const Vector3& newAngles)
{
	_angles = newAngles;
	_prevAngles = _angles;

	updateModelview();
	m_update();
	GlobalCamera().movedNotify();
}

const Vector3& Camera::getAngles() const {
	return _angles;
}

void Camera::setOrigin(const Vector3& newOrigin)
{
	_origin = newOrigin;
	_prevOrigin = _origin;

	updateModelview();
	m_update();
	GlobalCamera().movedNotify();
}

const Vector3& Camera::getOrigin() const {
	return _origin;
}

void Camera::moveUpdateAxes() {
	double ya = degrees_to_radians(_angles[CAMERA_YAW]);

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

	m_view->Construct(projection, modelview, width, height);
}

void Camera::onMotionDelta(int x, int y)
{
	mouseMove(x, y);
}

void Camera::pitchUpDiscrete() {
	Vector3 angles = getAngles();

	angles[CAMERA_PITCH] += SPEED_TURN;
	if (angles[CAMERA_PITCH] > 90)
		angles[CAMERA_PITCH] = 90;

	setAngles(angles);
}

void Camera::pitchDownDiscrete() {
	Vector3 angles = getAngles();

	angles[CAMERA_PITCH] -= SPEED_TURN;
	if (angles[CAMERA_PITCH] < -90)
		angles[CAMERA_PITCH] = -90;

	setAngles(angles);
}

void Camera::moveForwardDiscrete() {
	moveUpdateAxes();
	setOrigin(getOrigin() + forward * SPEED_MOVE);
}

void Camera::moveBackDiscrete() {
	moveUpdateAxes();
	setOrigin(getOrigin() + forward * (-SPEED_MOVE));
}

void Camera::moveUpDiscrete() {
	Vector3 origin = getOrigin();

	origin[2] += SPEED_MOVE;

	setOrigin(origin);
}

void Camera::moveDownDiscrete() {
	Vector3 origin = getOrigin();
	origin[2] -= SPEED_MOVE;
	setOrigin(origin);
}

void Camera::moveLeftDiscrete() {
	moveUpdateAxes();
	setOrigin(getOrigin() + right * (-SPEED_MOVE));
}
void Camera::moveRightDiscrete() {
	moveUpdateAxes();
	setOrigin(getOrigin() + right * (SPEED_MOVE));
}

void Camera::rotateLeftDiscrete() {
	Vector3 angles = getAngles();
	angles[CAMERA_YAW] += SPEED_TURN;
	setAngles(angles);
}
void Camera::rotateRightDiscrete() {
	Vector3 angles = getAngles();
	angles[CAMERA_YAW] -= SPEED_TURN;
	setAngles(angles);
}
