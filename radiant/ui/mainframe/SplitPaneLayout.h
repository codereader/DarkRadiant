#ifndef _SPLITPANE_LAYOUT_H_
#define _SPLITPANE_LAYOUT_H_

#include <map>
#include "gtkutil/Paned.h"
#include "gtkutil/PanedPosition.h"
#include "imainframelayout.h"

#include "camera/CamWnd.h"
#include "xyview/GlobalXYWnd.h"

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

	struct SplitPaneView
	{
		gtkutil::Paned horizPane;

		gtkutil::Paned vertPane1;
		gtkutil::Paned vertPane2;
		
		gtkutil::PanedPosition posHPane;
		gtkutil::PanedPosition posVPane1;
		gtkutil::PanedPosition posVPane2;

		SplitPaneView() :
			horizPane(gtkutil::Paned::Horizontal),
			vertPane1(gtkutil::Paned::Vertical),
			vertPane2(gtkutil::Paned::Vertical)
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
		GtkWidget* widget;	// the widget to pack (framed widget)
		
		bool isCamera;		// true => is camera view
		XYWndPtr xyWnd;		// the xywnd (NULL if isCamera == true)

		QuadrantInfo() :
			widget(NULL),
			isCamera(false)
		{}
	};

	typedef std::map<Position, QuadrantInfo> WidgetMap;
	WidgetMap _quadrants;

	GtkWidget* _camera;

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
