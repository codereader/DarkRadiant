#ifndef SELECTIONBOX_H_
#define SELECTIONBOX_H_

#include "math/Vector2.h"
#include <boost/function.hpp>

class Rectangle
{
public:
	Vector2 min;
	Vector2 max; 

	Rectangle()
	{}

	Rectangle(const Vector2& min_, const Vector2& max_) :
		min(min_),
		max(max_)
	{}

	bool empty() const
	{
		return (min - max).getLengthSquared() == 0;
	}

	// Converts this rectangle to window coordinates, pass width and height of the window
	void toScreenCoords(std::size_t width, std::size_t height)
	{
		min = device2screen(min, width, height);
		max = device2screen(max, width, height);
	}

	// Public typedef
	typedef boost::function<void (const Rectangle&)> Callback;

private:
	Vector2 device2screen(const Vector2& coord, std::size_t width, std::size_t height)
	{
		return Vector2(
			((coord.x() + 1.0) * 0.5) * width, 
			((coord.y() + 1.0) * 0.5) * height
		);
	}
};

// greebo: This returns the coordinates of a rectangle at the mouse position with edge length 2*epsilon 
inline Rectangle SelectionBoxForPoint(const double device_point[2], const double device_epsilon[2])
{
  Rectangle selection_box;
  selection_box.min[0] = device_point[0] - device_epsilon[0];
  selection_box.min[1] = device_point[1] - device_epsilon[1];
  selection_box.max[0] = device_point[0] + device_epsilon[0];
  selection_box.max[1] = device_point[1] + device_epsilon[1];
  return selection_box;
}

/* greebo: Returns the coordinates of the selected rectangle,
 * it is assured that the min values are smaller than the max values */ 
inline Rectangle SelectionBoxForArea(const double device_point[2], const double device_delta[2])
{
  Rectangle selection_box;
  selection_box.min[0] = (device_delta[0] < 0) ? (device_point[0] + device_delta[0]) : (device_point[0]);
  selection_box.min[1] = (device_delta[1] < 0) ? (device_point[1] + device_delta[1]) : (device_point[1]);
  selection_box.max[0] = (device_delta[0] > 0) ? (device_point[0] + device_delta[0]) : (device_point[0]);
  selection_box.max[1] = (device_delta[1] > 0) ? (device_point[1] + device_delta[1]) : (device_point[1]);
  return selection_box;
}

#endif /*SELECTIONBOX_H_*/
