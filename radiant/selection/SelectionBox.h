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

	// Constructs a Rectangle centered at the given point, with edge length 2*epsilon
	// This is used to construct small rectangles at some mouse coordinates for selection tests
	static Rectangle ConstructFromPoint(const Vector2& point, const Vector2& epsilon)
	{
		return Rectangle(point - epsilon, point + epsilon);
	}

	// greebo: Constructs a Rectangle from (start - delta) to (start + delta) and ensures
	// that the resulting Rectangle's min is smaller than its max, in case delta has negative components
	static Rectangle ConstructFromArea(const Vector2& start, const Vector2& delta)
	{
		return Rectangle(
			Vector2(
				delta[0] < 0 ? (start[0] + delta[0]) : start[0],
				delta[1] < 0 ? (start[1] + delta[1]) : start[1]
			),
			Vector2(
				delta[0] > 0 ? (start[0] + delta[0]) : start[0],
				delta[1] > 0 ? (start[1] + delta[1]) : start[1]
			)
		);
	}

private:
	Vector2 device2screen(const Vector2& coord, std::size_t width, std::size_t height)
	{
		return Vector2(
			((coord.x() + 1.0) * 0.5) * width,
			((coord.y() + 1.0) * 0.5) * height
		);
	}
};

#endif /* SELECTIONBOX_H_ */
