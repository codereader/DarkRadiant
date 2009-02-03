#ifndef _MAINFRAME_H_
#define _MAINFRAME_H_

#include "imainframe.h"
#include "imainframelayout.h"
#include "gtkutil/PanedPosition.h"
#include "gtkutil/WindowPosition.h"

// Camera window
class CamWnd;
typedef boost::shared_ptr<CamWnd> CamWndPtr;

namespace ui {

class MainFrame :
	public IMainFrame
{
public:
	enum EViewStyle
	{
		eRegular = 0,
		eFloating = 1,
		eSplit = 2,
		eRegularLeft = 3,
	};

	GtkWindow* _window;

private:
	bool _screenUpdatesEnabled;

	struct SplitPaneView {
		GtkWidget* horizPane;
		GtkWidget* vertPane1;
		GtkWidget* vertPane2;
		
		gtkutil::PanedPosition posHPane;
		gtkutil::PanedPosition posVPane1;
		gtkutil::PanedPosition posVPane2;
	} _splitPane;

	struct RegularView {
		GtkWidget* vertPane;
		GtkWidget* horizPane;
		GtkWidget* texCamPane;
		
		gtkutil::PanedPosition posVPane;
		gtkutil::PanedPosition posHPane;
		gtkutil::PanedPosition posTexCamPane;
	} _regular;
	
	gtkutil::WindowPosition _windowPosition;

	EViewStyle m_nCurrentStyle;

	// The current layout object (NULL if no layout active)
	IMainFrameLayoutPtr _currentLayout;

public:
	MainFrame();

	void construct();
	void destroy();

	EViewStyle CurrentStyle() {
		return m_nCurrentStyle;
	}

	bool FloatingGroupDialog() {
		return CurrentStyle() == eFloating || CurrentStyle() == eSplit;
	}

	// IMainFrame implementation
	bool screenUpdatesEnabled();
	void enableScreenUpdates();
	void disableScreenUpdates();

	GtkWindow* getTopLevelWindow();

	void updateAllWindows();

	// Apply the named viewstyle
	void applyLayout(const std::string& name);
	std::string getCurrentLayout();

	// RegisterableModule implementation
	const std::string& getName() const;
	const StringSet& getDependencies() const;
	void initialiseModule(const ApplicationContext& ctx);
	void shutdownModule();

private:
	void create();

	void saveWindowInfo();

	void removeLayout();

	// Restore the window position as saved to the registry
	void restoreWindowPosition();

	void shutdown();
	
	// Creates and returns the topmost application window
	GtkWindow* createTopLevelWindow();
	GtkWidget* createMenuBar();
	
	static gboolean onDelete(GtkWidget* widget, GdkEvent* ev, MainFrame* self);
};

} // namespace ui

#endif /* _MAINFRAME_H_ */
