#ifndef _SPLITPANE_LAYOUT_H_
#define _SPLITPANE_LAYOUT_H_

#include <map>
#include "gtkutil/PanedPosition.h"
#include "imainframelayout.h"

#include "camera/CamWnd.h"
#include "xyview/GlobalXYWnd.h"

namespace Gtk
{
	class HPaned;
	class VPaned;
}

namespace ui
{

#define SPLITPANE_LAYOUT_NAME "SplitPane"

class SplitPaneLayout;
typedef boost::shared_ptr<SplitPaneLayout> SplitPaneLayoutPtr;

class SplitPaneLayout :
	public IMainFrameLayout
{
private:
	// The camera view
	CamWndPtr _camWnd;

	struct SplitPaneView
	{
		boost::shared_ptr<Gtk::HPaned> horizPane;

		Gtk::VPaned* vertPane1;
		Gtk::VPaned* vertPane2;

		gtkutil::PanedPosition posHPane;
		gtkutil::PanedPosition posVPane1;
		gtkutil::PanedPosition posVPane2;

		SplitPaneView() :
			vertPane1(NULL),
			vertPane2(NULL)
		{}

	} _splitPane;

	// Possible positions for the various views
	enum Position
	{
		QuadrantTopLeft,
		QuadrantTopRight,
		QuadrantBottomLeft,
		QuadrantBottomRight,
	};

	Position _cameraPosition;

	// Widget distribution
	struct QuadrantInfo
	{
		Gtk::Widget* widget;	// the widget to pack (framed widget)

		bool isCamera;		// true => is camera view
		XYWndPtr xyWnd;		// the xywnd (NULL if isCamera == true)

		QuadrantInfo() :
			widget(NULL),
			isCamera(false)
		{}
	};

	typedef std::map<Position, QuadrantInfo> WidgetMap;
	WidgetMap _quadrants;

	Gtk::Widget* _camera;

	// Private constructor
	SplitPaneLayout();

public:
	// IMainFrameLayout implementation
	virtual std::string getName();
	virtual void activate();
	virtual void deactivate();
	virtual void toggleFullscreenCameraView();

	// The creation function, needed by the mainframe layout manager
	static SplitPaneLayoutPtr CreateInstance();

private:
	void maximiseCameraSize();
	void restorePanePositions();

	void clearQuadrantInfo();

	Position getCameraPositionFromRegistry();
	void saveCameraPositionToRegistry();

	void setCameraTopLeft(bool newState);
	void setCameraTopRight(bool newState);
	void setCameraBottomLeft(bool newState);
	void setCameraBottomRight(bool newState);

	void updateCameraPositionToggles();

	void constructLayout();
	void constructMenus();
	void deconstructMenus();
	void deconstructLayout();

	void distributeWidgets();

	// Saves the state of this window layout to the given XMLRegistry path (without trailing slash)
	void restoreStateFromPath(const std::string& path);
	void saveStateToPath(const std::string& path);
};

} // namespace ui

#endif /* _SPLITPANE_LAYOUT_H_ */
