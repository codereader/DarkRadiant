#ifndef GuiRenderer_h__
#define GuiRenderer_h__

#include "math/Vector2.h"
#include <boost/noncopyable.hpp>

#include "Gui.h"
#include <vector>

namespace gui
{

class GuiRenderer :
	public boost::noncopyable
{
private:
	GuiPtr _gui;

	Vector2 _areaTopLeft;
	Vector2 _areaBottomRight;

	// Whether invisible windowDefs should be rendered anyway
	bool _ignoreVisibility;

public:
	// Construct a new renderer
	GuiRenderer();

	void setGui(const GuiPtr& gui);

	void setIgnoreVisibility(bool ignoreVisibility);

	// Sets the visible area to be rendered, in absolute GUI coordinates [0,0..640,480]
	void setVisibleArea(const Vector2& topLeft, const Vector2& bottomRight);

	// Starts rendering the attached GUI
	// The GL context must be set before calling this method
	void render();

private:
	void render(const GuiWindowDefPtr& window);
};

}

#endif // GuiRenderer_h__
