#pragma once

#include "icommandsystem.h"
#include "imainframelayout.h"

namespace ui 
{

class FloatingCamWnd;
typedef std::shared_ptr<FloatingCamWnd> FloatingCamWndPtr;

#define FLOATING_LAYOUT_NAME "Floating"

class FloatingLayout;
typedef std::shared_ptr<FloatingLayout> FloatingLayoutPtr;

/**
 * \brief
 * Original GIMP-style layout with multiple floating windows
 *
 * This layout is the most flexible, as it allows windows to be positioned and
 * resized arbitrarily, but it is also more cumbersome to set up because the
 * default window positions are not likely to be desirable.
 */
class FloatingLayout: public IMainFrameLayout
{
	// The floating camera window
	FloatingCamWndPtr _floatingCamWnd;

public:
	// IMainFrameLayout implementation
	std::string getName() override;
	void activate() override;
	void deactivate() override;
	void toggleFullscreenCameraView() override;
	void restoreStateFromRegistry() override;

	// The creation function, needed by the mainframe layout manager
	static FloatingLayoutPtr CreateInstance();
};

} // namespace ui
