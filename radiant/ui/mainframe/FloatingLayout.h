#ifndef _FLOATING_LAYOUT_H_
#define _FLOATING_LAYOUT_H_

#include "icommandsystem.h"
#include "gtkutil/window/PersistentTransientWindow.h"
#include "gtkutil/WindowPosition.h"
#include "imainframelayout.h"

namespace ui {

#define FLOATING_LAYOUT_NAME "Floating"

class FloatingLayout;
typedef boost::shared_ptr<FloatingLayout> FloatingLayoutPtr;

class FloatingLayout :
	public IMainFrameLayout
{
	// The floating camera window
	gtkutil::PersistentTransientWindowPtr _floatingCamWnd;

	// The camera window position tracker
	gtkutil::WindowPosition _camWndPosition;

public:
	// IMainFrameLayout implementation
	virtual std::string getName();
	virtual void activate();
	virtual void deactivate();
	virtual void toggleFullscreenCameraView();

	// The creation function, needed by the mainframe layout manager
	static FloatingLayoutPtr CreateInstance();
};

} // namespace ui

#endif /* _FLOATING_LAYOUT_H_ */
