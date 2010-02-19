#ifndef GuiRenderer_h__
#define GuiRenderer_h__

#include "math/Vector2.h"
#include "gtkutil/GLWidget.h"
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

	Vector2 _viewPortTopLeft;
	Vector2 _viewPortBottomRight;

public:
	// Construct a new renderer
	GuiRenderer();

	void setGui(const GuiPtr& gui);

	// Starts rendering the attached GUI
	// The GL context must be set before calling this method
	void render();

private:
	void render(const GuiWindowDefPtr& window);
};

}

#endif // GuiRenderer_h__
