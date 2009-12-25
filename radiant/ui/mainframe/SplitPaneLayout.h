#ifndef _SPLITPANE_LAYOUT_H_
#define _SPLITPANE_LAYOUT_H_

#include "gtkutil/PanedPosition.h"
#include "imainframelayout.h"

#include "camera/CamWnd.h"

namespace ui {

#define SPLITPANE_LAYOUT_NAME "SplitPane"

class SplitPaneLayout;
typedef boost::shared_ptr<SplitPaneLayout> SplitPaneLayoutPtr;

class SplitPaneLayout :
	public IMainFrameLayout
{
private:
	// The camera view
	CamWndPtr _camWnd;

	struct SplitPaneView {
		GtkWidget* horizPane;
		GtkWidget* vertPane1;
		GtkWidget* vertPane2;
		
		gtkutil::PanedPosition posHPane;
		gtkutil::PanedPosition posVPane1;
		gtkutil::PanedPosition posVPane2;
	} _splitPane;

public:
	// IMainFrameLayout implementation
	virtual std::string getName();
	virtual void activate();
	virtual void deactivate();

	// Command to (un)maximise the camera view
	void toggleCameraFullScreen(const cmd::ArgumentList& args);

	// The creation function, needed by the mainframe layout manager
	static SplitPaneLayoutPtr CreateInstance();

private:
	void maximiseCameraSize();
	void restorePanePositions();

	// Saves the state of this window layout to the given XMLRegistry path (without trailing slash)
	void restoreStateFromPath(const std::string& path);
	void saveStateToPath(const std::string& path);
};

} // namespace ui

#endif /* _SPLITPANE_LAYOUT_H_ */
