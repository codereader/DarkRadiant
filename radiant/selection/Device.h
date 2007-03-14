#ifndef DEVICE_H_
#define DEVICE_H_

#include "math/Vector2.h"
#include "container/container.h"
#include "generic/callback.h"

// A vector representing the mouse pointer coordinates
typedef Vector2 DeviceVector;
typedef struct _GdkEventButton GdkEventButton;

inline float screen_normalised(float pos, std::size_t size) {
  return ((2.0f * pos) / size) - 1.0f;
}

inline DeviceVector window_to_normalised_device(WindowVector window, std::size_t width, std::size_t height) {
  return DeviceVector(screen_normalised(window.x(), width), screen_normalised(height - 1 - window.y(), height));
}

/* greebo: This returns a number between -1 and +1
 * returns -1, if pos < -1
 * returns pos, if -1 < pos < +1 
 * returns +1, if pos > +1
 * 
 * So this performs some kind of cut-off of the value <pos> at the [-1,+1] boundaries
 */
inline float device_constrained(float pos) {
  return std::min(1.0f, std::max(-1.0f, pos));
}

// See device_constrained(float), this cuts off the DeviceVector's components at -1 or +1
inline DeviceVector device_constrained(DeviceVector device) {
  return DeviceVector(device_constrained(device.x()), device_constrained(device.y()));
}

// greebo: The mouseOperations callbacks, should they really be a global?
typedef Callback1<DeviceVector> MouseEventCallback;
extern Single<MouseEventCallback> g_mouseUpCallback;
extern Single<MouseEventCallback> g_mouseMovedCallback;

#endif /*DEVICE_H_*/
