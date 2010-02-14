#ifndef GuiRenderer_h__
#define GuiRenderer_h__

#include "gtkutil/GLWidget.h"
#include <boost/noncopyable.hpp>

namespace gui
{

class GuiRenderer :
	public boost::noncopyable
{
private:
	// The target GL widget
	gtkutil::GLWidget& _glWidget;

public:
	// Construct a new renderer, taking the target GL widget as argument
	GuiRenderer(gtkutil::GLWidget& glWidget);

	void render();
};

}

#endif // GuiRenderer_h__
