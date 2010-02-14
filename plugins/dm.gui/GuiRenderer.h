#ifndef GuiRenderer_h__
#define GuiRenderer_h__

#include "gtkutil/GLWidget.h"
#include <boost/noncopyable.hpp>

namespace gui
{

class GuiRenderer :
	public boost::noncopyable
{
public:
	// Construct a new renderer
	GuiRenderer();

	// Starts rendering the attached GUI
	// The GL context must be set before calling this method
	void render();
};

}

#endif // GuiRenderer_h__
