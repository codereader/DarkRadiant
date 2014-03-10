#pragma once

#include "gtkutil/PanedPosition.h"
#include "imainframelayout.h"

#include "camera/CamWnd.h"

namespace ui
{

#define EMBEDDED_LAYOUT_NAME "Embedded"

class EmbeddedLayout;
typedef boost::shared_ptr<EmbeddedLayout> EmbeddedLayoutPtr;

class EmbeddedLayout :
	public IMainFrameLayout
{
private:
	// The camera view
	CamWndPtr _camWnd;

	wxSplitterWindow* _horizPane;
	wxSplitterWindow* _groupCamPane;

	wxutil::PanedPosition _posHPane;
	wxutil::PanedPosition _posGroupCamPane;

public:
	// IMainFrameLayout implementation
	virtual std::string getName();
	virtual void activate();
	virtual void deactivate();
	virtual void toggleFullscreenCameraView();

	// The creation function, needed by the mainframe layout manager
	static EmbeddedLayoutPtr CreateInstance();

private:
	void maximiseCameraSize();
	void restorePanePositions();

	// Saves the state of this window layout to the given XMLRegistry path (without trailing slash)
	void restoreStateFromPath(const std::string& path);
	void saveStateToPath(const std::string& path);
};

} // namespace ui
