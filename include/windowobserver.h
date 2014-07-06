#pragma once

#include "math/Vector2.h"

template<typename Element>
class BasicVector2;
typedef BasicVector2<double> Vector2;
typedef Vector2 WindowVector;
class wxMouseEvent;

/* greebo: The abstract base class defining a window observer.
 * It has to handle all the mouseDown/Up/Move and keyboard events
 * as well as window resizing.
 */
class WindowObserver
{
public:
	virtual ~WindowObserver() {}

	virtual void release() = 0;
	virtual void onSizeChanged(int width, int height) = 0;
	virtual void onMouseMotion(const WindowVector& position, unsigned int state) = 0;

	virtual void onMouseDown(const WindowVector& position, wxMouseEvent& ev) = 0;
	virtual void onMouseUp(const WindowVector& position, wxMouseEvent& ev) = 0;

	virtual void cancelOperation() = 0;
};
