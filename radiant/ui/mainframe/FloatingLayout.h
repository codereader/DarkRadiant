#pragma once

#include "icommandsystem.h"
#include "gtkutil/window/PersistentTransientWindow.h"
#include "gtkutil/WindowPosition.h"
#include "imainframelayout.h"

class FloatingCamWnd;
typedef boost::shared_ptr<FloatingCamWnd> FloatingCamWndPtr;

namespace ui 
{

#define FLOATING_LAYOUT_NAME "Floating"

class FloatingLayout;
typedef boost::shared_ptr<FloatingLayout> FloatingLayoutPtr;

class FloatingLayout :
	public IMainFrameLayout
{
	// The floating camera window
	FloatingCamWndPtr _floatingCamWnd;

	// The camera window position tracker
	wxutil::WindowPosition _camWndPosition;

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
