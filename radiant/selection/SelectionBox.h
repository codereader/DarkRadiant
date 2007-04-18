#ifndef SELECTIONBOX_H_
#define SELECTIONBOX_H_

#include "generic/callbackfwd.h"

struct Rectangle {
  double min[2];
  double max[2];
};

typedef Callback1<Rectangle> RectangleCallback;

// greebo: This returns the coordinates of a rectangle at the mouse position with edge length 2*epsilon 
inline const Rectangle SelectionBoxForPoint(const double device_point[2], const double device_epsilon[2]) {
  Rectangle selection_box;
  selection_box.min[0] = device_point[0] - device_epsilon[0];
  selection_box.min[1] = device_point[1] - device_epsilon[1];
  selection_box.max[0] = device_point[0] + device_epsilon[0];
  selection_box.max[1] = device_point[1] + device_epsilon[1];
  return selection_box;
}

/* greebo: Returns the coordinates of the selected rectangle,
 * it is assured that the min values are smaller than the max values */ 
inline const Rectangle SelectionBoxForArea(const double device_point[2], const double device_delta[2]) {
  Rectangle selection_box;
  selection_box.min[0] = (device_delta[0] < 0) ? (device_point[0] + device_delta[0]) : (device_point[0]);
  selection_box.min[1] = (device_delta[1] < 0) ? (device_point[1] + device_delta[1]) : (device_point[1]);
  selection_box.max[0] = (device_delta[0] > 0) ? (device_point[0] + device_delta[0]) : (device_point[0]);
  selection_box.max[1] = (device_delta[1] > 0) ? (device_point[1] + device_delta[1]) : (device_point[1]);
  return selection_box;
}

#endif /*SELECTIONBOX_H_*/
