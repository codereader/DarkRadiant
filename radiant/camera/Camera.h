#pragma once

#include "icameraview.h"
#include "ieventmanager.h"
#include "math/Vector3.h"
#include "math/Matrix4.h"
#include "wxutil/DeferredMotionDelta.h"
#include "generic/callback.h"
#include "render/View.h"
#include <wx/timer.h>
#include <wx/stopwatch.h>

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
	public wxEvtHandler,
	public ICameraView
{
	static Vector3 _prevOrigin;
	static Vector3 _prevAngles;

	Vector3 _origin;
	Vector3 _angles;

	// Triggers camera movement with a certain rate per second
	wxTimer _moveTimer;

	Callback _queueDraw;
	Callback _forceRedraw;

public:
	int width, height;

	bool timing;

	Vector3 color;   // background

	Vector3 forward, right; // move matrix (TTimo: used to have up but it was not updated)
	Vector3 vup, vpn, vright; // view matrix (taken from the modelview matrix)

	Matrix4 projection;
	Matrix4 modelview;

	bool m_strafe; // true when in strafemode toggled by the ctrl-key
	bool m_strafe_forward; // true when in strafemode by ctrl-key and shift is pressed for forward strafing

	bool freeMoveEnabled;

	unsigned int movementflags;  // movement flags
	wxStopWatch _keyControlTimer;

	float fieldOfView;

	wxutil::DeferredMotionDelta m_mouseMove;

	// Gets called with the accumulated delta values, as buffered by wxutil::DeferredMotionDelta
	void onMotionDelta(int x, int y);

	void camera_keymove(wxTimerEvent& ev);

	render::View& _view;

	Camera(render::View& view, const Callback& queueDraw, const Callback& forceRedraw);
	Camera(const Camera& other) = delete;
	Camera& operator=(const Camera& other) = delete;

	void keyControl(float dtime);
	void setMovementFlags(unsigned int mask);
	void clearMovementFlags(unsigned int mask);
	void keyMove();

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

	void mouseMove(int x, int y);
	void freeMove(int dx, int dy);
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

	void onForwardKey(KeyEventType eventType);
	void onBackwardKey(KeyEventType eventType);
	void onLeftKey(KeyEventType eventType);
	void onRightKey(KeyEventType eventType);
	void onUpKey(KeyEventType eventType);
	void onDownKey(KeyEventType eventType);

}; // class Camera

} // namespace ui
