#ifndef CAMERA_H_
#define CAMERA_H_

#include "math/Vector3.h"
#include "math/matrix.h"
#include "timer.h"
#include "gtkutil/cursor.h"
#include "gtkmisc.h"
#include "generic/callback.h"
#include "view.h"

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

class Camera {

public:
	int width, height;

	bool timing;

	Vector3 origin;
	Vector3 angles;

	Vector3 color;   // background

	Vector3 forward, right; // move matrix (TTimo: used to have up but it was not updated)
	Vector3 vup, vpn, vright; // view matrix (taken from the modelview matrix)

	Matrix4 projection;
	Matrix4 modelview;

	bool m_strafe; // true when in strafemode toggled by the ctrl-key
	bool m_strafe_forward; // true when in strafemode by ctrl-key and shift is pressed for forward strafing

	unsigned int movementflags;  // movement flags
	Timer m_keycontrol_timer;
	guint m_keymove_handler;


	float fieldOfView;

	DeferredMotionDelta m_mouseMove;

	static void motionDelta(int x, int y, void* data);
	static gboolean camera_keymove(gpointer data);

	View* m_view;
	Callback m_update;

	Camera(View* view, const Callback& update);

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

	Vector3 getOrigin() const;
	void setOrigin(const Vector3& newOrigin);

	Vector3 getAngles() const;
	void setAngles(const Vector3& newAngles);

	void mouseMove(int x, int y);
	void freeMove(int dx, int dy);
	void mouseControl(int x, int y);

	void pitchUpDiscrete();
	void pitchDownDiscrete();
	void rotateRightDiscrete();
	void rotateLeftDiscrete();
	void moveRightDiscrete();
	void moveLeftDiscrete();
	void moveDownDiscrete();
	void moveUpDiscrete();
	void moveBackDiscrete();
	void moveForwardDiscrete();

}; // class Camera

#endif /*CAMERA_H_*/
