#ifndef _EMBEDDED_LAYOUT_H_
#define _EMBEDDED_LAYOUT_H_

#include "gtkutil/PanedPosition.h"
#include "imainframelayout.h"

#include "camera/CamWnd.h"

namespace Gtk
{
	class HPaned;
	class VPaned;
}

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

	boost::shared_ptr<Gtk::HPaned> _horizPane;
	Gtk::VPaned* _groupCamPane;

	gtkutil::PanedPosition _posHPane;
	gtkutil::PanedPosition _posGroupCamPane;

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

#endif /* _EMBEDDED_LAYOUT_H_ */
