#pragma once

#include <map>
#include "wxutil/PanedPosition.h"
#include "imainframelayout.h"

#include "xyview/GlobalXYWnd.h"

namespace ui
{

class CamWnd;
typedef std::shared_ptr<CamWnd> CamWndPtr;

#define SPLITPANE_LAYOUT_NAME "SplitPane"

class SplitPaneLayout;
typedef std::shared_ptr<SplitPaneLayout> SplitPaneLayoutPtr;

class SplitPaneLayout :
	public IMainFrameLayout
{
private:
	// The camera view
	CamWndPtr _camWnd;

	struct SplitPaneView
	{
		wxSplitterWindow* horizPane;

		wxSplitterWindow* vertPane1;
		wxSplitterWindow* vertPane2;

		wxutil::PanedPosition posHPane;
		wxutil::PanedPosition posVPane1;
		wxutil::PanedPosition posVPane2;

		void clear() 
		{
			horizPane = NULL;
			vertPane1 = NULL;
			vertPane2 = NULL;
		}

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
	struct Quadrant
	{
		enum ChildType
		{
			Camera,
			OrthoView,
		};

		ChildType type;

		wxWindow* widget;
		XYWndPtr xyWnd;		// the xywnd (NULL if isCamera == true)

		Quadrant() :
			type(OrthoView),
			widget(NULL)
		{}
	};

	typedef std::map<Position, Quadrant> WidgetMap;
	WidgetMap _quadrants;

	// Private constructor
	SplitPaneLayout();

public:
	// IMainFrameLayout implementation
	std::string getName() override;
	void activate() override;
	void deactivate() override;
	void toggleFullscreenCameraView() override;
	void restoreStateFromRegistry() override;

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

    void createViews();
	void distributeWidgets();

	// Saves the state of this window layout to the given XMLRegistry path (without trailing slash)
    // RestoreState doesn't create any views, restores only sash position and view types
	void restoreStateFromPath(const std::string& path);
	void saveStateToPath(const std::string& path);
};

} // namespace ui
