#pragma once

#include "icameraview.h"
#include "math/Vector3.h"
#include "math/Matrix4.h"
#include "generic/callback.h"
#include "render/View.h"

namespace ui
{

#define SPEED_MOVE 32
#define SPEED_TURN 22.5

const unsigned int MOVE_NONE = 0;
const unsigned int MOVE_FORWARD = 1 << 0;
const unsigned int MOVE_BACK = 1 << 1;
const unsigned int MOVE_ROTRIGHT = 1 << 2;
const unsigned int MOVE_ROTLEFT = 1 << 3;
const unsigned int MOVE_STRAFERIGHT = 1 << 4;
const unsigned int MOVE_STRAFELEFT = 1 << 5;
const unsigned int MOVE_UP = 1 << 6;
const unsigned int MOVE_DOWN = 1 << 7;
const unsigned int MOVE_PITCHUP = 1 << 8;
const unsigned int MOVE_PITCHDOWN = 1 << 9;
const unsigned int MOVE_ALL = MOVE_FORWARD|MOVE_BACK|MOVE_ROTRIGHT|MOVE_ROTLEFT|MOVE_STRAFERIGHT|MOVE_STRAFELEFT|MOVE_UP|MOVE_DOWN|MOVE_PITCHUP|MOVE_PITCHDOWN;

class Camera :
	public ICameraView
{
	static Vector3 _prevOrigin;
	static Vector3 _prevAngles;

	Vector3 _origin;
	Vector3 _angles;

	Callback _queueDraw;
	Callback _forceRedraw;

	float _fieldOfView;
public:
	int width, height;

	Vector3 forward, right; // move matrix (TTimo: used to have up but it was not updated)
	Vector3 vup, vpn, vright; // view matrix (taken from the modelview matrix)

	Matrix4 projection;
	Matrix4 modelview;

	render::View& _view;

	Camera(render::View& view, const Callback& queueDraw, const Callback& forceRedraw);
	Camera(const Camera& other) = delete;
	Camera& operator=(const Camera& other) = delete;

	void updateVectors();
	void updateModelview();
	void updateProjection();

	float getFarClipPlane() const;

	// Returns true if cubic clipping is "on"
	bool farClipEnabled() const;

	void freemoveUpdateAxes();
	void moveUpdateAxes();

	const Vector3& getCameraOrigin() const override;
	void setCameraOrigin(const Vector3& newOrigin) override;

	const Vector3& getCameraAngles() const override;
	void setCameraAngles(const Vector3& newAngles) override;

	const Vector3& getRightVector() const override;
	const Vector3& getUpVector() const override;
	const Vector3& getForwardVector() const override;

	int getDeviceWidth() const override;
	int getDeviceHeight() const override;

	SelectionTestPtr createSelectionTestForPoint(const Vector2& point) override;
	const VolumeTest& getVolumeTest() const override;

	void queueDraw() override;
	void forceRedraw() override;

	void mouseControl(int x, int y);

	void pitchUpDiscrete();
	void pitchDownDiscrete();
	void rotateRightDiscrete();
	void rotateLeftDiscrete();
	void moveRightDiscrete(double units);
	void moveLeftDiscrete(double units);
	void moveDownDiscrete(double units);
	void moveUpDiscrete(double units);
	void moveBackDiscrete(double units);
	void moveForwardDiscrete(double units);

}; // class Camera

} // namespace ui
